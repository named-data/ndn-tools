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
 */

#include "ping-server.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/exception.hpp>

namespace ndn::ping::server {

PingServer::PingServer(Face& face, KeyChain& keyChain, const Options& options)
  : m_options(options)
  , m_face(face)
  , m_keyChain(keyChain)
{
  auto b = std::make_shared<Buffer>();
  b->assign(m_options.payloadSize, 'a');
  m_payload = Block(tlv::Content, std::move(b));
}

void
PingServer::start()
{
  m_registeredPrefix = m_face.setInterestFilter(Name(m_options.prefix).append("ping"),
                         [this] (const auto&, const auto& interest) { onInterest(interest); },
                         [] (const auto&, const auto& reason) {
                           NDN_THROW(std::runtime_error("Failed to register prefix: " + reason));
                         });
}

void
PingServer::stop()
{
  m_registeredPrefix.cancel();
}

size_t
PingServer::getNPings() const
{
  return m_nPings;
}

void
PingServer::onInterest(const Interest& interest)
{
  afterReceive(interest.getName());

  auto data = std::make_shared<Data>(interest.getName());
  data->setFreshnessPeriod(m_options.freshnessPeriod);
  data->setContent(m_payload);
  m_keyChain.sign(*data, signingWithSha256());
  m_face.put(*data);

  ++m_nPings;
  if (m_options.nMaxPings > 0 && m_options.nMaxPings == m_nPings) {
    afterFinish();
  }
}

} // namespace ndn::ping::server
