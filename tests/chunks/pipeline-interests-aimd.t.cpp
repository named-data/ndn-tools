/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2018, Regents of the University of California,
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
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 */

#include "tools/chunks/catchunks/pipeline-interests-aimd.hpp"
#include "tools/chunks/catchunks/options.hpp"

#include "pipeline-interests-fixture.hpp"

namespace ndn {
namespace chunks {
namespace aimd {
namespace tests {

using namespace ndn::tests;

class PipelineInterestAimdFixture : public chunks::tests::PipelineInterestsFixture
{
public:
  PipelineInterestAimdFixture()
    : opt(makePipelineOptions())
    , rttEstimator(makeRttEstimatorOptions())
  {
    createPipeline();
  }

  void
  createPipeline()
  {
    auto pline = make_unique<PipelineInterestsAimd>(face, rttEstimator, opt);
    aimdPipeline = pline.get();
    setPipeline(std::move(pline));
  }

private:
  static PipelineInterestsAimd::Options
  makePipelineOptions()
  {
    PipelineInterestsAimd::Options pipelineOptions;
    pipelineOptions.isQuiet = true;
    pipelineOptions.isVerbose = false;
    pipelineOptions.disableCwa = false;
    pipelineOptions.ignoreCongMarks = false;
    pipelineOptions.resetCwndToInit = false;
    pipelineOptions.initCwnd = 1.0;
    pipelineOptions.aiStep = 1.0;
    pipelineOptions.mdCoef = 0.5;
    pipelineOptions.initSsthresh = std::numeric_limits<int>::max();
    return pipelineOptions;
  }

  static RttEstimator::Options
  makeRttEstimatorOptions()
  {
    RttEstimator::Options rttOptions;
    rttOptions.alpha = 0.125;
    rttOptions.beta = 0.25;
    rttOptions.k = 4;
    rttOptions.minRto = Milliseconds(200);
    rttOptions.maxRto = Milliseconds(4000);
    return rttOptions;
  }

protected:
  PipelineInterestsAimd::Options opt;
  RttEstimator rttEstimator;
  PipelineInterestsAimd* aimdPipeline;
  static constexpr double MARGIN = 0.01;
};

constexpr double PipelineInterestAimdFixture::MARGIN;

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestPipelineInterestsAimd, PipelineInterestAimdFixture)

BOOST_AUTO_TEST_CASE(SlowStart)
{
  nDataSegments = 4;
  aimdPipeline->m_ssthresh = 8.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  double preCwnd = aimdPipeline->m_cwnd;
  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  for (uint64_t i = 1; i < nDataSegments - 1; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
    BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd - preCwnd, 1, MARGIN);
    preCwnd = aimdPipeline->m_cwnd;
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments - 1);
}

BOOST_AUTO_TEST_CASE(CongestionAvoidance)
{
  nDataSegments = 8;
  aimdPipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  double preCwnd = aimdPipeline->m_cwnd;
  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  for (uint64_t i = 1; i < aimdPipeline->m_ssthresh; ++i) { // slow start
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
    preCwnd = aimdPipeline->m_cwnd;
  }

  BOOST_CHECK_CLOSE(preCwnd, aimdPipeline->m_ssthresh, MARGIN);

  for (uint64_t i = aimdPipeline->m_ssthresh; i < nDataSegments - 1; ++i) { // congestion avoidance
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
    BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd - preCwnd, opt.aiStep / floor(aimdPipeline->m_cwnd), MARGIN);
    preCwnd = aimdPipeline->m_cwnd;
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments - 1);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  nDataSegments = 8;
  aimdPipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  // receive segment 1 and segment 2
  for (uint64_t i = 1; i < 3; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 3);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 3, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 5); // request for segment 5 has been sent

  advanceClocks(io, time::milliseconds(100));

  // receive segment 4
  face.receive(*makeDataWithSegment(4));
  advanceClocks(io, time::nanoseconds(1));

