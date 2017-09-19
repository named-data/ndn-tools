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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Klaus Schneider
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_ITERATIVE_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_ITERATIVE_HPP

#include "discover-version.hpp"

namespace ndn {
namespace chunks {

/**
 * @brief Options for discover version iterative DiscoverVersionIterative
 *
 * The canonical name to use is DiscoverVersionIterative::Options
 */
class DiscoverVersionIterativeOptions : public virtual Options
{
public:
  explicit
  DiscoverVersionIterativeOptions(const Options& opts = Options())
    : Options(opts)
  {
  }

public:
  int maxRetriesAfterVersionFound{0};       ///< how many times to retry after a discoveryTimeout
  time::milliseconds discoveryTimeout{300}; ///< timeout for version discovery
};

/**
 * @brief Service for discovering the latest Data version in the iterative way
 *
 * Identifies the latest retrievable version published under the specified namespace
 * (as specified by the Version marker).
 *
 * DiscoverVersionIterative declares the largest discovered version to be the latest after some
 * Interest timeouts (i.e. failed retrieval after exclusion and retransmission). The number of
 * timeouts are specified by the value of maxRetriesAfterVersionFound inside the iterative options.
 *
 * The received name component after version can be an invalid segment number, this component will
 * be excluded in the next interests. In the unlikely case that there are too many excluded
 * components such that the Interest cannot fit in ndn::MAX_NDN_PACKET_SIZE, the discovery
 * procedure will throw Face::Error.
 *
 * DiscoverVersionIterative's user is notified once after identifying the latest retrievable
 * version or on failure to find any version Data.
 */
class DiscoverVersionIterative : public DiscoverVersion, protected DiscoverVersionIterativeOptions
{
public:
  typedef DiscoverVersionIterativeOptions Options;

public:
  /**
   * @brief create a DiscoverVersionIterative service
   */
  DiscoverVersionIterative(const Name& prefix, Face& face, const Options& options);

  /**
   * @brief identify the latest Data version published.
   */
  void
  run() final;

private:
  void
  handleData(const Interest& interest, const Data& data) final;

  void
  handleTimeout(const Interest& interest, const std::string& reason) final;

private:
  uint64_t m_latestVersion;
  shared_ptr<const Data> m_latestVersionData;
  Exclude m_strayExcludes;
  bool m_foundVersion;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_ITERATIVE_HPP
