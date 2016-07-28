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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Weiwei Liu
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_HPP

#include "core/common.hpp"

namespace ndn {
namespace chunks {

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
class PipelineInterests
{
public:
  typedef function<void(const std::string& reason)> FailureCallback;

public:
  /**
   * @brief create a PipelineInterests service
   *
   * Configures the pipelining service without specifying the retrieval namespace. After this
   * configuration the method run must be called to start the Pipeline.
   */
  explicit
  PipelineInterests(Face& face);

  virtual
  ~PipelineInterests();

  /**
   * @brief start fetching all the segments of the specified prefix
   *
   * @param data a segment of the segmented Data to fetch; the Data name must end with a segment number
   * @param onData callback for every segment correctly received, must not be empty
   * @param onFailure callback if an error occurs, may be empty
   */
  void
  run(const Data& data, DataCallback onData, FailureCallback onFailure);

  /**
   * @brief stop all fetch operations
   */
  void
  cancel();

protected:
  bool
  isStopping() const
  {
    return m_isStopping;
  }

  void
  onData(const Interest& interest, const Data& data) const
  {
    m_onData(interest, data);
  }

  /**
   * @brief subclasses can call this method to signal an unrecoverable failure
   */
  void
  onFailure(const std::string& reason);

private:
  /**
   * @brief perform subclass-specific operations to fetch all the segments
   *
   * When overriding this function, at a minimum, the subclass should implement the retrieving
   * of all the segments. Segment m_excludedSegmentNo can be skipped. Subclass must guarantee
   * that onData is called at least once for every segment that is fetched successfully.
   *
   * @note m_lastSegmentNo contains a valid value only if m_hasFinalBlockId is true.
   */
  virtual void
  doRun() = 0;

  virtual void
  doCancel() = 0;

protected:
  Face& m_face;
  Name m_prefix;
  uint64_t m_lastSegmentNo;
  uint64_t m_excludedSegmentNo;

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  bool m_hasFinalBlockId; ///< true if the last segment number is known

private:
  DataCallback m_onData;
  FailureCallback m_onFailure;
  bool m_isStopping;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_HPP