  // receive segment 5
  face.receive(*makeDataWithSegment(5));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 4.25, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments - 1); // all the segment requests have been sent

  // timeout segment 3
  advanceClocks(io, time::milliseconds(150));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 2.125, MARGIN); // window size drop to 1/2 of previous size
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 1);

  // receive segment 6, retransmit 3
  face.receive(*makeDataWithSegment(6));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 2.625, MARGIN); // congestion avoidance
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 0);
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[3], 1);
}

BOOST_AUTO_TEST_CASE(CongestionMarksWithCwa)
{
  nDataSegments = 7;
  aimdPipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  // receive segments 1 to 4
  for (uint64_t i = 1; i < 5; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 4.25, MARGIN);

  // receive segment 5 with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(5));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 2.125, MARGIN); // window size drops to 1/2 of previous size
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments - 1); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 2.125, MARGIN); // conservative window adaption (window size should not decrease)
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packets
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[5], 0);
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(aimdPipeline->m_nCongMarks, 2);
}

BOOST_AUTO_TEST_CASE(CongestionMarksWithoutCwa)
{
  opt.disableCwa = true;
  createPipeline();

  nDataSegments = 7;
  aimdPipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  // receive segments 1 to 4
  for (uint64_t i = 1; i < 5; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 4.25, MARGIN);

  // receive segment 5 with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(5));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 2.125, MARGIN); // window size drops to 1/2 of previous size
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments - 1); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, PipelineInterestsAimd::MIN_SSTHRESH,
                    MARGIN); // window size should decrease, as cwa is disabled
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packets
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[5], 0);
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(aimdPipeline->m_nCongMarks, 2);
}

BOOST_AUTO_TEST_CASE(IgnoreCongestionMarks)
{
  opt.ignoreCongMarks = true;
  createPipeline();

  nDataSegments = 7;
  aimdPipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(aimdPipeline->m_cwnd, 1, MARGIN);

  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);

  // receive segments 1 to 5
  for (uint64_t i = 1; i < 6; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(io, time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 4.5, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments - 1); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(aimdPipeline->m_cwnd, 4.75, MARGIN); // window size increases
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packet
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(aimdPipeline->m_nCongMarks, 1);
}

BOOST_AUTO_TEST_CASE(Nack)
{
  nDataSegments = 5;
  aimdPipeline->m_cwnd = 10.0;
  runWithData(*makeDataWithSegment(0));
  advanceClocks(io, time::nanoseconds(1));

  face.receive(*makeDataWithSegment(1));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 4);

  // receive a nack with NackReason::DUPLICATE for segment 2
  auto nack1 = makeNack(face.sentInterests[1], lp::NackReason::DUPLICATE);
  face.receive(nack1);
  advanceClocks(io, time::nanoseconds(1));

  // nack1 is ignored
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxQueue.size(), 0);

  // receive a nack with NackReason::CONGESTION for segment 3
  auto nack2 = makeNack(face.sentInterests[2], lp::NackReason::CONGESTION);
  face.receive(nack2);
  advanceClocks(io, time::nanoseconds(1));

  // segment 3 is retransmitted
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[3], 1);

  // receive a nack with NackReason::NONE for segment 4
  auto nack3 = makeNack(face.sentInterests[3], lp::NackReason::NONE);
  face.receive(nack3);
  advanceClocks(io, time::nanoseconds(1));

  // Other types of Nack will trigger a failure
  BOOST_CHECK_EQUAL(hasFailed, true);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
}

