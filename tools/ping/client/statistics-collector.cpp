/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2024,  Arizona Board of Regents.
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
 * @author Teng Liang <philoliang@email.arizona.edu>
 */

#include "statistics-collector.hpp"

#include <cmath>

namespace ndn::ping::client {

StatisticsCollector::StatisticsCollector(Ping& ping, const Options& options)
  : m_ping(ping)
  , m_options(options)
{
  m_ping.afterData.connect([this] (auto&&, Rtt rtt) { recordData(rtt); });
  m_ping.afterNack.connect([this] (auto&&...) { recordNack(); });
  m_ping.afterTimeout.connect([this] (auto&&...) { recordTimeout(); });
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
StatisticsCollector::computeStatistics() const
{
  Statistics statistics;

  statistics.prefix = m_options.prefix;
  statistics.nSent = m_nSent;
  statistics.nReceived = m_nReceived;
  statistics.nNacked = m_nNacked;
  statistics.pingStartTime = m_pingStartTime;
  statistics.minRtt = m_minRtt;
  statistics.maxRtt = m_maxRtt;

  if (m_nSent > 0) {
    statistics.packetLossRate = static_cast<double>(m_nSent - m_nReceived - m_nNacked) / static_cast<double>(m_nSent);
    statistics.packetNackedRate = static_cast<double>(m_nNacked) / static_cast<double>(m_nSent);
  }
  else {
    statistics.packetLossRate = std::numeric_limits<double>::quiet_NaN();
    statistics.packetNackedRate = std::numeric_limits<double>::quiet_NaN();
  }

  statistics.sumRtt = m_sumRtt;

  if (m_nReceived > 0) {
    statistics.avgRtt = m_sumRtt / m_nReceived;
    statistics.stdDevRtt = std::sqrt((m_sumRttSquared / m_nReceived) - (statistics.avgRtt * statistics.avgRtt));
  }
  else {
    statistics.avgRtt = std::numeric_limits<double>::quiet_NaN();
    statistics.stdDevRtt = std::numeric_limits<double>::quiet_NaN();
  }

  return statistics;
}

void
Statistics::printSummary(std::ostream& os) const
{
  os << nReceived << "/" << nSent << " packets";

  if (nSent > 0) {
    os << ", " << packetLossRate * 100.0 << "% lost, " << packetNackedRate * 100.0 << "% nacked";
  }

  if (nReceived > 0) {
    os << ", min/avg/max/mdev = " << minRtt << "/" << avgRtt << "/" << maxRtt
       << "/" << stdDevRtt << " ms";
  }

  os << "\n";
}

void
Statistics::printFull(std::ostream& os) const
{
  os << "\n--- " << prefix << " ping statistics ---\n"
     << nSent << " packets transmitted"
     << ", " << nReceived << " received"
     << ", " << nNacked << " nacked";

  if (nSent > 0) {
    os << ", " << packetLossRate * 100.0 << "% lost, " << packetNackedRate * 100.0 << "% nacked";
  }

  os << ", time " << sumRtt << " ms";

  if (nReceived > 0) {
    os << "\nrtt min/avg/max/mdev = " << minRtt << "/" << avgRtt << "/" << maxRtt
       << "/" << stdDevRtt << " ms";
  }

  os << "\n";
}

} // namespace ndn::ping::client
