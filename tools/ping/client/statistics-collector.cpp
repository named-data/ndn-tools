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

#include "statistics-collector.hpp"

namespace ndn {
namespace ping {
namespace client {

StatisticsCollector::StatisticsCollector(Ping& ping, const Options& options)
  : m_ping(ping)
  , m_options(options)
  , m_nSent(0)
  , m_nReceived(0)
  , m_nNacked(0)
  , m_pingStartTime(time::steady_clock::now())
  , m_minRtt(std::numeric_limits<double>::max())
  , m_maxRtt(0.0)
  , m_sumRtt(0.0)
  , m_sumRttSquared(0.0)
{
  m_ping.afterData.connect(bind(&StatisticsCollector::recordData, this, _2));
  m_ping.afterNack.connect(bind(&StatisticsCollector::recordNack, this));
  m_ping.afterTimeout.connect(bind(&StatisticsCollector::recordTimeout, this));
}

void
StatisticsCollector::recordData(Rtt rtt)
{
  m_nSent++;
  m_nReceived++;

  double rttMs = rtt.count();

  m_minRtt = std::min(m_minRtt, rttMs);

  m_maxRtt = std::max(m_maxRtt, rttMs);

  m_sumRtt += rttMs;
  m_sumRttSquared += rttMs * rttMs;
}

void
StatisticsCollector::recordNack()
{
  m_nSent++;
  m_nNacked++;
}

void
StatisticsCollector::recordTimeout()
{
  m_nSent++;
}

Statistics
StatisticsCollector::computeStatistics()
{
  Statistics statistics;

  statistics.prefix = m_options.prefix;
  statistics.nSent = m_nSent;
  statistics.nReceived = m_nReceived;
  statistics.nNacked = m_nNacked;
  statistics.pingStartTime = m_pingStartTime;
  statistics.minRtt = m_minRtt;
  statistics.maxRtt = m_maxRtt;
  statistics.packetLossRate = static_cast<double>(m_nSent - m_nReceived - m_nNacked) / static_cast<double>(m_nSent);
  statistics.packetNackedRate = static_cast<double>(m_nNacked) / static_cast<double>(m_nSent);
  statistics.sumRtt = m_sumRtt;
  statistics.avgRtt = m_sumRtt / m_nReceived;
  statistics.stdDevRtt = std::sqrt((m_sumRttSquared / m_nReceived) - (statistics.avgRtt * statistics.avgRtt));

  return statistics;
}

std::ostream&
Statistics::printSummary(std::ostream& os) const
{
  os << nReceived << "/" << nSent << " packets, " << packetLossRate * 100.0
     << "% loss, min/avg/max/mdev = " << minRtt << "/" << avgRtt << "/" << maxRtt << "/"
     << stdDevRtt << " ms" << std::endl;

  return os;
}

std::ostream&
operator<<(std::ostream& os, const Statistics& statistics)
{
  os << "\n";
  os << "--- " << statistics.prefix <<" ping statistics ---\n";
  os << statistics.nSent << " packets transmitted";
  os << ", " << statistics.nReceived << " received";
  os << ", " << statistics.nNacked << " nacked";
  os << ", " << statistics.packetLossRate * 100.0 << "% packet loss";
  os << ", " << statistics.packetNackedRate * 100.0 << "% nacked";
  os << ", time " << statistics.sumRtt << " ms";
  if (statistics.nReceived > 0) {
    os << "\n";
    os << "rtt min/avg/max/mdev = ";
    os << statistics.minRtt << "/";
    os << statistics.avgRtt << "/";
    os << statistics.maxRtt << "/";
    os << statistics.stdDevRtt << " ms";
  }

  return os;
}

} // namespace client
} // namespace ping
} // namespace ndn
