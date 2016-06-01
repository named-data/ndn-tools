/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2016,  Regents of the University of California,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University.
 *
 * This file is part of ndn-tools (Named Data Networking Essential Tools).
 * See AUTHORS.md for complete list of ndn-tools authors and contributors.
 *
 * ndn-tools is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Andrea Tosatto
 */

#include "tools/chunks/catchunks/pipeline-interests-fixed-window.hpp"
#include "tools/chunks/catchunks/data-fetcher.hpp"

#include "pipeline-interests-fixture.hpp"

namespace ndn {
namespace chunks {
namespace tests {

class PipelineInterestFixedWindowFixture : public PipelineInterestsFixture
{
public:
  typedef PipelineInterestsFixedWindowOptions Options;

public:
  PipelineInterestFixedWindowFixture()
    : PipelineInterestsFixture()
    , opt(makeOptions())
  {
    setPipeline(make_unique<PipelineInterestsFixedWindow>(face, PipelineInterestsFixedWindow::Options(opt)));
  }
protected:
  Options opt;

private:
  static Options
  makeOptions()
  {
    Options options;
    options.isVerbose = false;
    options.interestLifetime = time::seconds(1);
    options.maxRetriesOnTimeoutOrNack = 3;
    options.maxPipelineSize = 5;
    return options;
  }
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_AUTO_TEST_SUITE(TestPipelineInterestsFixedWindow)

BOOST_FIXTURE_TEST_CASE(FewerSegmentsThanPipelineCapacity, PipelineInterestFixedWindowFixture)
{
  nDataSegments = 3;
  BOOST_ASSERT(nDataSegments <= opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), nDataSegments - 1);

  for (uint64_t i = 0; i < nDataSegments - 1; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1), 1);

    BOOST_CHECK_EQUAL(nReceivedSegments, i);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), nDataSegments - 1);
    // check if the interest for the segment i+1 is well formed
    auto sentInterest = face.sentInterests[i];
    BOOST_CHECK_EQUAL(sentInterest.getExclude().size(), 0);
    BOOST_CHECK_EQUAL(sentInterest.getMaxSuffixComponents(), 1);
    BOOST_CHECK_EQUAL(sentInterest.getMustBeFresh(), opt.mustBeFresh);
    BOOST_CHECK_EQUAL(Name(name).isPrefixOf(sentInterest.getName()), true);
    BOOST_CHECK_EQUAL(sentInterest.getName()[-1].toSegment(), i + 1);
  }

  BOOST_CHECK_EQUAL(hasFailed, false);

  advanceClocks(io, ndn::DEFAULT_INTEREST_LIFETIME, opt.maxRetriesOnTimeoutOrNack + 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_FIXTURE_TEST_CASE(FullPipeline, PipelineInterestFixedWindowFixture)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  for (uint64_t i = 0; i < nDataSegments - 1; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1), 1);
    BOOST_CHECK_EQUAL(nReceivedSegments, i + 1);

    if (i < nDataSegments - opt.maxPipelineSize - 1) {
      BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize + i + 1);
      // check if the interest for the segment i+1 is well formed
      auto sentInterest = face.sentInterests[i];
      BOOST_CHECK_EQUAL(sentInterest.getExclude().size(), 0);
      BOOST_CHECK_EQUAL(sentInterest.getMaxSuffixComponents(), 1);
      BOOST_CHECK_EQUAL(sentInterest.getMustBeFresh(), opt.mustBeFresh);
      BOOST_CHECK_EQUAL(Name(name).isPrefixOf(sentInterest.getName()), true);
      BOOST_CHECK_EQUAL(sentInterest.getName()[-1].toSegment(), i);
    }
    else {
      // all the interests have been sent for all the segments
      BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments - 1);
    }
  }

  BOOST_CHECK_EQUAL(hasFailed, false);
}

BOOST_FIXTURE_TEST_CASE(TimeoutAllSegments, PipelineInterestFixedWindowFixture)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  for (int i = 0; i < opt.maxRetriesOnTimeoutOrNack; ++i) {
    advanceClocks(io, opt.interestLifetime, 1);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * (i + 2));
    BOOST_CHECK_EQUAL(nReceivedSegments, 0);

    // A single retry for every pipeline element
    for (size_t j = 0; j < opt.maxPipelineSize; ++j) {
      auto interest = face.sentInterests[(opt.maxPipelineSize * (i + 1)) + j];
      BOOST_CHECK_EQUAL(static_cast<size_t>(interest.getName()[-1].toSegment()), j);
    }
  }

  advanceClocks(io, opt.interestLifetime, 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_FIXTURE_TEST_CASE(TimeoutAfterFinalBlockIdReceived, PipelineInterestFixedWindowFixture)
{
  // the FinalBlockId is sent with the first segment, after the first segment failure the pipeline
  // should fail

  nDataSegments = 18;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  // send a single segment for each pipeline element but not the first element
  advanceClocks(io, opt.interestLifetime, 1);
  for (uint64_t i = 1; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1), 1);
  }

  // send a single data packet for each pipeline element
  advanceClocks(io, opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack - 1);
  for (uint64_t i = 0; i < opt.maxPipelineSize - 1; ++i) {
    face.receive(*makeDataWithSegment(opt.maxPipelineSize + i));
    advanceClocks(io, time::nanoseconds(1), 1);
  }
  advanceClocks(io, opt.interestLifetime, 1);

  size_t interestAfterFailure = face.sentInterests.size();

  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
  BOOST_CHECK_EQUAL(hasFailed, true);

  // these new segments should not generate new interests
  advanceClocks(io, opt.interestLifetime, 1);
  for (uint64_t i = 0; i < opt.maxPipelineSize - 1; ++i) {
    face.receive(*makeDataWithSegment(opt.maxPipelineSize * 2 + i - 1));
    advanceClocks(io, time::nanoseconds(1), 1);
  }

  // no more interests after a failure
  advanceClocks(io, opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack);
  BOOST_CHECK_EQUAL(interestAfterFailure, face.sentInterests.size());
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
}

