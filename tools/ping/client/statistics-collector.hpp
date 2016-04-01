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
 * @author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author: Teng Liang <philoliang@email.arizona.edu>
 */

#ifndef NDN_TOOLS_PING_CLIENT_STATISTICS_COLLECTOR_HPP
#define NDN_TOOLS_PING_CLIENT_STATISTICS_COLLECTOR_HPP

#include "core/common.hpp"

#include "ping.hpp"

namespace ndn {
namespace ping {
namespace client {

/**
 * @brief statistics data
 */
struct Statistics
{
  Name prefix;                                  //!< prefix pinged
  int nSent;                                    //!< number of pings sent
  int nReceived;                                //!< number of pings received
  int nNacked;                                  //!< number of nacks received
  time::steady_clock::TimePoint pingStartTime;  //!< time pings started
  double minRtt;                                //!< minimum round trip time
  double maxRtt;                                //!< maximum round trip time
  double packetLossRate;                        //!< packet loss rate
  double packetNackedRate;                      //!< packet nacked rate
  double sumRtt;                                //!< sum of round trip times
  double avgRtt;                                //!< average round trip time
  double stdDevRtt;                             //!< std dev of round trip time

  std::ostream&
  printSummary(std::ostream& os) const;
};

/**
 * @brief Statistics collector from ping client
 */
class StatisticsCollector : noncopyable
{
public:
  /**
   * @param ping NDN ping client
   * @param options ping client options
   */
  StatisticsCollector(Ping& ping, const Options& options);

  /**
   * @brief Compute ping statistics as structure
   */
  Statistics
  computeStatistics();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * @brief Called when a Data packet is received
   *
   * @param rtt round trip time
   */
  void
  recordData(Rtt rtt);

  /**
   * @brief Called when a Nack is received
   */
  void
  recordNack();

  /**
   * @brief Called on ping timeout
   */
  void
  recordTimeout();

private:
  Ping& m_ping;
  const Options& m_options;
  int m_nSent;
  int m_nReceived;
  int m_nNacked;
  time::steady_clock::TimePoint m_pingStartTime;
  double m_minRtt;
  double m_maxRtt;
  double m_sumRtt;
  double m_sumRttSquared;
};

std::ostream&
operator<<(std::ostream& os, const Statistics& statistics);

} // namespace client
} // namespace ping
} // namespace ndn

#endif // NDN_TOOLS_PING_CLIENT_STATISTICS_COLLECTOR_HPP
