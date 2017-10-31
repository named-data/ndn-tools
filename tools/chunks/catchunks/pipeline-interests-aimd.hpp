/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2017, Regents of the University of California,
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

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_AIMD_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_AIMD_HPP

#include "options.hpp"
#include "aimd-rtt-estimator.hpp"
#include "pipeline-interests.hpp"

#include <queue>
#include <unordered_map>

namespace ndn {
namespace chunks {
namespace aimd {

class PipelineInterestsAimdOptions : public Options
{
public:
  explicit
  PipelineInterestsAimdOptions(const Options& options = Options())
    : Options(options)
  {
  }

public:
  double initCwnd = 1.0; ///< initial congestion window size
  double initSsthresh = std::numeric_limits<double>::max(); ///< initial slow start threshold
  double aiStep = 1.0; ///< additive increase step (in segments)
  double mdCoef = 0.5; ///< multiplicative decrease coefficient
  time::milliseconds rtoCheckInterval{10}; ///< interval for checking retransmission timer
  bool disableCwa = false; ///< disable Conservative Window Adaptation
  bool resetCwndToInit = false; ///< reduce cwnd to initCwnd when loss event occurs
};

/**
 * @brief indicates the state of the segment
 */
enum class SegmentState {
  FirstTimeSent, ///< segment has been sent for the first time
  InRetxQueue,   ///< segment is in retransmission queue
  Retransmitted, ///< segment has been retransmitted
  RetxReceived,  ///< segment has been received after retransmission
};

std::ostream&
operator<<(std::ostream& os, SegmentState state);

/**
 * @brief Wraps up information that's necessary for segment transmission
 */
struct SegmentInfo
{
  const PendingInterestId* interestId; ///< pending interest ID returned by ndn::Face::expressInterest
  time::steady_clock::TimePoint timeSent;
  Milliseconds rto;
  SegmentState state;
};

/**
 * @brief Service for retrieving Data via an Interest pipeline
 *
 * Retrieves all segmented Data under the specified prefix by maintaining a dynamic AIMD
 * congestion window combined with a Conservative Loss Adaptation algorithm. For details,
 * please refer to the description in section "Interest pipeline types in ndncatchunks" of
 * tools/chunks/README.md
 *
 * Provides retrieved Data on arrival with no ordering guarantees. Data is delivered to the
 * PipelineInterests' user via callback immediately upon arrival.
 */
class PipelineInterestsAimd : public PipelineInterests
{
public:
  typedef PipelineInterestsAimdOptions Options;

public:
  /**
   * @brief create a PipelineInterestsAimd service
   *
   * Configures the pipelining service without specifying the retrieval namespace. After this
   * configuration the method run must be called to start the Pipeline.
   */
  PipelineInterestsAimd(Face& face, RttEstimator& rttEstimator,
                        const Options& options = Options());

  ~PipelineInterestsAimd() final;

  /**
   * @brief Signals when cwnd changes
   *
   * The callback function should be: void(Milliseconds age, double cwnd) where age is the
   * duration since pipeline starts, and cwnd is the new congestion window size (in segments).
   */
  signal::Signal<PipelineInterestsAimd, Milliseconds, double> afterCwndChange;

private:
  /**
   * @brief fetch all the segments between 0 and lastSegment of the specified prefix
   *
   * Starts the pipeline with an AIMD algorithm to control the window size. The pipeline will fetch
   * every segment until the last segment is successfully received or an error occurs.
   * The segment with segment number equal to m_excludedSegmentNo will not be fetched.
   */
  void
  doRun() final;

  /**
   * @brief stop all fetch operations
   */
  void
  doCancel() final;

  /**
   * @brief check RTO for all sent-but-not-acked segments.
   */
  void
  checkRto();

  /**
   * @param segNo the segment # of the to-be-sent Interest
   * @param isRetransmission true if this is a retransmission
   */
  void
  sendInterest(uint64_t segNo, bool isRetransmission);

  void
  schedulePackets();

  void
  handleData(const Interest& interest, const Data& data);

  void
  handleNack(const Interest& interest, const lp::Nack& nack);

  void
  handleLifetimeExpiration(const Interest& interest);

  void
  recordTimeout();

  void
  enqueueForRetransmission(uint64_t segNo);

  void
  handleFail(uint64_t segNo, const std::string& reason);

  /**
   * @brief increase congestion window size based on AIMD scheme
   */
  void
  increaseWindow();

  /**
   * @brief decrease congestion window size based on AIMD scheme
   */
  void
  decreaseWindow();

  /** \return next segment number to retrieve
   *  \post m_nextSegmentNo == return-value + 1
   */
  uint64_t
  getNextSegmentNo();

  void
  cancelInFlightSegmentsGreaterThan(uint64_t segNo);

  void
  printSummary() const final;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  const Options m_options;
  RttEstimator& m_rttEstimator;
  Scheduler m_scheduler;
  scheduler::ScopedEventId m_checkRtoEvent;

  uint64_t m_nextSegmentNo;

  uint64_t m_highData; ///< the highest segment number of the Data packet the consumer has received so far
  uint64_t m_highInterest; ///< the highest segment number of the Interests the consumer has sent so far
  uint64_t m_recPoint; ///< the value of m_highInterest when a packet loss event occurred,
                       ///< it remains fixed until the next packet loss event happens

  int64_t m_nInFlight; ///< # of segments in flight
  int64_t m_nLossEvents; ///< # of loss events occurred
  int64_t m_nRetransmitted; ///< # of segments retransmitted

  double m_cwnd; ///< current congestion window size (in segments)
  double m_ssthresh; ///< current slow start threshold

  std::unordered_map<uint64_t, SegmentInfo> m_segmentInfo; ///< keeps all the internal information
                                                           ///< on sent but not acked segments
  std::unordered_map<uint64_t, int> m_retxCount; ///< maps segment number to its retransmission count;
                                                 ///< if the count reaches to the maximum number of
                                                 ///< timeout/nack retries, the pipeline will be aborted
  std::queue<uint64_t> m_retxQueue;

  bool m_hasFailure;
  uint64_t m_failedSegNo;
  std::string m_failureReason;
};

std::ostream&
operator<<(std::ostream& os, const PipelineInterestsAimdOptions& options);

} // namespace aimd

using aimd::PipelineInterestsAimd;

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_AIMD_HPP
