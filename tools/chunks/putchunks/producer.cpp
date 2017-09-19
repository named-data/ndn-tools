/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2017, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Klaus Schneider
 */

#include "producer.hpp"

namespace ndn {
namespace chunks {

Producer::Producer(const Name& prefix, Face& face, KeyChain& keyChain, std::istream& is,
                   const Options& opts)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_options(opts)
{
  if (prefix.size() > 0 && prefix[-1].isVersion()) {
    m_prefix = prefix.getPrefix(-1);
    m_versionedPrefix = prefix;
  }
  else {
    m_prefix = prefix;
    m_versionedPrefix = Name(m_prefix).appendVersion();
  }

  populateStore(is);

  if (m_options.wantShowVersion)
    std::cout << m_versionedPrefix[-1] << std::endl;

  m_face.setInterestFilter(m_prefix,
                           bind(&Producer::onInterest, this, _2),
                           RegisterPrefixSuccessCallback(),
                           bind(&Producer::onRegisterFailed, this, _1, _2));

  if (!m_options.isQuiet)
    std::cerr << "Data published with name: " << m_versionedPrefix << std::endl;
}

void
Producer::run()
{
  m_face.processEvents();
}

void
Producer::onInterest(const Interest& interest)
{
  BOOST_ASSERT(m_store.size() > 0);

  if (m_options.isVerbose)
    std::cerr << "Interest: " << interest << std::endl;

  const Name& name = interest.getName();
  shared_ptr<Data> data;

  // is this a discovery Interest or a sequence retrieval?
  if (name.size() == m_versionedPrefix.size() + 1 && m_versionedPrefix.isPrefixOf(name) &&
      name[-1].isSegment()) {
    const auto segmentNo = static_cast<size_t>(interest.getName()[-1].toSegment());
    // specific segment retrieval
    if (segmentNo < m_store.size()) {
      data = m_store[segmentNo];
    }
  }
  else if (interest.matchesData(*m_store[0])) {
    // Interest has version and is looking for the first segment or has no version
    data = m_store[0];
  }

  if (data != nullptr) {
    if (m_options.isVerbose)
      std::cerr << "Data: " << *data << std::endl;

    m_face.put(*data);
  }
}

void
Producer::populateStore(std::istream& is)
{
  BOOST_ASSERT(m_store.empty());

  if (!m_options.isQuiet)
    std::cerr << "Loading input ..." << std::endl;

  std::vector<uint8_t> buffer(m_options.maxSegmentSize);
  while (is.good()) {
    is.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    const auto nCharsRead = is.gcount();

    if (nCharsRead > 0) {
      auto data = make_shared<Data>(Name(m_versionedPrefix).appendSegment(m_store.size()));
      data->setFreshnessPeriod(m_options.freshnessPeriod);
      data->setContent(buffer.data(), static_cast<size_t>(nCharsRead));
      m_store.push_back(data);
    }
  }

  if (m_store.empty()) {
    auto data = make_shared<Data>(Name(m_versionedPrefix).appendSegment(0));
    data->setFreshnessPeriod(m_options.freshnessPeriod);
    m_store.push_back(data);
  }

  auto finalBlockId = name::Component::fromSegment(m_store.size() - 1);
  for (const auto& data : m_store) {
    data->setFinalBlockId(finalBlockId);
    m_keyChain.sign(*data, m_options.signingInfo);
  }

  if (!m_options.isQuiet)
    std::cerr << "Created " << m_store.size() << " chunks for prefix " << m_prefix << std::endl;
}

void
Producer::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  std::cerr << "ERROR: Failed to register prefix '"
            << prefix << "' (" << reason << ")" << std::endl;
  m_face.shutdown();
}

} // namespace chunks
} // namespace ndn
