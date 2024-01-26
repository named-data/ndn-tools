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
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 * @author Klaus Schneider
 */

#include "tools/chunks/catchunks/pipeline-interests-aimd.hpp"

#include "pipeline-interests-fixture.hpp"

#include <cmath>

namespace ndn::chunks::tests {

using namespace ndn::tests;

class PipelineInterestAimdFixture : public PipelineInterestsFixture
{
protected:
  PipelineInterestAimdFixture()
  {
    opt.isQuiet = true;
    createPipeline();
  }

  void
  createPipeline()
  {
    auto pline = std::make_unique<PipelineInterestsAimd>(face, rttEstimator, opt);
    pipeline = pline.get();
    setPipeline(std::move(pline));
  }

private:
  static std::shared_ptr<RttEstimatorWithStats::Options>
  makeRttEstimatorOptions()
  {
    auto rttOptions = std::make_shared<RttEstimatorWithStats::Options>();
    rttOptions->alpha = 0.125;
    rttOptions->beta = 0.25;
    rttOptions->k = 4;
    rttOptions->initialRto = 1_s;
    rttOptions->minRto = 200_ms;
    rttOptions->maxRto = 4_s;
    rttOptions->rtoBackoffMultiplier = 2;
    return rttOptions;
  }

protected:
  Options opt;
  RttEstimatorWithStats rttEstimator{makeRttEstimatorOptions()};
  PipelineInterestsAdaptive* pipeline;
  static constexpr double MARGIN = 0.001;
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestPipelineInterestsAimd, PipelineInterestAimdFixture)

BOOST_AUTO_TEST_CASE(SlowStart)
{
  nDataSegments = 4;
  pipeline->m_ssthresh = 8.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  double preCwnd = pipeline->m_cwnd;
  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  for (uint64_t i = 0; i < nDataSegments - 1; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
    BOOST_CHECK_CLOSE(pipeline->m_cwnd - preCwnd, 1, MARGIN);
    preCwnd = pipeline->m_cwnd;
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments - 1);
}

BOOST_AUTO_TEST_CASE(CongestionAvoidance)
{
  nDataSegments = 7;
  pipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  double preCwnd = pipeline->m_cwnd;
  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  for (uint64_t i = 0; i < pipeline->m_ssthresh; ++i) { // slow start
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
    preCwnd = pipeline->m_cwnd;
  }

  BOOST_CHECK_CLOSE(preCwnd, 4.5, MARGIN);

  for (uint64_t i = pipeline->m_ssthresh; i < nDataSegments - 1; ++i) { // congestion avoidance
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
    BOOST_CHECK_CLOSE(pipeline->m_cwnd - preCwnd, opt.aiStep / std::floor(preCwnd), MARGIN);
    preCwnd = pipeline->m_cwnd;
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments -1);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  nDataSegments = 8;
  pipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  // receive segment 0, 1, and 2
  for (uint64_t i = 0; i < 3; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 3);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 4.25, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 7); // request for segment 7 has been sent

  advanceClocks(time::milliseconds(100));

  // receive segment 4
  face.receive(*makeDataWithSegment(4));
  advanceClocks(time::nanoseconds(1));

  // receive segment 5
  face.receive(*makeDataWithSegment(5));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 4.75, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments); // all the segment requests have been sent

  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, 0);
  BOOST_CHECK_EQUAL(pipeline->m_nLossDecr, 0);
  BOOST_CHECK_EQUAL(pipeline->m_nMarkDecr, 0);
  BOOST_CHECK_EQUAL(pipeline->m_nRetransmitted, 0);
  BOOST_CHECK_EQUAL(pipeline->m_nSkippedRetx, 0);
  BOOST_CHECK_EQUAL(pipeline->m_nCongMarks, 0);

  // timeout segment 3 & 6
  advanceClocks(time::milliseconds(150));
  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, 2);
  BOOST_CHECK_EQUAL(pipeline->m_nRetransmitted, 1);
  BOOST_CHECK_EQUAL(pipeline->m_nLossDecr, 1);
  BOOST_CHECK_EQUAL(pipeline->m_nSkippedRetx, 0);

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 2.375, MARGIN); // window size drop to 1/2 of previous size
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 1);

  // receive segment 6, retransmit 3
  face.receive(*makeDataWithSegment(6));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 2.875, MARGIN); // congestion avoidance
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 0);
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[3], 1);

  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, 2);
  BOOST_CHECK_EQUAL(pipeline->m_nRetransmitted, 2);
  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, pipeline->m_nRetransmitted + pipeline->m_nSkippedRetx);
}

