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
 * @author Shuo Yang
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 * @author Klaus Schneider
 */

#include "pipeline-interests-adaptive.hpp"
#include "data-fetcher.hpp"

#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <iostream>

namespace ndn::chunks {

PipelineInterestsAdaptive::PipelineInterestsAdaptive(Face& face,
                                                     RttEstimatorWithStats& rttEstimator,
                                                     const Options& opts)
  : PipelineInterests(face, opts)
  , m_cwnd(m_options.initCwnd)
  , m_ssthresh(m_options.initSsthresh)
  , m_rttEstimator(rttEstimator)
  , m_scheduler(m_face.getIoContext())
{
}

PipelineInterestsAdaptive::~PipelineInterestsAdaptive()
{
  cancel();
}

void
PipelineInterestsAdaptive::doRun()
{
  if (allSegmentsReceived()) {
    cancel();
    if (!m_options.isQuiet) {
      printSummary();
    }
    return;
  }

  // schedule the event to check retransmission timer
  m_checkRtoEvent = m_scheduler.schedule(m_options.rtoCheckInterval, [this] { checkRto(); });

  schedulePackets();
}

void
PipelineInterestsAdaptive::doCancel()
{
  m_checkRtoEvent.cancel();
  m_segmentInfo.clear();
}

void
PipelineInterestsAdaptive::checkRto()
{
  if (isStopping())
    return;

  bool hasTimeout = false;
  uint64_t highTimeoutSeg = 0;

  for (auto& entry : m_segmentInfo) {
    SegmentInfo& segInfo = entry.second;
    if (segInfo.state != SegmentState::InRetxQueue) { // skip segments already in the retx queue
      auto timeElapsed = time::steady_clock::now() - segInfo.timeSent;
      if (timeElapsed > segInfo.rto) { // timer expired?
        m_nTimeouts++;
        hasTimeout = true;
        highTimeoutSeg = std::max(highTimeoutSeg, entry.first);
        enqueueForRetransmission(entry.first);
      }
    }
  }

  if (hasTimeout) {
    recordTimeout(highTimeoutSeg);
    schedulePackets();
  }

  // schedule the next check after predefined interval
  m_checkRtoEvent = m_scheduler.schedule(m_options.rtoCheckInterval, [this] { checkRto(); });
}

void
PipelineInterestsAdaptive::sendInterest(uint64_t segNo, bool isRetransmission)
{
  if (isStopping())
    return;

  if (m_hasFinalBlockId && segNo > m_lastSegmentNo)
    return;

  if (!isRetransmission && m_hasFailure)
    return;

  if (m_options.isVerbose) {
    std::cerr << (isRetransmission ? "Retransmitting" : "Requesting")
              << " segment #" << segNo << "\n";
  }

  if (isRetransmission) {
    // keep track of retx count for this segment
    auto ret = m_retxCount.emplace(segNo, 1);
    if (!ret.second) { // not the first retransmission
      m_retxCount[segNo] += 1;
      if (m_options.maxRetriesOnTimeoutOrNack != DataFetcher::MAX_RETRIES_INFINITE &&
          m_retxCount[segNo] > m_options.maxRetriesOnTimeoutOrNack) {
        return handleFail(segNo, "Reached the maximum number of retries (" +
                          std::to_string(m_options.maxRetriesOnTimeoutOrNack) +
                          ") while retrieving segment #" + std::to_string(segNo));
      }

      if (m_options.isVerbose) {
        std::cerr << "# of retries for segment #" << segNo
                  << " is " << m_retxCount[segNo] << "\n";
      }
    }
  }

  auto interest = Interest()
                  .setName(Name(m_prefix).appendSegment(segNo))
                  .setMustBeFresh(m_options.mustBeFresh)
                  .setInterestLifetime(m_options.interestLifetime);

  SegmentInfo& segInfo = m_segmentInfo[segNo];
  segInfo.interestHdl = m_face.expressInterest(interest,
                                               FORWARD_TO_MEM_FN(handleData),
                                               FORWARD_TO_MEM_FN(handleNack),
                                               FORWARD_TO_MEM_FN(handleLifetimeExpiration));
  segInfo.timeSent = time::steady_clock::now();
  segInfo.rto = m_rttEstimator.getEstimatedRto();

  m_nInFlight++;
  m_nSent++;

  if (isRetransmission) {
    segInfo.state = SegmentState::Retransmitted;
    m_nRetransmitted++;
  }
  else {
    m_highInterest = segNo;
    segInfo.state = SegmentState::FirstTimeSent;
  }
}

void
PipelineInterestsAdaptive::schedulePackets()
{
  BOOST_ASSERT(m_nInFlight >= 0);
  auto availableWindowSize = static_cast<int64_t>(m_cwnd) - m_nInFlight;

  while (availableWindowSize > 0) {
    if (!m_retxQueue.empty()) { // do retransmission first
      uint64_t retxSegNo = m_retxQueue.front();
      m_retxQueue.pop();
      if (m_segmentInfo.count(retxSegNo) == 0) {
        m_nSkippedRetx++;
        continue;
      }
      // the segment is still in the map, that means it needs to be retransmitted
      sendInterest(retxSegNo, true);
    }
    else { // send next segment
      sendInterest(getNextSegmentNo(), false);
    }
    availableWindowSize--;
  }
}

void
PipelineInterestsAdaptive::handleData(const Interest& interest, const Data& data)
{
  if (isStopping())
    return;

  // Interest was expressed with CanBePrefix=false
  BOOST_ASSERT(data.getName().equals(interest.getName()));

  if (!m_hasFinalBlockId && data.getFinalBlock()) {
    m_lastSegmentNo = data.getFinalBlock()->toSegment();
    m_hasFinalBlockId = true;
    cancelInFlightSegmentsGreaterThan(m_lastSegmentNo);
    if (m_hasFailure && m_lastSegmentNo >= m_failedSegNo) {
      // previously failed segment is part of the content
      return onFailure(m_failureReason);
    }
    else {
      m_hasFailure = false;
    }
  }

  uint64_t recvSegNo = getSegmentFromPacket(data);
  auto segIt = m_segmentInfo.find(recvSegNo);
  if (segIt == m_segmentInfo.end()) {
    return; // ignore already-received segment
  }

  SegmentInfo& segInfo = segIt->second;
  time::nanoseconds rtt = time::steady_clock::now() - segInfo.timeSent;
  if (m_options.isVerbose) {
    std::cerr << "Received segment #" << recvSegNo
              << ", rtt=" << rtt.count() / 1e6 << "ms"
              << ", rto=" << segInfo.rto.count() / 1e6 << "ms\n";
  }

  m_highData = std::max(m_highData, recvSegNo);

  // for segments in retx queue, we must not decrement m_nInFlight
  // because it was already decremented when the segment timed out
  if (segInfo.state != SegmentState::InRetxQueue) {
    m_nInFlight--;
  }

  // upon finding congestion mark, decrease the window size
  // without retransmitting any packet
  if (data.getCongestionMark() > 0) {
    m_nCongMarks++;
    if (!m_options.ignoreCongMarks) {
      if (m_options.disableCwa || m_highData > m_recPoint) {
        m_recPoint = m_highInterest;  // react to only one congestion event (timeout or congestion mark)
                                      // per RTT (conservative window adaptation)
        m_nMarkDecr++;
        decreaseWindow();

        if (m_options.isVerbose) {
          std::cerr << "Received congestion mark, value = " << data.getCongestionMark()
                    << ", new cwnd = " << m_cwnd << "\n";
        }
      }
    }
    else {
      increaseWindow();
    }
  }
  else {
    increaseWindow();
  }

  onData(data);

  // do not sample RTT for retransmitted segments
  if ((segInfo.state == SegmentState::FirstTimeSent ||
       segInfo.state == SegmentState::InRetxQueue) &&
      m_retxCount.count(recvSegNo) == 0) {
    auto nExpectedSamples = std::max<int64_t>((m_nInFlight + 1) >> 1, 1);
    BOOST_ASSERT(nExpectedSamples > 0);
    m_rttEstimator.addMeasurement(rtt, static_cast<size_t>(nExpectedSamples));
    afterRttMeasurement({recvSegNo, rtt,
                         m_rttEstimator.getSmoothedRtt(),
                         m_rttEstimator.getRttVariation(),
                         m_rttEstimator.getEstimatedRto()});
  }

  // remove the entry associated with the received segment
  m_segmentInfo.erase(segIt);

  if (allSegmentsReceived()) {
    cancel();
    if (!m_options.isQuiet) {
      printSummary();
    }
  }
  else {
    schedulePackets();
  }
}

void
PipelineInterestsAdaptive::handleNack(const Interest& interest, const lp::Nack& nack)
{
  if (isStopping())
    return;

  if (m_options.isVerbose)
    std::cerr << "Received Nack with reason " << nack.getReason()
              << " for Interest " << interest << "\n";

  uint64_t segNo = getSegmentFromPacket(interest);

  switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE:
      // ignore duplicates
      break;
    case lp::NackReason::CONGESTION:
      // treated the same as timeout for now
      enqueueForRetransmission(segNo);
      recordTimeout(segNo);
      schedulePackets();
      break;
    default:
      handleFail(segNo, "Could not retrieve data for " + interest.getName().toUri() +
                 ", reason: " + boost::lexical_cast<std::string>(nack.getReason()));
      break;
  }
}

