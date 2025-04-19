/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2025, Regents of the University of California,
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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 */

#ifndef NDN_TOOLS_GET_PIPELINE_INTERESTS_HPP
#define NDN_TOOLS_GET_PIPELINE_INTERESTS_HPP

#include "core/common.hpp"
#include "options.hpp"

#include <ndn-cxx/face.hpp>

#include <functional>

namespace ndn::get {

/**
 * @brief Service for retrieving Data via an Interest pipeline
 *
 * Retrieves all segments of Data under a given prefix by maintaining a (variable or fixed-size)
 * window of N Interests in flight. A user-specified callback function is used to notify
 * the arrival of each segment of Data.
 *
 * No guarantees are made as to the order in which segments are fetched or callbacks are invoked,
 * i.e. out-of-order delivery is possible.
 */
class PipelineInterests : noncopyable
{
public:
  /**
   * @brief Constructor.
   *
   * Configures the pipelining service without specifying the retrieval namespace.
   * After construction, the method run() must be called in order to start the pipeline.
   */
  PipelineInterests(Face& face, const Options& opts);

  virtual
  ~PipelineInterests();

  using DataCallback = std::function<void(const Data&)>;
  using FailureCallback = std::function<void(const std::string& reason)>;

  /**
   * @brief start fetching all the segments of the specified prefix
   *
   * @param versionedName the name of the segmented Data ending with a version number
   * @param onData callback for every segment correctly received, must not be empty
   * @param onFailure callback if an error occurs, may be empty
   */
  void
  run(const Name& versionedName, DataCallback onData, FailureCallback onFailure);

  /**
   * @brief stop all fetch operations
   */
  void
  cancel();

protected:
  time::steady_clock::time_point
  getStartTime() const
  {
    return m_startTime;
  }

  bool
  isStopping() const
  {
    return m_isStopping;
  }

  /**
   * @brief check if the transfer is complete
   * @return true if all segments have been received, false otherwise
   */
  [[nodiscard]] bool
  allSegmentsReceived() const;

  /**
   * @return next segment number to retrieve
   * @post m_nextSegmentNo == return-value + 1
   */
  uint64_t
  getNextSegmentNo();

  /**
   * @brief subclasses must call this method to notify successful retrieval of a segment
   */
  void
  onData(const Data& data);

  /**
   * @brief subclasses can call this method to signal an unrecoverable failure
   */
  void
  onFailure(const std::string& reason);

  void
  printOptions() const;

  /**
   * @brief print statistics about this fetching session
   *
   * Subclasses can override this method to print additional stats or change the summary format
   */
  virtual void
  printSummary() const;

  /**
   * @param throughput The throughput in bits/s
   */
  static std::string
  formatThroughput(double throughput);

private:
  /**
   * @brief perform subclass-specific operations to fetch all the segments
   *
   * When overriding this function, at a minimum, the subclass should implement the retrieving
   * of all the segments. Subclass must guarantee that `onData` is called once for every
   * segment that is fetched successfully.
   *
   * @note m_lastSegmentNo contains a valid value only if m_hasFinalBlockId is true.
   */
  virtual void
  doRun() = 0;

  virtual void
  doCancel() = 0;

protected:
  const Options& m_options;
  Face& m_face;
  Name m_prefix;

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  bool m_hasFinalBlockId = false; ///< true if the last segment number is known
  uint64_t m_lastSegmentNo = 0;   ///< valid only if m_hasFinalBlockId == true
  int64_t m_nReceived = 0;        ///< number of segments received
  size_t m_receivedSize = 0;      ///< size of received data in bytes

private:
  DataCallback m_onData;
  FailureCallback m_onFailure;
  uint64_t m_nextSegmentNo = 0;
  time::steady_clock::time_point m_startTime;
  bool m_isStopping = false;
};

template<typename Packet>
uint64_t
getSegmentFromPacket(const Packet& packet)
{
  return packet.getName().at(-1).toSegment();
}

} // namespace ndn::get

#endif // NDN_TOOLS_GET_PIPELINE_INTERESTS_HPP
