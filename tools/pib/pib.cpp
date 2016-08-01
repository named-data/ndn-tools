/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California.
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

#include "pib.hpp"

#include "encoding/pib-encoding.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/crypto.hpp>
#include <ndn-cxx/util/concepts.hpp>

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

using std::string;
using std::vector;
using std::set;

const Name Pib::PIB_PREFIX("/localhost/pib");
const Name Pib::EMPTY_SIGNER_NAME;
const name::Component Pib::MGMT_LABEL("mgmt");

// \todo make this a static method in KeyChain
static inline void
signWithDigestSha256(Data& data)
{
  DigestSha256 sig;
  data.setSignature(sig);
  Block sigValue(tlv::SignatureValue,
                 crypto::sha256(data.wireEncode().value(),
                                data.wireEncode().value_size() -
                                data.getSignature().getValue().size()));
  data.setSignatureValue(sigValue);
  data.wireEncode();
}

Pib::Pib(Face& face,
         const std::string& dbDir,
         const std::string& tpmLocator,
         const std::string& owner)
  : m_db(dbDir)
  , m_tpm(nullptr)
  , m_owner(owner)
  , m_validator(m_db)
  , m_face(face)
  , m_certPublisher(m_face, m_db)
  , m_getProcessor(m_db)
  , m_defaultProcessor(m_db)
  , m_listProcessor(m_db)
  , m_updateProcessor(m_db, *this)
  , m_deleteProcessor(m_db)
{
  if (!m_db.getOwnerName().empty() && m_db.getOwnerName() != owner)
    throw Error("owner argument differs from OwnerName in database");

  if (!m_db.getTpmLocator().empty() && m_db.getTpmLocator() != tpmLocator)
    throw Error("tpmLocator argument differs from TpmLocator in database");

  initializeTpm(tpmLocator);
  initializeMgmtCert();
  m_db.setTpmLocator(tpmLocator);

  registerPrefix();
}

Pib::~Pib()
{
  m_face.unsetInterestFilter(m_pibMgmtFilterId);
  m_face.unsetInterestFilter(m_pibGetFilterId);
  m_face.unsetInterestFilter(m_pibDefaultFilterId);
  m_face.unsetInterestFilter(m_pibListFilterId);
  m_face.unsetInterestFilter(m_pibUpdateFilterId);
  m_face.unsetInterestFilter(m_pibDeleteFilterId);

  m_face.unsetInterestFilter(m_pibPrefixId);
}

void
Pib::setMgmtCert(std::shared_ptr<IdentityCertificate> mgmtCert)
{
  if (mgmtCert != nullptr)
    m_mgmtCert = mgmtCert;
}

void
Pib::initializeTpm(const string& tpmLocator)
{
  m_tpm = KeyChain::createTpm(tpmLocator);
}

void
Pib::initializeMgmtCert()
{
  shared_ptr<IdentityCertificate> mgmtCert = m_db.getMgmtCertificate();

  if (mgmtCert == nullptr ||
      !m_tpm->doesKeyExistInTpm(mgmtCert->getPublicKeyName(), KeyClass::PRIVATE)) {
    // If mgmt cert is set, or corresponding private key of the current mgmt cert is missing,
    // generate new mgmt cert

    // key name: /localhost/pib/[UserName]/mgmt/dsk-...
    Name mgmtKeyName = PIB_PREFIX;
    mgmtKeyName.append(m_owner).append(MGMT_LABEL);
    std::ostringstream oss;
    oss << "dsk-" << time::toUnixTimestamp(time::system_clock::now()).count();
    mgmtKeyName.append(oss.str());

    // self-sign pib root key
    m_mgmtCert = prepareCertificate(mgmtKeyName, RsaKeyParams(),
                                    time::system_clock::now(),
                                    time::system_clock::now() + time::days(7300));

    // update management certificate in database
    m_db.updateMgmtCertificate(*m_mgmtCert);
  }
  else
    m_mgmtCert = mgmtCert;
}