void
PipelineInterestsAdaptive::handleLifetimeExpiration(const Interest& interest)
{
  if (isStopping())
    return;

  m_nTimeouts++;

  uint64_t segNo = getSegmentFromPacket(interest);
  enqueueForRetransmission(segNo);
  recordTimeout(segNo);
  schedulePackets();
}

void
PipelineInterestsAdaptive::recordTimeout(uint64_t segNo)
{
  if (m_options.disableCwa || segNo > m_recPoint) {
    // interests that are still outstanding during a timeout event
    // should not trigger another window decrease later (bug #5202)
    m_recPoint = m_highInterest;

    decreaseWindow();
    m_rttEstimator.backoffRto();
    m_nLossDecr++;

    if (m_options.isVerbose) {
      std::cerr << "Packet loss event, new cwnd = " << m_cwnd
                << ", ssthresh = " << m_ssthresh << "\n";
    }
  }
}

void
PipelineInterestsAdaptive::enqueueForRetransmission(uint64_t segNo)
{
  BOOST_ASSERT(m_nInFlight > 0);
  m_nInFlight--;
  m_retxQueue.push(segNo);
  m_segmentInfo.at(segNo).state = SegmentState::InRetxQueue;
}

void
PipelineInterestsAdaptive::handleFail(uint64_t segNo, const std::string& reason)
{
  if (isStopping())
    return;

  // if the failed segment is definitely part of the content, raise a fatal error
  if (m_hasFinalBlockId && segNo <= m_lastSegmentNo)
    return onFailure(reason);

  if (!m_hasFinalBlockId) {
    m_segmentInfo.erase(segNo);
    m_nInFlight--;

    if (m_segmentInfo.empty()) {
      onFailure("Fetching terminated but no final segment number has been found");
    }
    else {
      cancelInFlightSegmentsGreaterThan(segNo);
      m_hasFailure = true;
      m_failedSegNo = segNo;
      m_failureReason = reason;
    }
  }
}

