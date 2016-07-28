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
 * @author Shuo Yang
 * @author Weiwei Liu
 */

#include "pipeline-interests-aimd.hpp"

#include <cmath>

namespace ndn {
namespace chunks {
namespace aimd {

PipelineInterestsAimd::PipelineInterestsAimd(Face& face, RttEstimator& rttEstimator,
                                             const Options& options)
  : PipelineInterests(face)
  , m_options(options)
  , m_rttEstimator(rttEstimator)
  , m_scheduler(m_face.getIoService())
  , m_nextSegmentNo(0)
  , m_receivedSize(0)
  , m_highData(0)
  , m_highInterest(0)
  , m_recPoint(0)
  , m_nInFlight(0)
  , m_nReceived(0)
  , m_nLossEvents(0)
  , m_nRetransmitted(0)
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
  // record the start time of running pipeline
  m_startTime = time::steady_clock::now();

  // count the excluded segment
  m_nReceived++;

  // schedule the event to check retransmission timer
  m_scheduler.scheduleEvent(m_options.rtoCheckInterval, [this] { checkRto(); });

  sendInterest(getNextSegmentNo(), false);
}

void
PipelineInterestsAimd::doCancel()
{
  for (const auto& entry : m_segmentInfo) {
    const SegmentInfo& segInfo = entry.second;
    m_face.removePendingInterest(segInfo.interestId);
  }
  m_segmentInfo.clear();
  m_scheduler.cancelAllEvents();
}

void
PipelineInterestsAimd::checkRto()
{
  if (isStopping())
    return;

  int timeoutCount = 0;

  for (auto& entry : m_segmentInfo) {
    SegmentInfo& segInfo = entry.second;
    if (segInfo.state != SegmentState::InRetxQueue && // do not check segments currently in the retx queue
        segInfo.state != SegmentState::RetxReceived) { // or already-received retransmitted segments
      Milliseconds timeElapsed = time::steady_clock::now() - segInfo.timeSent;
      if (timeElapsed.count() > segInfo.rto.count()) { // timer expired?
        uint64_t timedoutSeg = entry.first;
        m_retxQueue.push(timedoutSeg); // put on retx queue
        segInfo.state = SegmentState::InRetxQueue; // update status
        timeoutCount++;
      }
    }
  }

  if (timeoutCount > 0) {
    handleTimeout(timeoutCount);
  }

  // schedule the next check after predefined interval
  m_scheduler.scheduleEvent(m_options.rtoCheckInterval, [this] { checkRto(); });
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
    if (isRetransmission)
      std::cerr << "Retransmitting segment #" << segNo << std::endl;
    else
      std::cerr << "Requesting segment #" << segNo << std::endl;
  }

