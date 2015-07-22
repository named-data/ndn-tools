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

#include "pib-validator.hpp"
#include "encoding/pib-common.hpp"
#include "encoding/update-param.hpp"
#include <set>
#include <string>

namespace ndn {
namespace pib {

using std::set;
using std::string;

PibValidator::PibValidator(const PibDb& db, size_t maxCacheSize)
  : m_db(db)
  , m_isMgmtReady(false)
{
  m_owner = m_db.getOwnerName();
  m_mgmtCert = m_db.getMgmtCertificate();

  if (!m_owner.empty() && m_mgmtCert != nullptr)
    m_isMgmtReady = true;

  m_mgmtChangeConnection =
    const_cast<PibDb&>(m_db).mgmtCertificateChanged.connect([this] () {
        m_owner = m_db.getOwnerName();
        m_mgmtCert = m_db.getMgmtCertificate();
        if (!m_owner.empty() && m_mgmtCert != nullptr)
          m_isMgmtReady = true;
      });

  m_keyDeletedConnection =
    const_cast<PibDb&>(m_db).keyDeleted.connect([this] (Name keyName) {
        m_keyCache.erase(keyName);
      });
}

PibValidator::~PibValidator()
{
}

void
PibValidator::checkPolicy(const Interest& interest,
                          int nSteps,
                          const OnInterestValidated& onValidated,
                          const OnInterestValidationFailed& onValidationFailed,
                          std::vector<shared_ptr<ValidationRequest>>& nextSteps)
{
  if (!m_isMgmtReady)
    return onValidationFailed(interest.shared_from_this(), "PibDb is not initialized");

  const Name& interestName = interest.getName();

  if (interestName.size() != SIGNED_PIB_INTEREST_SIZE) {
    return onValidationFailed(interest.shared_from_this(),
                              "Interest is not signed: " + interest.getName().toUri());
  }

  // Check if the user exists in PIB
  string user = interestName.get(OFFSET_USER).toUri();
  if (user != m_owner)
    return onValidationFailed(interest.shared_from_this(), "Wrong user: " + user);

  // Verify signature
  try {
    Signature signature(interestName[OFFSET_SIG_INFO].blockFromValue(),
                        interestName[OFFSET_SIG_VALUE].blockFromValue());
    // KeyLocator is required to contain the name of signing certificate (w/o version)
    if (!signature.hasKeyLocator())
      return onValidationFailed(interest.shared_from_this(), "No valid KeyLocator");

    const KeyLocator& keyLocator = signature.getKeyLocator();
    if (keyLocator.getType() != KeyLocator::KeyLocator_Name)
      return onValidationFailed(interest.shared_from_this(), "Key Locator is not a name");

    // Check if PIB has the corresponding public key
    shared_ptr<PublicKey> publicKey;

    if (keyLocator.getName() == m_mgmtCert->getName().getPrefix(-1)) {
      // the signing key is mgmt key.
      publicKey = make_shared<PublicKey>(m_mgmtCert->getPublicKeyInfo());
    }
    else {
      // the signing key is normal key.
      Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocator.getName());

      shared_ptr<PublicKey> key = m_keyCache.find(keyName);
      if (key != nullptr) {
        // the signing key is cached.
        publicKey = key;
      }
      else {
        // the signing key is not cached.
        publicKey = m_db.getKey(keyName);
        if (publicKey == nullptr) {
          // the signing key does not exist in PIB.
          return onValidationFailed(interest.shared_from_this(), "Public key is not trusted");
        }
        else {
          // the signing key is retrieved from PIB.
          m_keyCache.insert(keyName, publicKey);
        }
      }
    }

    if (verifySignature(interest, signature, *publicKey))
      onValidated(interest.shared_from_this());
    else
      onValidationFailed(interest.shared_from_this(), "Cannot verify signature");

  }
  catch (KeyLocator::Error&) {
    return onValidationFailed(interest.shared_from_this(),
                              "No valid KeyLocator");
  }
  catch (Signature::Error&) {
    return onValidationFailed(interest.shared_from_this(),
                              "No valid signature");
  }
  catch (IdentityCertificate::Error&) {
    return onValidationFailed(interest.shared_from_this(),
                              "Cannot determine the signing key");
  }
  catch (tlv::Error&) {
    return onValidationFailed(interest.shared_from_this(),
                              "Cannot decode signature");
  }
}

void
PibValidator::checkPolicy(const Data& data,
                          int nSteps,
                          const OnDataValidated& onValidated,
                          const OnDataValidationFailed& onValidationFailed,
                          std::vector<shared_ptr<ValidationRequest>>& nextSteps)
{
  // Pib does not express any interest, therefor should not validate any data.
  onValidationFailed(data.shared_from_this(),
                     "PibValidator should not receive data packet");
}

} // namespace pib
} // namespace ndn