BOOST_FIXTURE_TEST_CASE(TimeoutBeforeFinalBlockIdReceived, PipelineInterestFixedWindowFixture)
{
  // the FinalBlockId is sent only with the last segment, all segments are sent except for the
  // second one (segment #1); all segments are received correctly until the FinalBlockId is received

  nDataSegments = 22;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1, false));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  advanceClocks(io, opt.interestLifetime, 1);
  for (uint64_t i = 2; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(i, false));
    advanceClocks(io, time::nanoseconds(1), 1);

    auto lastInterest = face.sentInterests.back();
    BOOST_CHECK_EQUAL(lastInterest.getName()[-1].toSegment(), opt.maxPipelineSize + i - 2);
  }
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * 3 - 2);

  // nack for the first pipeline element (segment #0)
  auto nack = make_shared<lp::Nack>(face.sentInterests[opt.maxPipelineSize]);
  nack->setReason(lp::NackReason::DUPLICATE);
  face.receive(*nack);

  // all the pipeline elements are two retries near the timeout error, but not the
  // second (segment #1) that is only one retry near the timeout
  advanceClocks(io, opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack - 1);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for the first pipeline element (segment #0)
  face.receive(*makeDataWithSegment(0, false));
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for all the pipeline element, but not the second (segment #1)
  for (uint64_t i = opt.maxPipelineSize; i < nDataSegments - 1; ++i) {
    if (i == nDataSegments - 2) {
      face.receive(*makeDataWithSegment(i, true));
    }
    else {
      face.receive(*makeDataWithSegment(i, false));
    }
    advanceClocks(io, time::nanoseconds(1), 1);
  }
  // timeout for the second pipeline element (segment #1), this should trigger an error
  advanceClocks(io, opt.interestLifetime, 1);

  BOOST_CHECK_EQUAL(nReceivedSegments, nDataSegments - 2);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_FIXTURE_TEST_CASE(SegmentReceivedAfterTimeout, PipelineInterestFixedWindowFixture)
{
  // the FinalBlockId is never sent, all the pipeline elements with a segment number greater than
  // segment #0 will fail, after this failure also segment #0 fail and this should trigger an error

  nDataSegments = 22;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1, false));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  advanceClocks(io, opt.interestLifetime, 1);

  // nack for the first pipeline element (segment #0)
  auto nack = make_shared<lp::Nack>(face.sentInterests[opt.maxPipelineSize]);
  nack->setReason(lp::NackReason::DUPLICATE);
  face.receive(*nack);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // timeout for all the pipeline elements, but not the first (segment #0)
  advanceClocks(io, opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for the first pipeline element (segment #0), this should trigger an error because the
  // others pipeline elements failed
  face.receive(*makeDataWithSegment(0, false));
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_CHECK_EQUAL(nReceivedSegments, 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_FIXTURE_TEST_CASE(CongestionAllSegments, PipelineInterestFixedWindowFixture)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  runWithData(*makeDataWithSegment(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  // send nack for all the pipeline elements first interest
  for (size_t j = 0; j < opt.maxPipelineSize; j++) {
    auto nack = make_shared<lp::Nack>(face.sentInterests[j]);
    nack->setReason(lp::NackReason::CONGESTION);
    face.receive(*nack);
    advanceClocks(io, time::nanoseconds(1), 1);
  }

  // send nack for all the pipeline elements interests after the first
  for (int i = 1; i <= opt.maxRetriesOnTimeoutOrNack; ++i) {
    time::milliseconds backoffTime(static_cast<uint64_t>(std::pow(2, i)));
    if (backoffTime > DataFetcher::MAX_CONGESTION_BACKOFF_TIME)
      backoffTime = DataFetcher::MAX_CONGESTION_BACKOFF_TIME;

    advanceClocks(io, backoffTime, 1);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * (i +1));

    // A single retry for every pipeline element
    for (size_t j = 0; j < opt.maxPipelineSize; ++j) {
      auto interest = face.sentInterests[(opt.maxPipelineSize * i) + j];
      BOOST_CHECK_LT(static_cast<size_t>(interest.getName()[-1].toSegment()), opt.maxPipelineSize);
    }

    for (size_t j = 0; j < opt.maxPipelineSize; j++) {
      auto nack = make_shared<lp::Nack>(face.sentInterests[(opt.maxPipelineSize * i) + j]);
      nack->setReason(lp::NackReason::CONGESTION);
      face.receive(*nack);
      advanceClocks(io, time::nanoseconds(1), 1);
    }
  }

  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_SUITE_END() // TestPipelineInterests
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
