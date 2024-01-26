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

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_ADAPTIVE_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_ADAPTIVE_HPP

#include "pipeline-interests.hpp"

#include <ndn-cxx/util/rtt-estimator.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/signal.hpp>

#include <queue>
#include <unordered_map>

namespace ndn::chunks {

using util::RttEstimatorWithStats;

/**
 * @brief indicates the state of the segment
 */
enum class SegmentState {
  FirstTimeSent, ///< segment has been sent for the first time
  InRetxQueue,   ///< segment is in retransmission queue
  Retransmitted, ///< segment has been retransmitted
};

std::ostream&
operator<<(std::ostream& os, SegmentState state);

/**
 * @brief Wraps up information that's necessary for segment transmission
 */
struct SegmentInfo
{
  ScopedPendingInterestHandle interestHdl;
  time::steady_clock::time_point timeSent;
  time::nanoseconds rto;
  SegmentState state;
};

/**
 * @brief Service for retrieving Data via an Interest pipeline
 *
 * Retrieves all segmented Data under the specified prefix by maintaining a dynamic
 * congestion window combined with a Conservative Loss Adaptation algorithm. For details,
 * please refer to the description in section "Interest pipeline types in ndncatchunks" of
 * tools/chunks/README.md
 *
 * Provides retrieved Data on arrival with no ordering guarantees. Data is delivered to the
 * PipelineInterests' user via callback immediately upon arrival.
 */
class PipelineInterestsAdaptive : public PipelineInterests
{
public:
  /**
   * @brief Constructor.
   *
   * Configures the pipelining service without specifying the retrieval namespace. After this
   * configuration the method run must be called to start the Pipeline.
   */
  PipelineInterestsAdaptive(Face& face, RttEstimatorWithStats& rttEstimator, const Options& opts);

  ~PipelineInterestsAdaptive() override;

  /**
   * @brief Signals when the congestion window changes.
   *
   * The callback function should be: `void(nanoseconds age, double cwnd)`, where `age` is the
   * time since the pipeline started and `cwnd` is the new congestion window size (in segments).
   */
  signal::Signal<PipelineInterestsAdaptive, time::nanoseconds, double> afterCwndChange;

  struct RttSample
  {
    uint64_t segNum;          ///< segment number on which this sample was taken
    time::nanoseconds rtt;    ///< measured RTT
    time::nanoseconds sRtt;   ///< smoothed RTT
    time::nanoseconds rttVar; ///< RTT variation
    time::nanoseconds rto;    ///< retransmission timeout
  };

  /**
   * @brief Signals when a new RTT sample has been taken.
   */
  signal::Signal<PipelineInterestsAdaptive, RttSample> afterRttMeasurement;

protected:
  DECLARE_SIGNAL_EMIT(afterCwndChange)

  void
  printOptions() const;

private:
  /**
   * @brief Increase congestion window.
   */
  virtual void
  increaseWindow() = 0;

  /**
   * @brief Decrease congestion window.
   */
  virtual void
  decreaseWindow() = 0;

private:
  /**
   * @brief Fetch all the segments between 0 and lastSegment of the specified prefix.
   *
   * Starts the pipeline with an adaptive window algorithm to control the window size.
   * The pipeline will fetch every segment until the last segment is successfully received
   * or an error occurs.
   */
  void
  doRun() final;

  /**
   * @brief Stop all fetch operations.
   */
  void
  doCancel() final;

  /**
   * @brief Check RTO for all sent-but-not-acked segments.
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
  recordTimeout(uint64_t segNo);

  void
  enqueueForRetransmission(uint64_t segNo);

  void
  handleFail(uint64_t segNo, const std::string& reason);

  void
  cancelInFlightSegmentsGreaterThan(uint64_t segNo);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  printSummary() const final;

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  static constexpr double MIN_SSTHRESH = 2.0;

  double m_cwnd; ///< current congestion window size (in segments)
  double m_ssthresh; ///< current slow start threshold
  RttEstimatorWithStats& m_rttEstimator;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  Scheduler m_scheduler;
  scheduler::ScopedEventId m_checkRtoEvent;

  uint64_t m_highData = 0; ///< the highest segment number of the Data packet the consumer has received so far
  uint64_t m_highInterest = 0; ///< the highest segment number of the Interests the consumer has sent so far
  uint64_t m_recPoint = 0; ///< the value of m_highInterest when a packet loss event occurred,
                           ///< it remains fixed until the next packet loss event happens

  int64_t m_nInFlight = 0; ///< # of segments in flight
  int64_t m_nLossDecr = 0; ///< # of window decreases caused by packet loss
  int64_t m_nMarkDecr = 0; ///< # of window decreases caused by congestion marks
  int64_t m_nTimeouts = 0; ///< # of timed out segments
  int64_t m_nSkippedRetx = 0; ///< # of segments queued for retransmission but received before the
                              ///< retransmission occurred
  int64_t m_nRetransmitted = 0; ///< # of retransmitted segments
  int64_t m_nCongMarks = 0; ///< # of data packets with congestion mark
  int64_t m_nSent = 0; ///< # of interest packets sent out (including retransmissions)

  std::unordered_map<uint64_t, SegmentInfo> m_segmentInfo; ///< keeps all the internal information
                                                           ///< on sent but not acked segments
  std::unordered_map<uint64_t, int> m_retxCount; ///< maps segment number to its retransmission count;
                                                 ///< if the count reaches to the maximum number of
                                                 ///< timeout/nack retries, the pipeline will be aborted
  std::queue<uint64_t> m_retxQueue;

  bool m_hasFailure = false;
  uint64_t m_failedSegNo = 0;
  std::string m_failureReason;
};

} // namespace ndn::chunks

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_ADAPTIVE_HPP
