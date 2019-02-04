/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include "ndnpoke.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

#include <sstream>

namespace ndn {
namespace peek {

NdnPoke::NdnPoke(Face& face, KeyChain& keyChain, std::istream& inStream, const PokeOptions& options)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_inStream(inStream)
  , m_options(options)
  , m_wasDataSent(false)
{
}

void
NdnPoke::start()
{
  shared_ptr<Data> dataPacket = createDataPacket();
  if (m_options.wantForceData) {
    m_face.put(*dataPacket);
    m_wasDataSent = true;
  }
  else {
    m_registeredPrefix = m_face.setInterestFilter(m_options.prefixName,
                                                  bind(&NdnPoke::onInterest, this, _1, _2, dataPacket),
                                                  nullptr,
                                                  bind(&NdnPoke::onRegisterFailed, this, _1, _2));
  }
}

shared_ptr<Data>
NdnPoke::createDataPacket()
{
  auto dataPacket = make_shared<Data>(m_options.prefixName);

  std::stringstream payloadStream;
  payloadStream << m_inStream.rdbuf();
  std::string payload = payloadStream.str();
  dataPacket->setContent(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());

  if (m_options.freshnessPeriod) {
    dataPacket->setFreshnessPeriod(*m_options.freshnessPeriod);
  }

  if (m_options.wantLastAsFinalBlockId) {
    dataPacket->setFinalBlock(m_options.prefixName.get(-1));
  }

  m_keyChain.sign(*dataPacket, m_options.signingInfo);

  return dataPacket;
}

void
NdnPoke::onInterest(const Name& name, const Interest& interest, const shared_ptr<Data>& data)
{
  try {
    m_face.put(*data);
    m_wasDataSent = true;
  }
  catch (const Face::OversizedPacketError& e) {
    std::cerr << "Data exceeded maximum packet size" << std::endl;
  }

  m_registeredPrefix.cancel();
}

void
NdnPoke::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  std::cerr << "Prefix Registration Failure. Reason = " << reason << std::endl;
}

} // namespace peek
} // namespace ndn
