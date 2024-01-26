/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
 * @author Davide Pesavento <davidepesa@gmail.com>
 */

#include "ndnpoke.hpp"

#include <ndn-cxx/encoding/buffer-stream.hpp>

#include <iostream>

namespace ndn::peek {

NdnPoke::NdnPoke(Face& face, KeyChain& keyChain, std::istream& input, const PokeOptions& options)
  : m_options(options)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_input(input)
  , m_scheduler(m_face.getIoContext())
{
}

void
NdnPoke::start()
{
  auto data = createData();

  if (m_options.wantUnsolicited) {
    return sendData(*data);
  }

  m_registeredPrefix = m_face.setInterestFilter(m_options.name,
    [this, data] (auto&&, const auto& interest) { this->onInterest(interest, *data); },
    [this] (auto&&) { this->onRegSuccess(); },
    [this] (auto&&, const auto& reason) { this->onRegFailure(reason); });
}

std::shared_ptr<Data>
NdnPoke::createData() const
{
  auto data = std::make_shared<Data>(m_options.name);
  data->setFreshnessPeriod(m_options.freshnessPeriod);
  if (m_options.wantFinalBlockId) {
    data->setFinalBlock(m_options.name.at(-1));
  }

  OBufferStream os;
  os << m_input.rdbuf();
  data->setContent(os.buf());

  m_keyChain.sign(*data, m_options.signingInfo);

  return data;
}

void
NdnPoke::sendData(const Data& data)
{
  m_face.put(data);
  m_result = Result::DATA_SENT;

  if (m_options.isVerbose) {
    std::cerr << "DATA: " << data;
  }
}

void
NdnPoke::onInterest(const Interest& interest, const Data& data)
{
  if (m_options.isVerbose) {
    std::cerr << "INTEREST: " << interest << std::endl;
  }

  if (interest.matchesData(data)) {
    m_timeoutEvent.cancel();
    m_registeredPrefix.cancel();
    sendData(data);
  }
  else if (m_options.isVerbose) {
    std::cerr << "Interest cannot be satisfied" << std::endl;
  }
}

void
NdnPoke::onRegSuccess()
{
  if (m_options.isVerbose) {
    std::cerr << "Prefix registration successful" << std::endl;
  }

  if (m_options.timeout) {
    m_timeoutEvent = m_scheduler.schedule(*m_options.timeout, [this] {
      m_result = Result::TIMEOUT;
      m_registeredPrefix.cancel();

      if (m_options.isVerbose) {
        std::cerr << "TIMEOUT" << std::endl;
      }
    });
  }
}

void
NdnPoke::onRegFailure(std::string_view reason)
{
  m_result = Result::PREFIX_REG_FAIL;
  std::cerr << "Prefix registration failure (" << reason << ")" << std::endl;
}

} // namespace ndn::peek
