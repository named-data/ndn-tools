/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2019, Regents of the University of California,
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
 * @author Chavoosh Ghasemi <chghasemi@cs.arizona.edu>
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_REALTIME_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_REALTIME_HPP

#include "discover-version.hpp"

namespace ndn {
namespace chunks {

/**
 * @brief Options for discover version realtime
 *
 * The canonical name to use is DiscoverVersionRealtime::Options
 */
class DiscoverVersionRealtimeOptions : public virtual Options
{
public:
  explicit
  DiscoverVersionRealtimeOptions(const Options& opts = Options())
    : Options(opts)
  {
  }

public:
  time::milliseconds discoveryTimeout{DEFAULT_INTEREST_LIFETIME}; ///< timeout for version discovery
};

/**
 * @brief Service for discovering Data version by sending discovery interests
 *
 * This service employs RDR-style metadata class for version discovery.
 * @see https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR
 */
class DiscoverVersionRealtime : public DiscoverVersion, protected DiscoverVersionRealtimeOptions
{
public:
  typedef DiscoverVersionRealtimeOptions Options;

public:
  /**
   * @param prefix Name of solicited data
   * @param face The face through which discovery Interests are expressed
   * @param options Additional information about the service
   */
  DiscoverVersionRealtime(const Name& prefix, Face& face, const Options& options);

  /**
   * @brief Send a discovery Interest out
   */
  void
  run() final;

private:
  void
  handleData(const Interest& interest, const Data& data) final;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_DISCOVER_VERSION_REALTIME_HPP
