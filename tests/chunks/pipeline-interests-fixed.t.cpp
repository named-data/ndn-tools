/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2024, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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
 * @author Chavoosh Ghasemi
 */

#include "tools/chunks/catchunks/pipeline-interests-fixed.hpp"
#include "tools/chunks/catchunks/data-fetcher.hpp"

#include "pipeline-interests-fixture.hpp"

#include <cmath>

namespace ndn::chunks::tests {

class PipelineInterestFixedFixture : public PipelineInterestsFixture
{
public:
  PipelineInterestFixedFixture()
  {
    opt.interestLifetime = 1_s;
    opt.maxRetriesOnTimeoutOrNack = 3;
    opt.isQuiet = true;
    opt.maxPipelineSize = 5;
    createPipeline();
  }

  void
  createPipeline()
  {
    auto pline = std::make_unique<PipelineInterestsFixed>(face, opt);
    pipeline = pline.get();
    setPipeline(std::move(pline));
  }

protected:
  Options opt;
  PipelineInterestsFixed* pipeline;
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestPipelineInterestsFixed, PipelineInterestFixedFixture)

BOOST_AUTO_TEST_CASE(FullPipeline)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  for (uint64_t i = 0; i < nDataSegments - 1; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
    BOOST_CHECK_EQUAL(pipeline->m_nReceived, i + 1);

    if (i < nDataSegments - opt.maxPipelineSize) {
      BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize + i + 1);
      // check if the interest for the segment i is well formed
      const auto& sentInterest = face.sentInterests[i];
      BOOST_CHECK_EQUAL(sentInterest.getCanBePrefix(), false);
      BOOST_CHECK_EQUAL(sentInterest.getMustBeFresh(), opt.mustBeFresh);
      BOOST_CHECK_EQUAL(Name(name).isPrefixOf(sentInterest.getName()), true);
      BOOST_CHECK_EQUAL(getSegmentFromPacket(sentInterest), i);
    }
    else {
      // all the interests have been sent for all the segments
      BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments);
    }
  }

  BOOST_CHECK_EQUAL(hasFailed, false);

  advanceClocks(ndn::DEFAULT_INTEREST_LIFETIME, opt.maxRetriesOnTimeoutOrNack + 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(TimeoutAllSegments)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  for (int i = 0; i < opt.maxRetriesOnTimeoutOrNack; ++i) {
    advanceClocks(opt.interestLifetime);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * (i + 2));
    BOOST_CHECK_EQUAL(pipeline->m_nReceived, 0);

    // A single retry for every pipeline element
    for (size_t j = 0; j < opt.maxPipelineSize; ++j) {
      const auto& interest = face.sentInterests[(opt.maxPipelineSize * (i + 1)) + j];
      BOOST_CHECK_EQUAL(static_cast<size_t>(getSegmentFromPacket(interest)), j);
    }
  }

  advanceClocks(opt.interestLifetime);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(TimeoutAfterFinalBlockIdReceived)
{
  // the FinalBlockId is sent with the first segment, after the first segment failure the pipeline
  // should fail

  nDataSegments = 18;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  // send a single segment for each pipeline element but not the first element
  advanceClocks(opt.interestLifetime);
  for (uint64_t i = 1; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
  }

  // send a single data packet for each pipeline element
  advanceClocks(opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack - 1);
  for (uint64_t i = 0; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(opt.maxPipelineSize + i));
    advanceClocks(time::nanoseconds(1));
  }
  advanceClocks(opt.interestLifetime);

  size_t interestAfterFailure = face.sentInterests.size();

  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
  BOOST_CHECK_EQUAL(hasFailed, true);

  // these new segments should not generate new interests
  advanceClocks(opt.interestLifetime);
  for (uint64_t i = 0; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(opt.maxPipelineSize * 2 + i - 1));
    advanceClocks(time::nanoseconds(1));
  }

  // no more interests after a failure
  advanceClocks(opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack);
  BOOST_CHECK_EQUAL(interestAfterFailure, face.sentInterests.size());
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
}

