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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Chavoosh Ghasemi
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP

#include "options.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn::chunks {

class DataFetcher;

/**
 * @brief Service for discovering the latest Data version.
 *
 * DiscoverVersion's user is notified once after identifying the latest retrievable version or
 * on failure to find any Data version.
 */
class DiscoverVersion
{
public:
  DiscoverVersion(Face& face, const Name& prefix, const Options& options);

  /**
   * @brief Signal emitted when the versioned name of Data is found.
   */
  signal::Signal<DiscoverVersion, Name> onDiscoverySuccess;

  /**
   * @brief Signal emitted when a failure occurs.
   */
  signal::Signal<DiscoverVersion, std::string> onDiscoveryFailure;

  void
  run();

private:
  void
  handleData(const Interest& interest, const Data& data);

private:
  Face& m_face;
  const Name m_prefix;
  const Options& m_options;
  std::shared_ptr<DataFetcher> m_fetcher;
};

} // namespace ndn::chunks

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_HPP