  if (isRetransmission) {
    auto ret = m_retxCount.insert(std::make_pair(segNo, 1));
    if (ret.second == false) { // not the first retransmission
      m_retxCount[segNo] += 1;
      if (m_retxCount[segNo] > m_options.maxRetriesOnTimeoutOrNack) {
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
                                           bind(&PipelineInterestsAimd::handleLifetimeExpiration,
                                                this, _1));

  m_nInFlight++;

  if (isRetransmission) {
    SegmentInfo& segInfo = m_segmentInfo[segNo];
    segInfo.state = SegmentState::Retransmitted;
    segInfo.rto = m_rttEstimator.getEstimatedRto();
    segInfo.timeSent = time::steady_clock::now();
    m_nRetransmitted++;
  }
  else {
    m_highInterest = segNo;
    Milliseconds rto = m_rttEstimator.getEstimatedRto();
    SegmentInfo segInfo{interestId, SegmentState::FirstTimeSent, rto, time::steady_clock::now()};

    m_segmentInfo.emplace(segNo, segInfo);
  }
}

void
PipelineInterestsAimd::schedulePackets()
{
  int availableWindowSize = static_cast<int>(m_cwnd) - m_nInFlight;
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

  if (!m_hasFinalBlockId && !data.getFinalBlockId().empty()) {
    m_lastSegmentNo = data.getFinalBlockId().toSegment();
    m_hasFinalBlockId = true;
    cancelInFlightSegmentsGreaterThan(m_lastSegmentNo);
    if (m_hasFailure && m_lastSegmentNo >= m_failedSegNo) {
      // previously failed segment is part of the content
      return onFailure(m_failureReason);
    } else {
      m_hasFailure = false;
    }
  }

  uint64_t recvSegNo = data.getName()[-1].toSegment();
  if (m_highData < recvSegNo) {
    m_highData = recvSegNo;
  }

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

  // for segments in retransmission queue, no need to decrement m_nInFlight since
  // it's already been decremented when segments timed out
  if (segInfo.state != SegmentState::InRetxQueue && m_nInFlight > 0) {
    m_nInFlight--;
  }

  m_receivedSize += data.getContent().value_size();
  m_nReceived++;

  increaseWindow();
  onData(interest, data);

  if (segInfo.state == SegmentState::FirstTimeSent ||
      segInfo.state == SegmentState::InRetxQueue) { // do not sample RTT for retransmitted segments
    size_t nExpectedSamples = std::max(static_cast<int>(std::ceil(m_nInFlight / 2.0)), 1);
    m_rttEstimator.addMeasurement(recvSegNo, rtt, nExpectedSamples);
    m_segmentInfo.erase(recvSegNo); // remove the entry associated with the received segment
  }
  else { // retransmission
    segInfo.state = SegmentState::RetxReceived;
  }

  BOOST_ASSERT(m_nReceived > 0);
  if (m_hasFinalBlockId && m_nReceived - 1 >= m_lastSegmentNo) { // all segments have been received
    cancel();
    if (m_options.isVerbose) {
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

  uint64_t segNo = interest.getName()[-1].toSegment();

  switch (nack.getReason()) {
    case lp::NackReason::DUPLICATE: {
      break; // ignore duplicates
    }
    case lp::NackReason::CONGESTION: { // treated the same as timeout for now
      m_retxQueue.push(segNo); // put on retx queue
      m_segmentInfo[segNo].state = SegmentState::InRetxQueue; // update state
      handleTimeout(1);
      break;
    }
    default: {
      handleFail(segNo, "Could not retrieve data for " + interest.getName().toUri() +
                 ", reason: " + boost::lexical_cast<std::string>(nack.getReason()));
      break;
    }
  }
}

void
PipelineInterestsAimd::handleLifetimeExpiration(const Interest& interest)
{
  if (isStopping())
    return;

  uint64_t segNo = interest.getName()[-1].toSegment();
  m_retxQueue.push(segNo); // put on retx queue
  m_segmentInfo[segNo].state = SegmentState::InRetxQueue; // update state
  handleTimeout(1);
}

void
PipelineInterestsAimd::handleTimeout(int timeoutCount)
{
  if (timeoutCount <= 0)
    return;

  if (m_options.disableCwa || m_highData > m_recPoint) {
    // react to only one timeout per RTT (conservative window adaptation)
    m_recPoint = m_highInterest;

    decreaseWindow();
    m_rttEstimator.backoffRto();
    m_nLossEvents++;

    if (m_options.isVerbose) {
      std::cerr << "Packet loss event, cwnd = " << m_cwnd
                << ", ssthresh = " << m_ssthresh << std::endl;
    }
  }

  if (m_nInFlight > static_cast<uint64_t>(timeoutCount))
    m_nInFlight -= timeoutCount;
  else
    m_nInFlight = 0;

  schedulePackets();
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
    if (m_nInFlight > 0)
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
  } else {
    m_cwnd += m_options.aiStep / std::floor(m_cwnd); // congestion avoidance
  }
  afterCwndChange(time::steady_clock::now() - m_startTime, m_cwnd);
}

void
PipelineInterestsAimd::decreaseWindow()
{
  // please refer to RFC 5681, Section 3.1 for the rationale behind it
  m_ssthresh = std::max(2.0, m_cwnd * m_options.mdCoef); // multiplicative decrease
  m_cwnd = m_options.resetCwndToInit ? m_options.initCwnd : m_ssthresh;
  afterCwndChange(time::steady_clock::now() - m_startTime, m_cwnd);
}

uint64_t
PipelineInterestsAimd::getNextSegmentNo()
{
  // get around the excluded segment
  if (m_nextSegmentNo == m_excludedSegmentNo)
    m_nextSegmentNo++;
  return m_nextSegmentNo++;
}

void
PipelineInterestsAimd::cancelInFlightSegmentsGreaterThan(uint64_t segmentNo)
{
  for (auto it = m_segmentInfo.begin(); it != m_segmentInfo.end();) {
    // cancel fetching all segments that follow
    if (it->first > segmentNo) {
      m_face.removePendingInterest(it->second.interestId);
      it = m_segmentInfo.erase(it);
      if (m_nInFlight > 0)
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
  Milliseconds timePassed = time::steady_clock::now() - m_startTime;
  double throughput = (8 * m_receivedSize * 1000) / timePassed.count();

  int pow = 0;
  std::string throughputUnit;
  while (throughput >= 1000.0 && pow < 4) {
    throughput /= 1000.0;
    pow++;
  }
  switch (pow) {
    case 0:
      throughputUnit = "bit/s";
      break;
    case 1:
      throughputUnit = "kbit/s";
      break;
    case 2:
      throughputUnit = "Mbit/s";
      break;
    case 3:
      throughputUnit = "Gbit/s";
      break;
    case 4:
      throughputUnit = "Tbit/s";
      break;
  }

  std::cerr << "\nAll segments have been received.\n"
            << "Total # of segments received: " << m_nReceived << "\n"
            << "Time used: " << timePassed.count() << " ms" << "\n"
            << "Total # of packet loss burst: " << m_nLossEvents << "\n"
            << "Packet loss rate: "
            << static_cast<double>(m_nLossEvents) / static_cast<double>(m_nReceived) << "\n"
            << "Total # of retransmitted segments: " << m_nRetransmitted << "\n"
            << "Goodput: " << throughput << " " << throughputUnit << "\n";
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
  os << "PipelineInterestsAimd initial parameters:" << "\n"
     << "\tInitial congestion window size = " << options.initCwnd << "\n"
     << "\tInitial slow start threshold = " << options.initSsthresh << "\n"
     << "\tMultiplicative decrease factor = " << options.mdCoef << "\n"
     << "\tAdditive increase step = " << options.aiStep << "\n"
     << "\tRTO check interval = " << options.rtoCheckInterval << "\n"
     << "\tMax retries on timeout or Nack = " << options.maxRetriesOnTimeoutOrNack << "\n";

  std::string cwaStatus = options.disableCwa ? "disabled" : "enabled";
  os << "\tConservative Window Adaptation " << cwaStatus << "\n";

  std::string cwndStatus = options.resetCwndToInit ? "initCwnd" : "ssthresh";
  os << "\tResetting cwnd to " << cwndStatus << " when loss event occurs" << "\n";
  return os;
}

} // namespace aimd
} // namespace chunks
} // namespace ndn