BOOST_AUTO_TEST_CASE(CongestionMarksWithCwa)
{
  nDataSegments = 7;
  pipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  // receive segments 0 to 4
  for (uint64_t i = 0; i < 5; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 4.75, MARGIN);

  // receive segment 5 with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(5));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 2.375, MARGIN); // window size drops to 1/2 of previous size
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 2.375, MARGIN); // conservative window adaption (window size should not decrease)
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packets
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[5], 0);
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(pipeline->m_nCongMarks, 2);
}

BOOST_AUTO_TEST_CASE(CongestionMarksWithoutCwa)
{
  opt.disableCwa = true;
  createPipeline();

  nDataSegments = 7;
  pipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  // receive segments 0 to 4
  for (uint64_t i = 0; i < 5; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 5);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 4.75, MARGIN);

  // receive segment 5 with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(5));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 2.375, MARGIN); // window size drops to 1/2 of previous size
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, PipelineInterestsAdaptive::MIN_SSTHRESH,
                    MARGIN); // window size should decrease, as cwa is disabled
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packets
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[5], 0);
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(pipeline->m_nCongMarks, 2);
}

BOOST_AUTO_TEST_CASE(IgnoreCongestionMarks)
{
  opt.ignoreCongMarks = true;
  createPipeline();

  nDataSegments = 7;
  pipeline->m_ssthresh = 4.0;
  BOOST_REQUIRE_CLOSE(pipeline->m_cwnd, 2, MARGIN);

  run(name);
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 2);

  // receive segments 0 to 5
  for (uint64_t i = 0; i < 6; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::nanoseconds(1));
  }

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 6);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 5.0, MARGIN);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nDataSegments); // all interests have been sent

  // receive the last segment with congestion mark
  face.receive(*makeDataWithSegmentAndCongMark(nDataSegments - 1));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, nDataSegments);
  BOOST_CHECK_CLOSE(pipeline->m_cwnd, 5.2, MARGIN); // window size increases
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 0);

  // make sure no interest is retransmitted for marked data packet
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[nDataSegments - 1], 0);

  // check number of received marked data packets
  BOOST_CHECK_EQUAL(pipeline->m_nCongMarks, 1);
}

BOOST_AUTO_TEST_CASE(Nack)
{
  nDataSegments = 5;
  pipeline->m_cwnd = 10.0;
  run(name);
  advanceClocks(time::nanoseconds(1));

  face.receive(*makeDataWithSegment(0));
  advanceClocks(time::nanoseconds(1));

  face.receive(*makeDataWithSegment(1));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 10);

  // receive a nack with NackReason::DUPLICATE for segment 1
  auto nack1 = makeNack(face.sentInterests[1], lp::NackReason::DUPLICATE);
  face.receive(nack1);
  advanceClocks(time::nanoseconds(1));

  // nack1 is ignored
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_CHECK_EQUAL(pipeline->m_retxQueue.size(), 0);

  // receive a nack with NackReason::CONGESTION for segment 2
  auto nack2 = makeNack(face.sentInterests[2], lp::NackReason::CONGESTION);
  face.receive(nack2);
  advanceClocks(time::nanoseconds(1));

  // segment 2 is retransmitted
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[2], 1);

  // receive a nack with NackReason::NONE for segment 3
  auto nack3 = makeNack(face.sentInterests[3], lp::NackReason::NONE);
  face.receive(nack3);
  advanceClocks(time::nanoseconds(1));

  // Other types of Nack will trigger a failure
  BOOST_CHECK_EQUAL(hasFailed, true);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
}

BOOST_AUTO_TEST_CASE(FinalBlockIdNotSetAtBeginning)
{
  nDataSegments = 4;
  pipeline->m_cwnd = 4;
  run(name);
  advanceClocks(time::nanoseconds(1));

  // receive segment 0 without FinalBlockId
  face.receive(*makeDataWithSegment(0, false));
  advanceClocks(time::nanoseconds(1));

  // interests for segment 0 - 5 have been sent
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 6);
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 1);
  BOOST_CHECK_EQUAL(pipeline->m_hasFinalBlockId, false);
  // pending interests: segment 1, 2, 3, 4, 5
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 5);

  // receive segment 1 with FinalBlockId
  face.receive(*makeDataWithSegment(1));
  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(pipeline->m_nReceived, 2);
  BOOST_CHECK_EQUAL(pipeline->m_hasFinalBlockId, true);

  // pending interests for segment 1, 4, 5 haven been removed
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 2);
}