void
PipelineInterestsAdaptive::cancelInFlightSegmentsGreaterThan(uint64_t segNo)
{
  for (auto it = m_segmentInfo.begin(); it != m_segmentInfo.end();) {
    // cancel fetching all segments that follow
    if (it->first > segNo) {
      it = m_segmentInfo.erase(it);
      m_nInFlight--;
    }
    else {
      ++it;
    }
  }
}

void
PipelineInterestsAdaptive::printOptions() const
{
  PipelineInterests::printOptions();
  std::cerr
      << "\tInitial congestion window size = " << m_options.initCwnd << "\n"
      << "\tInitial slow start threshold = " << m_options.initSsthresh << "\n"
      << "\tAdditive increase step = " << m_options.aiStep << "\n"
      << "\tMultiplicative decrease factor = " << m_options.mdCoef << "\n"
      << "\tRTO check interval = " << m_options.rtoCheckInterval << "\n"
      << "\tReact to congestion marks = " << (m_options.ignoreCongMarks ? "no" : "yes") << "\n"
      << "\tConservative window adaptation = " << (m_options.disableCwa ? "no" : "yes") << "\n"
      << "\tResetting window to " << (m_options.resetCwndToInit ?
                                        "initial value" : "ssthresh") << " upon loss event\n";
}

void
PipelineInterestsAdaptive::printSummary() const
{
  PipelineInterests::printSummary();
  std::cerr << "Congestion marks: " << m_nCongMarks << " (caused " << m_nMarkDecr << " window decreases)\n"
            << "Timeouts: " << m_nTimeouts << " (caused " << m_nLossDecr << " window decreases)\n"
            << "Retransmitted segments: " << m_nRetransmitted
            << " (" << (m_nSent == 0 ? 0 : (m_nRetransmitted * 100.0 / m_nSent)) << "%)"
            << ", skipped: " << m_nSkippedRetx << "\n"
            << "RTT ";

  if (m_rttEstimator.getMinRtt() == time::nanoseconds::max() ||
      m_rttEstimator.getMaxRtt() == time::nanoseconds::min()) {
    std::cerr << "stats unavailable\n";
  }
  else {
    std::cerr << "min/avg/max = " << std::fixed << std::setprecision(3)
              << m_rttEstimator.getMinRtt().count() / 1e6 << "/"
              << m_rttEstimator.getAvgRtt().count() / 1e6 << "/"
              << m_rttEstimator.getMaxRtt().count() / 1e6 << " ms\n";
  }
}

std::ostream&
operator<<(std::ostream& os, SegmentState state)
{
  switch (state) {
  case SegmentState::FirstTimeSent:
    os << "FirstTimeSent";
    break;
  case SegmentState::InRetxQueue:
    os << "InRetxQueue";
    break;
  case SegmentState::Retransmitted:
    os << "Retransmitted";
    break;
  }
  return os;
}

} // namespace ndn::chunks