BOOST_AUTO_TEST_CASE(FinalBlockIdNotSetAtBeginning)
{
  nDataSegments = 4;
  aimdPipeline->m_cwnd = 4;
  runWithData(*makeDataWithSegment(0, false));
  advanceClocks(io, time::nanoseconds(1));

  // receive segment 1 without FinalBlockId
  face.receive(*makeDataWithSegment(1, false));
  advanceClocks(io, time::nanoseconds(1));

  // interests for segment 1 - 6 have been sent
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 6);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_CHECK_EQUAL(aimdPipeline->m_hasFinalBlockId, false);
  // pending interests: segment 2, 3, 4, 5, 6
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 5);

  // receive segment 2 with FinalBlockId
  face.receive(*makeDataWithSegment(2));
  advanceClocks(io, time::nanoseconds(1));
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 3);
  BOOST_CHECK_EQUAL(aimdPipeline->m_hasFinalBlockId, true);

  // pending interests for segment 2, 4, 5, 6 haven been removed
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 1);
}

BOOST_AUTO_TEST_CASE(FailureBeforeFinalBlockIdReceived)
{
  // failed to retrieve segNo while the FinalBlockId has not yet been
  // set, and later received a FinalBlockId >= segNo, i.e. segNo is
  // part of the content.

  nDataSegments = 4;
  aimdPipeline->m_cwnd = 4;
  runWithData(*makeDataWithSegment(0, false));
  advanceClocks(io, time::nanoseconds(1));

  // receive segment 1 without FinalBlockId
  face.receive(*makeDataWithSegment(1, false));
  advanceClocks(io, time::nanoseconds(1));
  // interests for segment 1 - 6 have been sent
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 6);

  // receive nack with NackReason::NONE for segment 3
  auto nack = makeNack(face.sentInterests[2], lp::NackReason::NONE);
  face.receive(nack);
  advanceClocks(io, time::nanoseconds(1));

  // error not triggered
  // pending interests for segment > 3 haven been removed
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 1);

  // receive segment 2 with FinalBlockId
  face.receive(*makeDataWithSegment(2));
  advanceClocks(io, time::nanoseconds(1));

  // error triggered since segment 3 is part of the content
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(SpuriousFailureBeforeFinalBlockIdReceived)
{
  // failed to retrieve segNo while the FinalBlockId has not yet been
  // set, and later received a FinalBlockId < segNo, i.e. segNo is
  // not part of the content, and it was actually a spurious failure

  nDataSegments = 4;
  aimdPipeline->m_cwnd = 4;
  runWithData(*makeDataWithSegment(0, false));
  advanceClocks(io, time::nanoseconds(1));

  // receive segment 1 without FinalBlockId
  face.receive(*makeDataWithSegment(1, false));
  advanceClocks(io, time::nanoseconds(1));
  // interests for segment 1 - 6 have been sent
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 6);

  // receive nack with NackReason::NONE for segment 4
  auto nack = makeNack(face.sentInterests[3], lp::NackReason::NONE);
  face.receive(nack);
  advanceClocks(io, time::nanoseconds(1));

  // error not triggered
  // pending interests for segment > 4 have been removed
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 2);

  // receive segment 2 with FinalBlockId
  face.receive(*makeDataWithSegment(2));
  advanceClocks(io, time::nanoseconds(1));

  // timeout segment 3
  advanceClocks(io, time::seconds(1));

  // segment 3 is retransmitted
  BOOST_CHECK_EQUAL(aimdPipeline->m_retxCount[3], 1);

  // receive segment 3
  face.receive(*makeDataWithSegment(3));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK_EQUAL(hasFailed, false);
}

BOOST_AUTO_TEST_CASE(PrintSummaryWithNoRttMeasurements)
{
  // test the console ouptut when no RTT measurement is available,
  // to make sure a proper message will be printed out

  std::stringstream ss;

  // change the underlying buffer and save the old buffer
  auto oldBuf = std::cerr.rdbuf(ss.rdbuf());

  aimdPipeline->printSummary();
  std::string line;

  bool found = false;
  while (std::getline(ss, line)) {
    if (line == "RTT stats unavailable") {
      found = true;
      break;
    }
  }
  BOOST_CHECK(found);
  std::cerr.rdbuf(oldBuf); // reset
}

BOOST_AUTO_TEST_SUITE_END() // TestPipelineInterestsAimd
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace aimd
} // namespace chunks
} // namespace ndn
