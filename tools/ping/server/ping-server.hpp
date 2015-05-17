/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015,  Arizona Board of Regents.
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
 * @author Eric Newberry <enewberry@email.arizona.edu>
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#ifndef NDN_TOOLS_PING_SERVER_PING_SERVER_HPP
#define NDN_TOOLS_PING_SERVER_PING_SERVER_HPP

#include "core/common.hpp"

namespace ndn {
namespace ping {
namespace server {

/**
 * @brief options for ndnping server
 */
struct Options
{
  Name prefix;                        //!< prefix to register
  time::milliseconds freshnessPeriod; //!< freshness period
  bool shouldLimitSatisfied;          //!< should limit the number of pings satisfied
  int nMaxPings;                      //!< max number of pings to satisfy
  bool shouldPrintTimestamp;          //!< print timestamp when response sent
  int payloadSize;                    //!< user specified payload size
};

/**
 * @brief NDN modular ping server
 */
class PingServer : noncopyable
{
public:
  PingServer(Face& face, const Options& options);

  /**
   * Signals when Interest received
   * @param name incoming interest name
   */
  signal::Signal<PingServer, Name> afterReceive;

  /** @brief starts ping server
   *
   *  If options.shouldLimitSatisfied is false, this method does not return unless there's an error.
   *  Otherwise, this method returns when options.nMaxPings Interests are processed.
   */
  void
  run();

  /**
   * @brief gets the number of pings received
   */
  int
  getNPings() const;

private:
  /**
   * Called when interest received
   * @param interest incoming interest
   */
  void
  onInterest(const Interest& interest);

  /**
   * Called when prefix registration failed
   * @param reason reason for failure
   */
  void
  onRegisterFailed(const std::string& reason);

private:
  const Options& m_options;
  Name m_name;
  KeyChain m_keyChain;
  int m_nPings;
  Face& m_face;
  Block m_payload;
};

} // namespace server
} // namespace ping
} // namespace ndn

#endif //NDN_TOOLS_PING_SERVER_PING_SERVER_HPP