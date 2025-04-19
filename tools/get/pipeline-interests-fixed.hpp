/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2025,  Regents of the University of California,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University.
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
 * @author Chavoosh Ghasemi
 */

#ifndef NDN_TOOLS_GET_PIPELINE_INTERESTS_FIXED_HPP
#define NDN_TOOLS_GET_PIPELINE_INTERESTS_FIXED_HPP

#include "pipeline-interests.hpp"

#include <vector>

namespace ndn::get {

class DataFetcher;

/**
 * @brief Service for retrieving Data via an Interest pipeline
 *
 * Retrieves all segments of Data under a given prefix by maintaining a fixed-size window of
 * N Interests in flight. A user-specified callback function is used to notify the arrival of
 * each segment of Data.
 *
 * No guarantees are made as to the order in which segments are fetched or callbacks are invoked,
 * i.e. out-of-order delivery is possible.
 */
class PipelineInterestsFixed final : public PipelineInterests
{
public:
  PipelineInterestsFixed(Face& face, const Options& opts);

  ~PipelineInterestsFixed() final;

private:
  /**
   * @brief fetch all the segments between 0 and m_lastSegmentNo
   *
   * Starts a fixed-window pipeline with size equal to m_options.maxPipelineSize. The pipeline
   * will fetch every segment until the last segment is successfully received or an error occurs.
   */
  void
  doRun() final;

  void
  doCancel() final;

  /**
   * @brief fetch the next segment that has not been requested yet
   *
   * @return false if there is an error or all the segments have been fetched, true otherwise
   */
  bool
  fetchNextSegment(size_t pipeNo);

  void
  handleData(const Interest& interest, const Data& data, size_t pipeNo);

  void
  handleFail(const std::string& reason, size_t pipeNo);

private:
  std::vector<std::pair<std::shared_ptr<DataFetcher>, uint64_t>> m_segmentFetchers;

  /**
   * true if one or more segment fetchers encountered an error; if m_hasFinalBlockId
   * is false, this is usually not a fatal error for the pipeline
   */
  bool m_hasFailure = false;
};

} // namespace ndn::get

#endif // NDN_TOOLS_GET_PIPELINE_INTERESTS_FIXED_HPP
