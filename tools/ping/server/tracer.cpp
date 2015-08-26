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
 */

#include "tracer.hpp"

namespace ndn {
namespace ping {
namespace server {

Tracer::Tracer(PingServer& pingServer, const Options& options)
  : m_options(options)
{
  pingServer.afterReceive.connect(bind(&Tracer::onReceive, this, _1));
}

void
Tracer::onReceive(const Name& name)
{
  if (m_options.shouldPrintTimestamp) {
    std::cout << time::toIsoString(time::system_clock::now())  << " - ";
  }

  std::cout << "interest received: seq=" << name.at(-1).toUri() << std::endl;
}

} // namespace server
} // namespace ping
} // namespace ndn
