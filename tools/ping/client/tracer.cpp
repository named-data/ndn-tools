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

#include "tracer.hpp"

namespace ndn {
namespace ping {
namespace client {

Tracer::Tracer(Ping& ping, const Options& options)
  : m_options(options)
{
  ping.afterData.connect(bind(&Tracer::onData, this, _1, _2));
  ping.afterNack.connect(bind(&Tracer::onNack, this, _1, _2, _3));
  ping.afterTimeout.connect(bind(&Tracer::onTimeout, this, _1));
}

void
Tracer::onData(uint64_t seq, Rtt rtt)
{
  if (m_options.shouldPrintTimestamp) {
    std::cout << time::toIsoString(time::system_clock::now()) << " - ";
  }

  std::cout << "content from " << m_options.prefix << ": seq=" << seq << " time="
            << rtt.count() << " ms" << std::endl;
}

void
Tracer::onNack(uint64_t seq, Rtt rtt, const lp::NackHeader& header)
{
  if (m_options.shouldPrintTimestamp) {
    std::cout << time::toIsoString(time::system_clock::now()) << " - ";
  }

  std::cout << "nack from " << m_options.prefix << ": seq=" << seq << " time="
            << rtt.count() << " ms" << " reason=" << header.getReason() << std::endl;
}

void
Tracer::onTimeout(uint64_t seq)
{
  if (m_options.shouldPrintTimestamp) {
    std::cout << time::toIsoString(time::system_clock::now()) << " - ";
  }

  std::cout << "timeout from " << m_options.prefix << ": seq=" << seq << std::endl;
}

void
Tracer::onError(std::string msg)
{
  std::cerr << "ERROR: " << msg << std::endl;
}

} // namespace client
} // namespace ping
} // namespace ndn
