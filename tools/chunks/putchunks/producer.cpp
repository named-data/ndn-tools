/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2024, Regents of the University of California,
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
 * @author Chavoosh Ghasemi
 */

#include "producer.hpp"

#include <ndn-cxx/metadata-object.hpp>
#include <ndn-cxx/util/segmenter.hpp>

#include <iostream>

namespace ndn::chunks {

Producer::Producer(const Name& prefix, Face& face, KeyChain& keyChain, std::istream& is,
                   const Options& opts)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_options(opts)
{
  if (!prefix.empty() && prefix[-1].isVersion()) {
    m_prefix = prefix.getPrefix(-1);
    m_versionedPrefix = prefix;
  }
  else {
    m_prefix = prefix;
    m_versionedPrefix = Name(m_prefix).appendVersion();
  }

  if (!m_options.isQuiet) {
    std::cerr << "Loading input ...\n";
  }
  Segmenter segmenter(m_keyChain, m_options.signingInfo);
  m_store = segmenter.segment(is, m_versionedPrefix, m_options.maxSegmentSize, m_options.freshnessPeriod);

  // register m_prefix without Interest handler
  m_face.registerPrefix(m_prefix, nullptr, [this] (const Name& prefix, const auto& reason) {
    std::cerr << "ERROR: Failed to register prefix '" << prefix << "' (" << reason << ")\n";
    m_face.shutdown();
  });

  // match Interests whose name starts with m_versionedPrefix
  face.setInterestFilter(m_versionedPrefix, [this] (const auto&, const auto& interest) {
    processSegmentInterest(interest);
  });

  // match Interests whose name is exactly m_prefix
  face.setInterestFilter(InterestFilter(m_prefix, ""), [this] (const auto&, const auto& interest) {
    processSegmentInterest(interest);
  });

  // match discovery Interests
  auto discoveryName = MetadataObject::makeDiscoveryInterest(m_prefix).getName();
  face.setInterestFilter(discoveryName, [this] (const auto&, const auto& interest) {
    processDiscoveryInterest(interest);
  });

  if (m_options.wantShowVersion) {
    std::cout << m_versionedPrefix[-1] << "\n";
  }
  if (!m_options.isQuiet) {
    std::cerr << "Published " << m_store.size() << " Data packet" << (m_store.size() > 1 ? "s" : "")
              << " with prefix " << m_versionedPrefix << "\n";
  }
}

void
Producer::run()
{
  m_face.processEvents();
}

void
Producer::processDiscoveryInterest(const Interest& interest)
{
  if (m_options.isVerbose)
    std::cerr << "Discovery Interest: " << interest << "\n";

  if (!interest.getCanBePrefix()) {
    if (m_options.isVerbose) {
      std::cerr << "Discovery Interest lacks CanBePrefix, sending Nack\n";
    }
    m_face.put(lp::Nack(interest));
    return;
  }

  MetadataObject mobject;
  mobject.setVersionedName(m_versionedPrefix);

  // make a metadata packet based on the received discovery Interest name
  auto mdata = mobject.makeData(interest.getName(), m_keyChain, m_options.signingInfo);

  if (m_options.isVerbose)
    std::cerr << "Sending metadata: " << mdata << "\n";

  m_face.put(mdata);
}

void
Producer::processSegmentInterest(const Interest& interest)
{
  BOOST_ASSERT(!m_store.empty());

  if (m_options.isVerbose)
    std::cerr << "Interest: " << interest << "\n";

  const Name& name = interest.getName();
  std::shared_ptr<Data> data;

  if (name.size() == m_versionedPrefix.size() + 1 && name[-1].isSegment()) {
    const auto segmentNo = static_cast<size_t>(interest.getName()[-1].toSegment());
    // specific segment retrieval
    if (segmentNo < m_store.size()) {
      data = m_store[segmentNo];
    }
  }
  else if (interest.matchesData(*m_store[0])) {
    // unspecified version or segment number, return first segment
    data = m_store[0];
  }

  if (data != nullptr) {
    if (m_options.isVerbose) {
      std::cerr << "Data: " << *data << "\n";
    }
    m_face.put(*data);
  }
  else {
    if (m_options.isVerbose) {
      std::cerr << "Interest cannot be satisfied, sending Nack\n";
    }
    m_face.put(lp::Nack(interest));
  }
}

} // namespace ndn::chunks
