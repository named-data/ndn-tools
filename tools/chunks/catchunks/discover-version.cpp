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
 * @author Chavoosh Ghasemi
 */

#include "discover-version.hpp"
#include "data-fetcher.hpp"

#include <ndn-cxx/metadata-object.hpp>

#include <iostream>

namespace ndn::chunks {

DiscoverVersion::DiscoverVersion(Face& face, const Name& prefix, const Options& options)
  : m_face(face)
  , m_prefix(prefix)
  , m_options(options)
{
}

void
DiscoverVersion::run()
{
  if (m_options.disableVersionDiscovery || (!m_prefix.empty() && m_prefix[-1].isVersion())) {
    onDiscoverySuccess(m_prefix);
    return;
  }

  Interest interest = MetadataObject::makeDiscoveryInterest(m_prefix)
                      .setInterestLifetime(m_options.interestLifetime);

  m_fetcher = DataFetcher::fetch(m_face, interest,
                                 m_options.maxRetriesOnTimeoutOrNack,
                                 m_options.maxRetriesOnTimeoutOrNack,
                                 FORWARD_TO_MEM_FN(handleData),
                                 [this] (const auto&, const auto& reason) {
                                   onDiscoveryFailure(reason);
                                 },
                                 [this] (const auto&, const auto& reason) {
                                   onDiscoveryFailure(reason);
                                 },
                                 m_options.isVerbose);
}

void
DiscoverVersion::handleData(const Interest& interest, const Data& data)
{
  if (m_options.isVerbose)
    std::cerr << "Data: " << data << "\n";

  // make a metadata object from received metadata packet
  MetadataObject mobject;
  try {
    mobject = MetadataObject(data);
  }
  catch (const tlv::Error& e) {
    onDiscoveryFailure("Invalid metadata packet: "s + e.what());
    return;
  }

  if (mobject.getVersionedName().empty() || !mobject.getVersionedName()[-1].isVersion()) {
    onDiscoveryFailure(mobject.getVersionedName().toUri() + " is not a valid versioned name");
    return;
  }

  if (m_options.isVerbose) {
    std::cerr << "Discovered Data version: " << mobject.getVersionedName()[-1] << "\n";
  }

  onDiscoverySuccess(mobject.getVersionedName());
}

} // namespace ndn::chunks
