/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015-2016,  Arizona Board of Regents.
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
 * @author: Eric Newberry <enewberry@email.arizona.edu>
 * @author: Teng Liang <philoliang@email.arizona.edu>
 */

#ifndef NDN_TOOLS_PING_CLIENT_TRACER_HPP
#define NDN_TOOLS_PING_CLIENT_TRACER_HPP

#include "core/common.hpp"

#include "ping.hpp"

namespace ndn {
namespace ping {
namespace client {

/**
 * @brief prints ping responses and timeouts
 */
class Tracer : noncopyable
{
public:
  /**
   * @param ping NDN ping client
   * @param options ping client options
   */
  Tracer(Ping& ping, const Options& options);

  /**
   * @brief Prints ping results when a Data packet is received
   *
   * @param seq ping sequence number
   * @param rtt round trip time
   */
  void
  onData(uint64_t seq, Rtt rtt);

  /**
   * @brief Prints NackReason when a Nack is received
   *
   * @param seq ping sequence number
   * @param rtt round trip time
   * @param header the header of Nack
   */
  void
  onNack(uint64_t seq, Rtt rtt, const lp::NackHeader& header);

  /**
   * @brief Prints ping results when timed out
   *
   * @param seq ping sequence number
   */
  void
  onTimeout(uint64_t seq);

  /**
   * @brief Outputs ping errors to cerr
   */
  void
  onError(std::string msg);

private:
  const Options& m_options;
};

} // namespace client
} // namespace ping
} // namespace ndn

#endif // NDN_TOOLS_PING_CLIENT_TRACER_HPP