BOOST_AUTO_TEST_CASE(FailureBeforeFinalBlockIdReceived)
{
  // failed to retrieve segNo while the FinalBlockId has not yet been
  // set, and later received a FinalBlockId >= segNo, i.e. segNo is
  // part of the content.

  nDataSegments = 4;
  pipeline->m_cwnd = 4;
  run(name);
  advanceClocks(time::nanoseconds(1));

  // receive segment 0 without FinalBlockId
  face.receive(*makeDataWithSegment(0, false));
  advanceClocks(time::nanoseconds(1));

  // receive segment 1 without FinalBlockId
  face.receive(*makeDataWithSegment(1, false));
  advanceClocks(time::nanoseconds(1));

  // interests for segment 0 - 7 have been sent
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 8);

  // receive nack with NackReason::NONE for segment 3
  auto nack = makeNack(face.sentInterests[3], lp::NackReason::NONE);
  face.receive(nack);
  advanceClocks(time::nanoseconds(1));

  // error not triggered
  // pending interests for segment > 3 haven been removed
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 1);

  // receive segment 2 with FinalBlockId
  face.receive(*makeDataWithSegment(2));
  advanceClocks(time::nanoseconds(1));

  // error triggered since segment 3 is part of the content
  BOOST_CHECK_EQUAL(hasFailed, true);
}

BOOST_AUTO_TEST_CASE(SpuriousFailureBeforeFinalBlockIdReceived)
{
  // failed to retrieve segNo while the FinalBlockId has not yet been
  // set, and later received a FinalBlockId < segNo, i.e. segNo is
  // not part of the content, and it was actually a spurious failure

  nDataSegments = 4;
  pipeline->m_cwnd = 4;
  run(name);
  advanceClocks(time::nanoseconds(1));

  // receive segment 0 without FinalBlockId
  face.receive(*makeDataWithSegment(0, false));
  advanceClocks(time::nanoseconds(1));

  // receive segment 1 without FinalBlockId
  face.receive(*makeDataWithSegment(1, false));
  advanceClocks(time::nanoseconds(1));

  // interests for segment 0 - 7 have been sent
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 8);

  // receive nack with NackReason::NONE for segment 4
  auto nack = makeNack(face.sentInterests[4], lp::NackReason::NONE);
  face.receive(nack);
  advanceClocks(time::nanoseconds(1));

  // error not triggered
  // pending interests for segment > 3 have been removed
  BOOST_CHECK_EQUAL(hasFailed, false);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 2);

  // receive segment 2 with FinalBlockId
  face.receive(*makeDataWithSegment(2));
  advanceClocks(time::nanoseconds(1));

  // timeout segment 3
  advanceClocks(time::seconds(1));

  // segment 3 is retransmitted
  BOOST_CHECK_EQUAL(pipeline->m_retxCount[3], 1);

  // receive segment 3
  face.receive(*makeDataWithSegment(3));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(hasFailed, false);
}

