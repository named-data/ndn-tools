/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Arizona Board of Regents.
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

#include "ping.hpp"

#include <ndn-cxx/util/random.hpp>

namespace ndn::ping::client {

Ping::Ping(Face& face, const Options& options)
  : m_options(options)
  , m_nextSeq(options.startSeq)
  , m_face(face)
  , m_scheduler(m_face.getIoContext())
{
  if (m_options.shouldGenerateRandomSeq) {
    m_nextSeq = random::generateWord64();
  }
}

void
Ping::start()
{
  performPing();
}

void
Ping::stop()
{
  m_nextPingEvent.cancel();
}

void
Ping::performPing()
{
  BOOST_ASSERT((m_options.nPings < 0) || (m_nSent < m_options.nPings));

  Interest interest(makePingName(m_nextSeq));
  interest.setMustBeFresh(!m_options.shouldAllowStaleData);
  interest.setInterestLifetime(m_options.timeout);

  auto now = time::steady_clock::now();
  m_face.expressInterest(interest,
    [this, seq = m_nextSeq, now] (auto&&...) { onData(seq, now); },
    [this, seq = m_nextSeq, now] (auto&&, const auto& nack) { onNack(seq, now, nack); },
    [this, seq = m_nextSeq] (auto&&...) { onTimeout(seq); });

  ++m_nSent;
  ++m_nextSeq;
  ++m_nOutstanding;

  if ((m_options.nPings < 0) || (m_nSent < m_options.nPings)) {
    m_nextPingEvent = m_scheduler.schedule(m_options.interval, [this] { performPing(); });
  }
  else {
    finish();
  }
}

void
Ping::onData(uint64_t seq, const time::steady_clock::time_point& sendTime)
{
  time::nanoseconds rtt = time::steady_clock::now() - sendTime;
  afterData(seq, rtt);
  finish();
}

void
Ping::onNack(uint64_t seq, const time::steady_clock::time_point& sendTime, const lp::Nack& nack)
{
  time::nanoseconds rtt = time::steady_clock::now() - sendTime;
  afterNack(seq, rtt, nack.getHeader());
  finish();
}

void
Ping::onTimeout(uint64_t seq)
{
  afterTimeout(seq);
  finish();
}

void
Ping::finish()
{
  if (--m_nOutstanding >= 0) {
    return;
  }
  afterFinish();
}

Name
Ping::makePingName(uint64_t seq) const
{
  Name name(m_options.prefix);
  name.append("ping");
  if (!m_options.clientIdentifier.empty()) {
    name.append(m_options.clientIdentifier);
  }
  name.append(std::to_string(seq));
  return name;
}

} // namespace ndn::ping::client
