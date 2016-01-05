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

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP

#include "core/common.hpp"
#include "options.hpp"

namespace ndn {
namespace chunks {

class DataFetcher;

/**
 * @brief Base class of services for discovering the latest Data version
 *
 * DiscoverVersion's user is notified once after identifying the latest retrievable version or
 * on failure to find any Data version.
 */
class DiscoverVersion : virtual protected Options, noncopyable
{
public: // signals
  /**
   * @brief Signal emited when the first segment of a specific version is found.
   */
  signal::Signal<DiscoverVersion, const Data&> onDiscoverySuccess;

  /**
   * @brief Signal emitted when a failure occurs.
   */
  signal::Signal<DiscoverVersion, const std::string&> onDiscoveryFailure;

  DECLARE_SIGNAL_EMIT(onDiscoverySuccess)
  DECLARE_SIGNAL_EMIT(onDiscoveryFailure)

public:
  /**
   * @brief create a DiscoverVersion service
   */
  DiscoverVersion(const Name& prefix, Face& face, const Options& options);

  /**
   * @brief identify the latest Data version published.
   */
  virtual void
  run() = 0;

protected:
  void
  expressInterest(const Interest& interest, int maxRetriesNack, int maxRetriesTimeout);

  virtual void
  handleData(const Interest& interest, const Data& data);

  virtual void
  handleNack(const Interest& interest, const std::string& reason);

  virtual void
  handleTimeout(const Interest& interest, const std::string& reason);

protected:
  const Name m_prefix;
  Face& m_face;
  shared_ptr<DataFetcher> fetcher;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP
