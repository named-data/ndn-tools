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
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_CONSUMER_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_CONSUMER_HPP

#include "discover-version.hpp"
#include "pipeline-interests.hpp"

#include <ndn-cxx/security/validator.hpp>

namespace ndn {
namespace chunks {

/**
 * @brief Segmented version consumer
 *
 * Discover the latest version of the data published under a specified prefix, and retrieve all the
 * segments associated to that version. The segments are fetched in order and written to a
 * user-specified stream in the same order.
 */
class Consumer : noncopyable
{
public:
  class ApplicationNackError : public std::runtime_error
  {
  public:
    explicit
    ApplicationNackError(const Data& data)
      : std::runtime_error("Application generated Nack: " + boost::lexical_cast<std::string>(data))
    {
    }
  };

  /**
   * @brief Create the consumer
   */
  Consumer(Validator& validator, bool isVerbose, std::ostream& os = std::cout);

  /**
   * @brief Run the consumer
   */
  void
  run(unique_ptr<DiscoverVersion> discover, unique_ptr<PipelineInterests> pipeline);

private:
  void
  startPipeline(const Data& data);

  void
  onData(const Interest& interest, const Data& data);

  void
  onDataValidated(shared_ptr<const Data> data);

  void
  onFailure(const std::string& reason);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  writeInOrderData();

private:
  Validator& m_validator;
  std::ostream& m_outputStream;
  unique_ptr<DiscoverVersion> m_discover;
  unique_ptr<PipelineInterests> m_pipeline;
  uint64_t m_nextToPrint;
  bool m_isVerbose;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::map<uint64_t, shared_ptr<const Data>> m_bufferedData;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_CONSUMER_HPP
