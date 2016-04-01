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
 * @author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author: Eric Newberry <enewberry@email.arizona.edu>
 * @author: Teng Liang <philoliang@email.arizona.edu>
 */

#ifndef NDN_TOOLS_PING_CLIENT_PING_HPP
#define NDN_TOOLS_PING_CLIENT_PING_HPP

#include "core/common.hpp"

namespace ndn {
namespace ping {
namespace client {

typedef time::duration<double, time::milliseconds::period> Rtt;

/**
 * @brief options for ndnping client
 */
struct Options
{
  Name prefix;                      //!< prefix pinged
  bool shouldAllowStaleData;        //!< allow stale Data
  bool shouldGenerateRandomSeq;     //!< random ping sequence
  bool shouldPrintTimestamp;        //!< print timestamp
  int nPings;                       //!< number of pings
  time::milliseconds interval;      //!< ping interval
  time::milliseconds timeout;       //!< timeout threshold
  uint64_t startSeq;                //!< start ping sequence number
  name::Component clientIdentifier; //!< client identifier
};

/**
 * @brief NDN modular ping client
 */
class Ping : noncopyable
{
public:
  Ping(Face& face, const Options& options);

  /**
   * @brief Signals on the successful return of a Data packet
   *
   * @param seq ping sequence number
   * @param rtt round trip time
   */
  signal::Signal<Ping, uint64_t, Rtt> afterData;

  /**
   * @brief Signals on the return of a Nack
   *
   * @param seq ping sequence number
   * @param rtt round trip time
   * @param header the received Network NACK header
   */
  signal::Signal<Ping, uint64_t, Rtt, lp::NackHeader> afterNack;

  /**
   * @brief Signals on timeout of a packet
   *
   * @param seq ping sequence number
   */
  signal::Signal<Ping, uint64_t> afterTimeout;

  /**
   * @brief Signals when finished pinging
   */
  signal::Signal<Ping> afterFinish;

  /**
   * @brief Start sending ping interests
   *
   * @note This method is non-blocking and caller need to call face.processEvents()
   */
  void
  start();

  /**
   * @brief Stop sending ping interests
   *
   * This method cancels any future ping interests and does not affect already pending interests.
   *
   * @todo Cancel pending ping interest
   */
  void
  stop();

private:
  /**
   * @brief Creates a ping Name from the sequence number
   *
   * @param seq ping sequence number
   */
  Name
  makePingName(uint64_t seq) const;

  /**
   * @brief Performs individual ping
   */
  void
  performPing();

  /**
   * @brief Called when a Data packet is received in response to a ping
   *
   * @param interest NDN interest
   * @param data returned data
   * @param seq ping sequence number
   * @param sendTime time ping sent
   */
  void
  onData(const Interest& interest,
         const Data& data,
         uint64_t seq,
         const time::steady_clock::TimePoint& sendTime);

  /**
   * @brief Called when a Nack is received in response to a ping
   *
   * @param interest NDN interest
   * @param nack returned nack
   * @param seq ping sequence number
   * @param sendTime time ping sent
   */
  void
  onNack(const Interest& interest,
         const lp::Nack& nack,
         uint64_t seq,
         const time::steady_clock::TimePoint& sendTime);

  /**
   * @brief Called when ping timed out
   *
   * @param interest NDN interest
   * @param seq ping sequence number
   */
  void
  onTimeout(const Interest& interest, uint64_t seq);

  /**
   * @brief Called after ping received or timed out
   */
  void
  finish();

private:
  const Options& m_options;
  int m_nSent;
  uint64_t m_nextSeq;
  int m_nOutstanding;
  Face& m_face;
  scheduler::Scheduler m_scheduler;
  scheduler::ScopedEventId m_nextPingEvent;
};

} // namespace client
} // namespace ping
} // namespace ndn

#endif // NDN_TOOLS_PING_CLIENT_PING_HPP