BOOST_AUTO_TEST_CASE(SegmentInfoMaintenance)
{
  // test that m_segmentInfo is properly maintained when
  // a segment is received after two consecutive timeouts

  nDataSegments = 3;

  run(name);
  advanceClocks(time::nanoseconds(1));

  // receive segment 0
  face.receive(*makeDataWithSegment(0));
  advanceClocks(time::nanoseconds(1));

  // receive segment 1
  face.receive(*makeDataWithSegment(1));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 3);

  // check if segment 2's state is FirstTimeSent
  auto it = pipeline->m_segmentInfo.find(2);
  BOOST_REQUIRE(it != pipeline->m_segmentInfo.end());
  BOOST_CHECK(it->second.state == SegmentState::FirstTimeSent);

  // timeout segment 2 twice
  advanceClocks(time::milliseconds(400), 3);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 5);

  // check if segment 2's state is Retransmitted
  it = pipeline->m_segmentInfo.find(2);
  BOOST_REQUIRE(it != pipeline->m_segmentInfo.end());
  BOOST_CHECK(it->second.state == SegmentState::Retransmitted);

  // check if segment 2 was retransmitted twice
  BOOST_CHECK_EQUAL(pipeline->m_retxCount.at(2), 2);

  // receive segment 2 the first time
  face.receive(*makeDataWithSegment(2));
  advanceClocks(time::nanoseconds(1));

  // check if segment 2 was erased from m_segmentInfo
  it = pipeline->m_segmentInfo.find(2);
  BOOST_CHECK(it == pipeline->m_segmentInfo.end());

  auto prevRtt = rttEstimator.getAvgRtt();
  auto prevRto = rttEstimator.getEstimatedRto();

  // receive segment 2 the second time
  face.receive(*makeDataWithSegment(2));
  advanceClocks(time::nanoseconds(1));

  // nothing changed
  it = pipeline->m_segmentInfo.find(2);
  BOOST_CHECK(it == pipeline->m_segmentInfo.end());
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 5);
  BOOST_CHECK_EQUAL(rttEstimator.getAvgRtt(), prevRtt);
  BOOST_CHECK_EQUAL(rttEstimator.getEstimatedRto(), prevRto);
}

BOOST_AUTO_TEST_CASE(Bug5202)
{
  // If an interest is pending during a window decrease, it should not trigger
  // another window decrease when it times out.
  // This test emulates a network where RTT = 20ms and transmission time = 1ms.

  // adding small sample to RTT estimator. This should set rto = 200ms
  rttEstimator.addMeasurement(time::nanoseconds(1));
  BOOST_REQUIRE_EQUAL(rttEstimator.getEstimatedRto(), time::milliseconds(200));

  nDataSegments = 300;
  pipeline->m_ssthresh = 0;
  pipeline->m_cwnd = 20;

  run(name);

  advanceClocks(time::nanoseconds(1));
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 20);
  advanceClocks(time::milliseconds(20));

  // Segment 1 is lost. Receive segment 0 and 2-99
  face.receive(*makeDataWithSegment(0));
  advanceClocks(time::milliseconds(1));
  for (uint64_t i = 2; i <= 99; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::milliseconds(1));
  }

  // Segment 100 is lost. Receive segment 100 to 181
  for (uint64_t i = 101; i <= 181; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::milliseconds(1));
  }

  // 200ms passed after sending segment 1, check for timeout
  BOOST_CHECK_GT(pipeline->m_cwnd, 27./2);
  BOOST_CHECK_LT(pipeline->m_cwnd, 28./2);
  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, 1);
  BOOST_CHECK_EQUAL(pipeline->m_nLossDecr, 1);

  // Receive segment 182 to 300
  for (uint64_t i = 182; i <= 300; ++i) {
    face.receive(*makeDataWithSegment(i));
    advanceClocks(time::milliseconds(1));
  }

  // The second packet should timeout now
  BOOST_CHECK_EQUAL(pipeline->m_nTimeouts, 2);
  // This timeout should NOT trigger another window decrease
  BOOST_CHECK_EQUAL(pipeline->m_nLossDecr, 1);
}

BOOST_AUTO_TEST_CASE(PrintSummaryWithNoRttMeasurements)
{
  // test the console ouptut when no RTT measurement is available,
  // to make sure a proper message will be printed out

  std::stringstream ss;

  // change the underlying buffer and save the old buffer
  auto oldBuf = std::cerr.rdbuf(ss.rdbuf());

  pipeline->printSummary();
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

BOOST_AUTO_TEST_CASE(StopsWhenFileSizeLessThanChunkSize)
{
  // test to see if the program doesn't hang,
  // when transfer is complete, for files less than the chunk size
  // (i.e. when only one segment is sent/received)

  createPipeline();
  nDataSegments = 1;

  run(name);
  advanceClocks(time::nanoseconds(1));

  face.receive(*makeDataWithSegment(0));
  advanceClocks(time::nanoseconds(1));

  BOOST_CHECK_EQUAL(pipeline->m_hasFinalBlockId, true);
  BOOST_CHECK_EQUAL(pipeline->m_segmentInfo.size(), 0);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestPipelineInterestsAimd
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace ndn::chunks::tests
