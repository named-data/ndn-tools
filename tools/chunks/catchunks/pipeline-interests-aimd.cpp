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
 * @author Shuo Yang
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 */

#include "pipeline-interests-aimd.hpp"
#include "data-fetcher.hpp"

#include <cmath>
#include <iomanip>

namespace ndn {
namespace chunks {
namespace aimd {

constexpr double PipelineInterestsAimd::MIN_SSTHRESH;

PipelineInterestsAimd::PipelineInterestsAimd(Face& face, RttEstimator& rttEstimator,
                                             const Options& options)
  : PipelineInterests(face)
  , m_options(options)
  , m_rttEstimator(rttEstimator)
  , m_scheduler(m_face.getIoService())
  , m_checkRtoEvent(m_scheduler)
  , m_highData(0)
  , m_highInterest(0)
  , m_recPoint(0)
  , m_nInFlight(0)
  , m_nLossEvents(0)
  , m_nRetransmitted(0)
  , m_nCongMarks(0)
  , m_nSent(0)
  , m_cwnd(m_options.initCwnd)
  , m_ssthresh(m_options.initSsthresh)
  , m_hasFailure(false)
  , m_failedSegNo(0)
{
  if (m_options.isVerbose) {
    std::cerr << m_options;
  }
}

PipelineInterestsAimd::~PipelineInterestsAimd()
{
  cancel();
}

void
PipelineInterestsAimd::doRun()
{
  // schedule the event to check retransmission timer
  m_checkRtoEvent = m_scheduler.scheduleEvent(m_options.rtoCheckInterval, [this] { checkRto(); });

  schedulePackets();
}

void
PipelineInterestsAimd::doCancel()
{
  for (const auto& entry : m_segmentInfo) {
    m_face.removePendingInterest(entry.second.interestId);
  }
  m_checkRtoEvent.cancel();
  m_segmentInfo.clear();
}

void
PipelineInterestsAimd::checkRto()
{
  if (isStopping())
    return;

  bool hasTimeout = false;

  for (auto& entry : m_segmentInfo) {
    SegmentInfo& segInfo = entry.second;
    if (segInfo.state != SegmentState::InRetxQueue && // do not check segments currently in the retx queue
        segInfo.state != SegmentState::RetxReceived) { // or already-received retransmitted segments
      Milliseconds timeElapsed = time::steady_clock::now() - segInfo.timeSent;
      if (timeElapsed.count() > segInfo.rto.count()) { // timer expired?
        hasTimeout = true;
        enqueueForRetransmission(entry.first);
      }
    }
  }

  if (hasTimeout) {
    recordTimeout();
    schedulePackets();
  }

  // schedule the next check after predefined interval
  m_checkRtoEvent = m_scheduler.scheduleEvent(m_options.rtoCheckInterval, [this] { checkRto(); });
}

void
PipelineInterestsAimd::sendInterest(uint64_t segNo, bool isRetransmission)
{
  if (isStopping())
    return;

  if (m_hasFinalBlockId && segNo > m_lastSegmentNo && !isRetransmission)
    return;

  if (!isRetransmission && m_hasFailure)
    return;

  if (m_options.isVerbose) {
    std::cerr << (isRetransmission ? "Retransmitting" : "Requesting")
              << " segment #" << segNo << std::endl;
  }

  if (isRetransmission) {
    // keep track of retx count for this segment
    auto ret = m_retxCount.emplace(segNo, 1);
    if (ret.second == false) { // not the first retransmission
      m_retxCount[segNo] += 1;
      if (m_options.maxRetriesOnTimeoutOrNack != DataFetcher::MAX_RETRIES_INFINITE &&
          m_retxCount[segNo] > m_options.maxRetriesOnTimeoutOrNack) {
        return handleFail(segNo, "Reached the maximum number of retries (" +
                          to_string(m_options.maxRetriesOnTimeoutOrNack) +
                          ") while retrieving segment #" + to_string(segNo));
      }

      if (m_options.isVerbose) {
        std::cerr << "# of retries for segment #" << segNo
                  << " is " << m_retxCount[segNo] << std::endl;
      }
    }

    m_face.removePendingInterest(m_segmentInfo[segNo].interestId);
  }

  Interest interest(Name(m_prefix).appendSegment(segNo));
  interest.setInterestLifetime(m_options.interestLifetime);
  interest.setMustBeFresh(m_options.mustBeFresh);
  interest.setMaxSuffixComponents(1);

  auto interestId = m_face.expressInterest(interest,
                                           bind(&PipelineInterestsAimd::handleData, this, _1, _2),
                                           bind(&PipelineInterestsAimd::handleNack, this, _1, _2),
                                           bind(&PipelineInterestsAimd::handleLifetimeExpiration, this, _1));
  m_nInFlight++;
  m_nSent++;

  if (isRetransmission) {
    SegmentInfo& segInfo = m_segmentInfo[segNo];
    segInfo.timeSent = time::steady_clock::now();
    segInfo.rto = m_rttEstimator.getEstimatedRto();
    segInfo.state = SegmentState::Retransmitted;
    m_nRetransmitted++;
  }
  else {
    m_highInterest = segNo;
    m_segmentInfo[segNo] = {interestId,
                            time::steady_clock::now(),
                            m_rttEstimator.getEstimatedRto(),
                            SegmentState::FirstTimeSent};
  }
}

void
PipelineInterestsAimd::schedulePackets()
{
  BOOST_ASSERT(m_nInFlight >= 0);
  auto availableWindowSize = static_cast<int64_t>(m_cwnd) - m_nInFlight;

  while (availableWindowSize > 0) {
    if (!m_retxQueue.empty()) { // do retransmission first
      uint64_t retxSegNo = m_retxQueue.front();
      m_retxQueue.pop();

      auto it = m_segmentInfo.find(retxSegNo);
      if (it == m_segmentInfo.end()) {
        continue;
      }
      // the segment is still in the map, it means that it needs to be retransmitted
      sendInterest(retxSegNo, true);
    }
    else { // send next segment
      sendInterest(getNextSegmentNo(), false);
    }
    availableWindowSize--;
  }
}

void
PipelineInterestsAimd::handleData(const Interest& interest, const Data& data)
{
  if (isStopping())
    return;

  // Data name will not have extra components because MaxSuffixComponents is set to 1
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
  SegmentInfo& segInfo = m_segmentInfo[recvSegNo];
  if (segInfo.state == SegmentState::RetxReceived) {
    m_segmentInfo.erase(recvSegNo);
    return; // ignore already-received segment
  }

  Milliseconds rtt = time::steady_clock::now() - segInfo.timeSent;
  if (m_options.isVerbose) {
    std::cerr << "Received segment #" << recvSegNo
              << ", rtt=" << rtt.count() << "ms"
              << ", rto=" << segInfo.rto.count() << "ms" << std::endl;
  }

  if (m_highData < recvSegNo) {
    m_highData = recvSegNo;
  }

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
        decreaseWindow();

        if (m_options.isVerbose) {
          std::cerr << "Received congestion mark, value = " << data.getCongestionMark()
                    << ", new cwnd = " << m_cwnd << std::endl;
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

  if (segInfo.state == SegmentState::FirstTimeSent ||
      segInfo.state == SegmentState::InRetxQueue) { // do not sample RTT for retransmitted segments
    auto nExpectedSamples = std::max<int64_t>((m_nInFlight + 1) >> 1, 1);
    BOOST_ASSERT(nExpectedSamples > 0);
    m_rttEstimator.addMeasurement(recvSegNo, rtt, static_cast<size_t>(nExpectedSamples));
    m_segmentInfo.erase(recvSegNo); // remove the entry associated with the received segment
  }
  else { // retransmission
    BOOST_ASSERT(segInfo.state == SegmentState::Retransmitted);
    segInfo.state = SegmentState::RetxReceived;
  }

  BOOST_ASSERT(m_nReceived > 0);
  if (m_hasFinalBlockId &&
      static_cast<uint64_t>(m_nReceived - 1) >= m_lastSegmentNo) { // all segments have been received
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
PipelineInterestsAimd::handleNack(const Interest& interest, const lp::Nack& nack)
{
  if (isStopping())
    return;

  if (m_options.isVerbose)
    std::cerr << "Received Nack with reason " << nack.getReason()
              << " for Interest " << interest << std::endl;

  uint64_t segNo = getSegmentFromPacket(interest);

  switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE:
      // ignore duplicates
      break;
    case lp::NackReason::CONGESTION:
      // treated the same as timeout for now
      enqueueForRetransmission(segNo);
      recordTimeout();
      schedulePackets();
      break;
    default:
      handleFail(segNo, "Could not retrieve data for " + interest.getName().toUri() +
                 ", reason: " + boost::lexical_cast<std::string>(nack.getReason()));
      break;
  }
}

void
PipelineInterestsAimd::handleLifetimeExpiration(const Interest& interest)
{
  if (isStopping())
    return;

  enqueueForRetransmission(getSegmentFromPacket(interest));
  recordTimeout();
  schedulePackets();
}

void
PipelineInterestsAimd::recordTimeout()
{
  if (m_options.disableCwa || m_highData > m_recPoint) {
    // react to only one timeout per RTT (conservative window adaptation)
    m_recPoint = m_highInterest;

    decreaseWindow();
    m_rttEstimator.backoffRto();
    m_nLossEvents++;

    if (m_options.isVerbose) {
      std::cerr << "Packet loss event, new cwnd = " << m_cwnd
                << ", ssthresh = " << m_ssthresh << std::endl;
    }
  }
}

void
PipelineInterestsAimd::enqueueForRetransmission(uint64_t segNo)
{
  BOOST_ASSERT(m_nInFlight > 0);
  m_nInFlight--;
  m_retxQueue.push(segNo);
  m_segmentInfo.at(segNo).state = SegmentState::InRetxQueue;
}

void
PipelineInterestsAimd::handleFail(uint64_t segNo, const std::string& reason)
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
PipelineInterestsAimd::increaseWindow()
{
  if (m_cwnd < m_ssthresh) {
    m_cwnd += m_options.aiStep; // additive increase
  }
  else {
    m_cwnd += m_options.aiStep / std::floor(m_cwnd); // congestion avoidance
  }

  afterCwndChange(time::steady_clock::now() - getStartTime(), m_cwnd);
}

void
PipelineInterestsAimd::decreaseWindow()
{
  // please refer to RFC 5681, Section 3.1 for the rationale behind it
  m_ssthresh = std::max(MIN_SSTHRESH, m_cwnd * m_options.mdCoef); // multiplicative decrease
  m_cwnd = m_options.resetCwndToInit ? m_options.initCwnd : m_ssthresh;

  afterCwndChange(time::steady_clock::now() - getStartTime(), m_cwnd);
}

void
PipelineInterestsAimd::cancelInFlightSegmentsGreaterThan(uint64_t segNo)
{
  for (auto it = m_segmentInfo.begin(); it != m_segmentInfo.end();) {
    // cancel fetching all segments that follow
    if (it->first > segNo) {
      m_face.removePendingInterest(it->second.interestId);
      it = m_segmentInfo.erase(it);
      m_nInFlight--;
    }
    else {
      ++it;
    }
  }
}

void
PipelineInterestsAimd::printSummary() const
{
  PipelineInterests::printSummary();
  std::cerr << "Total # of lost/retransmitted segments: " << m_nRetransmitted
            << " (caused " << m_nLossEvents << " window decreases)\n"
            << "Packet loss rate: "
            << (static_cast<double>(m_nRetransmitted) / static_cast<double>(m_nSent)) * 100 << "%\n"
            << "Total # of received congestion marks: " << m_nCongMarks << "\n"
            << "RTT ";

  if (m_rttEstimator.getMinRtt() == std::numeric_limits<double>::max() ||
      m_rttEstimator.getMaxRtt() == std::numeric_limits<double>::min()) {
     std::cerr << "stats unavailable\n";
   }
   else {
     std::cerr << "min/avg/max = " << std::fixed << std::setprecision(3)
                                   << m_rttEstimator.getMinRtt() << "/"
                                   << m_rttEstimator.getAvgRtt() << "/"
                                   << m_rttEstimator.getMaxRtt() << " ms\n";
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
  case SegmentState::RetxReceived:
    os << "RetxReceived";
    break;
  }
  return os;
}

std::ostream&
operator<<(std::ostream& os, const PipelineInterestsAimdOptions& options)
{
  os << "AIMD pipeline parameters:\n"
     << "\tInitial congestion window size = " << options.initCwnd << "\n"
     << "\tInitial slow start threshold = " << options.initSsthresh << "\n"
     << "\tAdditive increase step = " << options.aiStep << "\n"
     << "\tMultiplicative decrease factor = " << options.mdCoef << "\n"
     << "\tRTO check interval = " << options.rtoCheckInterval << "\n"
     << "\tMax retries on timeout or Nack = " << (options.maxRetriesOnTimeoutOrNack == DataFetcher::MAX_RETRIES_INFINITE ?
                                                    "infinite" : to_string(options.maxRetriesOnTimeoutOrNack)) << "\n"
     << "\tReaction to congestion marks " << (options.ignoreCongMarks ? "disabled" : "enabled") << "\n"
     << "\tConservative window adaptation " << (options.disableCwa ? "disabled" : "enabled") << "\n"
     << "\tResetting cwnd to " << (options.resetCwndToInit ? "initCwnd" : "ssthresh") << " upon loss event\n";
  return os;
}

} // namespace aimd
} // namespace chunks
} // namespace ndn
