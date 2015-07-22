/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California.
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
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "cert-publisher.hpp"

namespace ndn {
namespace pib {

CertPublisher::CertPublisher(Face& face, PibDb& db)
  : m_face(face)
  , m_db(db)
{
  startPublishAll();

  m_certDeletedConnection =
    m_db.certificateDeleted.connect(bind(&CertPublisher::stopPublish, this, _1));
  m_certInsertedConnection =
    m_db.certificateInserted.connect(bind(&CertPublisher::startPublish, this, _1));
}

CertPublisher::~CertPublisher()
{
  for (const auto& it : m_registeredPrefixes)
    m_face.unsetInterestFilter(it.second);
}

void
CertPublisher::startPublishAll()
{
  // For now we have to register the prefix of certificates separately.
  // The reason is that certificates do not share the same prefix that
  // can aggregate the certificates publishing without attracting interests
  // for non-PIB data.

  std::vector<Name> identities = m_db.listIdentities();

  for (const auto& identity : identities) {
    std::vector<Name> keyNames = m_db.listKeyNamesOfIdentity(identity);

    for (const auto& key : keyNames) {
      std::vector<Name> certNames = m_db.listCertNamesOfKey(key);

      for (const auto& certName : certNames)
        startPublish(certName);
    }
  }
}

void
CertPublisher::registerCertPrefix(const Name& certName)
{
  BOOST_ASSERT(!certName.empty());

  const Name& prefix = certName.getPrefix(-1);

  if (m_registeredPrefixes.find(prefix) == m_registeredPrefixes.end()) {
    m_registeredPrefixes[prefix] =
      m_face.setInterestFilter(certName.getPrefix(-1),
        bind(&CertPublisher::processInterest, this, _1, _2),
        [] (const Name& name) {},
        [=] (const Name& name, const std::string& msg) {
          if (m_registeredPrefixes.erase(prefix) == 0) {
            // registration no longer needed - certificates deleted
            return;
          }
          // retry
          registerCertPrefix(certName);
        });

  }
}

void
CertPublisher::processInterest(const InterestFilter& interestFilter,
                               const Interest& interest)
{
  shared_ptr<const Data> certificate = m_responseCache.find(interest);
  if (certificate != nullptr) {
    m_face.put(*certificate);
  }
}

void
CertPublisher::startPublish(const Name& certName)
{
  m_responseCache.insert(*m_db.getCertificate(certName));
  registerCertPrefix(certName);
}

void
CertPublisher::stopPublish(const Name& certName)
{
  BOOST_ASSERT(!certName.empty());

  m_responseCache.erase(certName);

  // clear the listener if this is the only cert using the prefix
  const Name& prefix = certName.getPrefix(-1); // strip version component

  if (m_responseCache.find(prefix) != nullptr)
    return;

  auto it = m_registeredPrefixes.find(prefix);
  BOOST_ASSERT(it != m_registeredPrefixes.end());
  m_face.unsetInterestFilter(it->second);
  m_registeredPrefixes.erase(it);
}

} // namespace pib
} // namespace ndn