BOOST_AUTO_TEST_CASE(TimeoutBeforeFinalBlockIdReceived)
{
  // the FinalBlockId is sent only with the last segment, all segments are sent except for the
  // second one (segment #1); all segments are received correctly until the FinalBlockId is received

  nDataSegments = 22;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  advanceClocks(opt.interestLifetime);
  for (uint64_t i = 2; i < opt.maxPipelineSize; ++i) {
    face.receive(*makeDataWithSegment(i, false));
    advanceClocks(time::nanoseconds(1));

    const auto& lastInterest = face.sentInterests.back();
    BOOST_CHECK_EQUAL(getSegmentFromPacket(lastInterest), opt.maxPipelineSize + i - 2);
  }
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * 3 - 2);

  // nack for the first pipeline element (segment #0)
  auto nack = std::make_shared<lp::Nack>(face.sentInterests[opt.maxPipelineSize]);
  nack->setReason(lp::NackReason::DUPLICATE);
  face.receive(*nack);

  // all the pipeline elements are two retries near the timeout error, but not the
  // second (segment #1) that is only one retry near the timeout
  advanceClocks(opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack - 1);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for the first pipeline element (segment #0)
  face.receive(*makeDataWithSegment(0, false));
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for all the pipeline element, but not the second (segment #1)
  for (uint64_t i = opt.maxPipelineSize; i < nDataSegments; ++i) {
    if (i == nDataSegments - 1) {
      face.receive(*makeDataWithSegment(i, true));
    }
    else {
      face.receive(*makeDataWithSegment(i, false));
    }
    advanceClocks(time::nanoseconds(1));
  }
  // timeout for the second pipeline element (segment #1), this should trigger an error
  advanceClocks(opt.interestLifetime);

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments - 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(SegmentReceivedAfterTimeout)
{
  // the FinalBlockId is never sent, all the pipeline elements with a segment number greater than
  // segment #0 will fail, after this failure also segment #0 fail and this should trigger an error

  nDataSegments = 22;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  advanceClocks(opt.interestLifetime);

  // nack for the first pipeline element (segment #0)
  auto nack = std::make_shared<lp::Nack>(face.sentInterests[opt.maxPipelineSize]);
  nack->setReason(lp::NackReason::DUPLICATE);
  face.receive(*nack);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // timeout for all the pipeline elements, but not the first (segment #0)
  advanceClocks(opt.interestLifetime, opt.maxRetriesOnTimeoutOrNack);
  BOOST_CHECK_EQUAL(hasFailed, false);

  // data for the first pipeline element (segment #0), this should trigger an error because the
  // other pipeline elements failed
  face.receive(*makeDataWithSegment(0, false));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 1);
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(CongestionAllSegments)
{
  nDataSegments = 13;
  BOOST_ASSERT(nDataSegments > opt.maxPipelineSize);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize);

  // send nack for all the pipeline elements first interest
  for (size_t i = 0; i < opt.maxPipelineSize; i++) {
    auto nack = std::make_shared<lp::Nack>(face.sentInterests[i]);
    nack->setReason(lp::NackReason::CONGESTION);
    face.receive(*nack);
    advanceClocks(time::nanoseconds(1));
  }

  // send nack for all the pipeline elements interests after the first
  for (int i = 1; i <= opt.maxRetriesOnTimeoutOrNack; ++i) {
    time::milliseconds backoffTime(static_cast<uint64_t>(std::pow(2, i)));
    if (backoffTime > DataFetcher::MAX_CONGESTION_BACKOFF_TIME)
      backoffTime = DataFetcher::MAX_CONGESTION_BACKOFF_TIME;

    advanceClocks(backoffTime);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), opt.maxPipelineSize * (i +1));

    // A single retry for every pipeline element
    for (size_t j = 0; j < opt.maxPipelineSize; ++j) {
      const auto& interest = face.sentInterests[(opt.maxPipelineSize * i) + j];
      BOOST_CHECK_LT(static_cast<size_t>(getSegmentFromPacket(interest)), opt.maxPipelineSize);
    }

    for (size_t j = 0; j < opt.maxPipelineSize; j++) {
      auto nack = std::make_shared<lp::Nack>(face.sentInterests[(opt.maxPipelineSize * i) + j]);
      nack->setReason(lp::NackReason::CONGESTION);
      face.receive(*nack);
      advanceClocks(time::nanoseconds(1));
    }
  }

  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_SUITE_END() // TestPipelineInterests
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace ndn::chunks::tests
