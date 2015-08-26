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

#include "ping-server.hpp"

namespace ndn {
namespace ping {
namespace server {

PingServer::PingServer(Face& face, KeyChain& keyChain, const Options& options)
  : m_options(options)
  , m_keyChain(keyChain)
  , m_name(options.prefix)
  , m_nPings(0)
  , m_face(face)
  , m_registeredPrefixId(nullptr)
{
  shared_ptr<Buffer> b = make_shared<Buffer>();
  b->assign(m_options.payloadSize, 'a');
  m_payload = Block(tlv::Content, b);
}

void
PingServer::start()
{
  m_name.append("ping");
  m_registeredPrefixId = m_face.setInterestFilter(m_name,
                                                  bind(&PingServer::onInterest,
                                                       this, _2),
                                                  bind(&PingServer::onRegisterFailed,
                                                       this, _2));
}

void
PingServer::stop()
{
  if (m_registeredPrefixId != nullptr) {
    m_face.unsetInterestFilter(m_registeredPrefixId);
  }
}

int
PingServer::getNPings() const
{
  return m_nPings;
}

void
PingServer::onInterest(const Interest& interest)
{
  Name interestName = interest.getName();

  afterReceive(interestName);

  shared_ptr<Data> data = make_shared<Data>(interestName);
  data->setFreshnessPeriod(m_options.freshnessPeriod);
  data->setContent(m_payload);
  m_keyChain.sign(*data, signingWithSha256());
  m_face.put(*data);

  ++m_nPings;
  if (m_options.shouldLimitSatisfied && m_options.nMaxPings > 0 && m_options.nMaxPings == m_nPings) {
    afterFinish();
  }
}

void
PingServer::onRegisterFailed(const std::string& reason)
{
  throw "Failed to register prefix in local hub's daemon, REASON: " + reason;
}

} // namespace server
} // namespace ping
} // namespace ndn