shared_ptr<IdentityCertificate>
Pib::prepareCertificate(const Name& keyName, const KeyParams& keyParams,
                        const time::system_clock::TimePoint& notBefore,
                        const time::system_clock::TimePoint& notAfter,
                        const Name& signerName)
{
  // Generate mgmt key
  m_tpm->generateKeyPairInTpm(keyName, keyParams);
  shared_ptr<PublicKey> publicKey = m_tpm->getPublicKeyFromTpm(keyName);

  // Set mgmt cert
  auto certificate = make_shared<IdentityCertificate>();
  Name certName = keyName.getPrefix(-1);
  certName.append("KEY").append(keyName.get(-1)).append("ID-CERT").appendVersion();
  certificate->setName(certName);
  certificate->setNotBefore(notBefore);
  certificate->setNotAfter(notAfter);
  certificate->setPublicKeyInfo(*publicKey);
  CertificateSubjectDescription subjectName(oid::ATTRIBUTE_NAME, keyName.getPrefix(-1).toUri());
  certificate->addSubjectDescription(subjectName);
  certificate->encode();


  Name signingKeyName;
  KeyLocator keyLocator;
  if (signerName == EMPTY_SIGNER_NAME) {
    // Self-sign mgmt cert
    keyLocator = KeyLocator(certificate->getName().getPrefix(-1));
    signingKeyName = keyName;
  }
  else {
    keyLocator = KeyLocator(signerName.getPrefix(-1));
    signingKeyName = IdentityCertificate::certificateNameToPublicKeyName(signerName);
  }

  SignatureSha256WithRsa signature(keyLocator);
  certificate->setSignature(signature);
  EncodingBuffer encoder;
  certificate->wireEncode(encoder, true);
  Block signatureValue = m_tpm->signInTpm(encoder.buf(), encoder.size(),
                                          signingKeyName, DigestAlgorithm::SHA256);
  certificate->wireEncode(encoder, signatureValue);

  return certificate;
}

void
Pib::registerPrefix()
{
  // register pib prefix
  Name pibPrefix = PIB_PREFIX;
  pibPrefix.append(m_owner);
  m_pibPrefixId =
    m_face.registerPrefix(pibPrefix,
                          [] (const Name& name) {},
                          [] (const Name& name, const string& msg) {
                            throw Error("cannot register pib prefix");
                          });

  // set interest filter for management certificate
  m_pibMgmtFilterId =
    m_face.setInterestFilter(Name(pibPrefix).append(MGMT_LABEL),
                             [this] (const InterestFilter&, const Interest& interest) {
                               if (m_mgmtCert != nullptr) {
                                 m_face.put(*m_mgmtCert);
                               }
                             });

  // set interest filter for get command
  m_pibGetFilterId = registerProcessor(Name(pibPrefix).append(GetParam::VERB), m_getProcessor);

  // set interest filter for default command
  m_pibDefaultFilterId = registerProcessor(Name(pibPrefix).append(DefaultParam::VERB),
                                           m_defaultProcessor);

  // set interest filter for list command
  m_pibListFilterId = registerProcessor(Name(pibPrefix).append(ListParam::VERB),
                                        m_listProcessor);

  // set interest filter for update command
  m_pibUpdateFilterId = registerSignedCommandProcessor(Name(pibPrefix).append(UpdateParam::VERB),
                                                       m_updateProcessor);

  // set interest filter for delete command
  m_pibDeleteFilterId = registerSignedCommandProcessor(Name(pibPrefix).append(DeleteParam::VERB),
                                                       m_deleteProcessor);
}

template<class Processor>
const InterestFilterId*
Pib::registerProcessor(const Name& prefix, Processor& process)
{
  return m_face.setInterestFilter(prefix,
                                  [&] (const InterestFilter&, const Interest& interest) {
                                    processCommand(process, interest);
                                  });
}

template<class Processor>
const InterestFilterId*
Pib::registerSignedCommandProcessor(const Name& prefix, Processor& process)
{
  const InterestFilterId* filterId =
    m_face.setInterestFilter(prefix,
      [&] (const InterestFilter&, const Interest& interest) {
        m_validator.validate(interest,
                             [&] (const shared_ptr<const Interest>& interest) {
                               processCommand(process, *interest);
                             },
                             [] (const shared_ptr<const Interest>&, const string&) {});
      });

  return filterId;
}

template<class Processor>
void
Pib::processCommand(Processor& process, const Interest& interest)
{
  std::pair<bool, Block> result = process(interest);
  if (result.first)
    returnResult(Name(interest.getName()).appendVersion(),
                 result.second);
}

void
Pib::returnResult(const Name& dataName, const Block& content)
{
  shared_ptr<Data> data = make_shared<Data>(dataName);

  data->setFreshnessPeriod(time::milliseconds::zero());
  data->setContent(content);
  signWithDigestSha256(*data);

  // Put data into response cache
  m_responseCache.insert(*data);

  // Put data to face.
  m_face.put(*data);
}

} // namespace pib
} // namespace ndn
