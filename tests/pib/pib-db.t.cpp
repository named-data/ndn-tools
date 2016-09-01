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

#include "tools/pib/pib-db.hpp"

#include "tests/identity-management-fixture.hpp"

#include <boost/filesystem.hpp>

namespace ndn {
namespace pib {
namespace tests {

class PibDbTestFixture : public ndn::tests::IdentityManagementTimeFixture
{
public:
  PibDbTestFixture()
    : tmpPath(boost::filesystem::path(TMP_TESTS_PATH) / "DbTest")
    , db(tmpPath.c_str())
  {
  }

  ~PibDbTestFixture()
  {
    boost::filesystem::remove_all(tmpPath);
  }

  boost::asio::io_service io;
  boost::filesystem::path tmpPath;
  PibDb db;
  std::vector<Name> deletedIds;
  std::vector<Name> deletedKeys;
  std::vector<Name> deletedCerts;
  std::vector<Name> insertedCerts;
};


BOOST_AUTO_TEST_SUITE(Pib)
BOOST_FIXTURE_TEST_SUITE(TestPibDb, PibDbTestFixture)

BOOST_AUTO_TEST_CASE(MgmtTest)
{
  Name testUser("/localhost/pib/test/mgmt");
  addIdentity(testUser);
  Name testUserCertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser);
  shared_ptr<IdentityCertificate> testUserCert = m_keyChain.getCertificate(testUserCertName);


  BOOST_CHECK_EQUAL(db.getOwnerName(), "");
  BOOST_CHECK(db.getMgmtCertificate() == nullptr);

  db.updateMgmtCertificate(*testUserCert);
  BOOST_CHECK_EQUAL(db.getOwnerName(), "test");
  BOOST_REQUIRE(db.getMgmtCertificate() != nullptr);
  BOOST_CHECK_EQUAL(db.getMgmtCertificate()->getName(), testUserCertName);

  db.setTpmLocator("tpmLocator");
  BOOST_CHECK_EQUAL(db.getTpmLocator(), "tpmLocator");

  Name testUser2("/localhost/pib/test2/mgmt");
  addIdentity(testUser2);
  Name testUser2CertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser2);
  shared_ptr<IdentityCertificate> testUser2Cert = m_keyChain.getCertificate(testUser2CertName);

  BOOST_CHECK_THROW(db.updateMgmtCertificate(*testUser2Cert), PibDb::Error);

  Name testUserKeyName2 = m_keyChain.generateRsaKeyPairAsDefault(testUser);
  shared_ptr<IdentityCertificate> testUserCert2 = m_keyChain.selfSign(testUserKeyName2);

  BOOST_CHECK_NO_THROW(db.updateMgmtCertificate(*testUserCert2));
  BOOST_REQUIRE(db.getMgmtCertificate() != nullptr);
  BOOST_CHECK_EQUAL(db.getMgmtCertificate()->getName(),
                    testUserCert2->getName());
}

BOOST_AUTO_TEST_CASE(IdentityTest)
{
  db.identityDeleted.connect([this] (const Name& id) {
      this->deletedIds.push_back(id);
    });

  Name identity("/test/identity");
  Name identity2("/test/identity2");

  // Add an identity: /test/identity
  // Since there is no default identity,
  // the new added identity will be set as the default identity.
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), PibDb::NON_EXISTING_IDENTITY);
  db.addIdentity(identity);
  BOOST_CHECK(db.hasIdentity(identity));
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), identity);

  // Add the second identity: /test/identity2
  // Since the default identity exists,
  // the new added identity will not be set as the default identity.
  db.addIdentity(identity2);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity2), true);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), identity);

  // Set the second identity: /test/identity2 as default explicitly
  db.setDefaultIdentity(identity2);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), identity2);

  // Delete identity /test/identity2, which is also the default one
  // This will trigger the identityDeleted signal
  // and also causes no default identity.
  db.deleteIdentity(identity2);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity2), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity), true);
  BOOST_CHECK_EQUAL(deletedIds.size(), 1);
  BOOST_CHECK_EQUAL(deletedIds[0], identity2);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), PibDb::NON_EXISTING_IDENTITY);
  deletedIds.clear();

  // Add the second identity back
  // Since there is no default identity (though another identity still exists),
  // the second identity will be set as default.
  db.addIdentity(identity2);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity2), true);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), identity2);

  // Delete identity /test/identity
  db.deleteIdentity(identity);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity2), true);
  BOOST_CHECK_EQUAL(deletedIds.size(), 1);
  BOOST_CHECK_EQUAL(deletedIds[0], identity);
  deletedIds.clear();

  // Delete identity /test/identity2
  db.deleteIdentity(identity2);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(identity2), false);
  BOOST_CHECK_EQUAL(deletedIds.size(), 1);
  BOOST_CHECK_EQUAL(deletedIds[0], identity2);
  deletedIds.clear();
}


BOOST_AUTO_TEST_CASE(KeyTest)
{
  db.identityDeleted.connect([this] (const Name& id) {
      this->deletedIds.push_back(id);
    });

  db.keyDeleted.connect([this] (const Name& key) {
      this->deletedKeys.push_back(key);
    });

  // Initialize id1
  Name id1("/test/identity");
  addIdentity(id1);
  Name certName111 = m_keyChain.getDefaultCertificateNameForIdentity(id1);
  shared_ptr<IdentityCertificate> cert111 = m_keyChain.getCertificate(certName111);
  Name keyName11 = cert111->getPublicKeyName();
  PublicKey& key11 = cert111->getPublicKeyInfo();

  advanceClocks(io, time::milliseconds(100));
  Name keyName12 = m_keyChain.generateRsaKeyPairAsDefault(id1);
  shared_ptr<IdentityCertificate> cert121 = m_keyChain.selfSign(keyName12);
  PublicKey& key12 = cert121->getPublicKeyInfo();

  // Initialize id2
  advanceClocks(io, time::milliseconds(100));
  Name id2("/test/identity2");
  addIdentity(id2);
  Name certName211 = m_keyChain.getDefaultCertificateNameForIdentity(id2);
  shared_ptr<IdentityCertificate> cert211 = m_keyChain.getCertificate(certName211);
  Name keyName21 = cert211->getPublicKeyName();
  PublicKey& key21 = cert211->getPublicKeyInfo();

  advanceClocks(io, time::milliseconds(100));
  Name keyName22 = m_keyChain.generateRsaKeyPairAsDefault(id2);
  shared_ptr<IdentityCertificate> cert221 = m_keyChain.selfSign(keyName22);
  PublicKey& key22 = cert221->getPublicKeyInfo();

  // Add a key, the corresponding identity should be added as well
  // Since the PIB does not have any default identity set before,
  // the added identity will be set as default.
  // Since there is no default key for the identity,
  // the added key will be set as default.
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), PibDb::NON_EXISTING_KEY);
  BOOST_CHECK(db.getKey(keyName11) == nullptr);
  db.addKey(keyName11, key11);
  BOOST_CHECK(db.hasIdentity(id1));
  BOOST_CHECK(db.getKey(keyName11) != nullptr);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), id1);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), keyName11);

  // Add the second key of /test/identity.
  // Since the default key of /test/identity has been set,
  // The new added key will not be set as default.
  db.addKey(keyName12, key12);
  BOOST_CHECK(db.getKey(keyName12) != nullptr);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), keyName11);

  // Explicitly set the second key as the default key of /test/identity
  db.setDefaultKeyNameOfIdentity(keyName12);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), keyName12);

  // Delete the second key which is also the default key.
  // This will trigger the keyDeleted signal.
  // This will also cause no default key for /test/identity
  db.deleteKey(keyName12);
  BOOST_CHECK_EQUAL(db.hasKey(keyName12), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName22), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName21), false);
  BOOST_CHECK_EQUAL(deletedKeys.size(), 1);
  BOOST_CHECK_EQUAL(deletedKeys[0], keyName12);
  deletedKeys.clear();

  // Add the second key back.
  // Since there is no default key of /test/identity (although another key still exists)
  // The second key will be set as the default key of /test/identity
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), PibDb::NON_EXISTING_KEY);
  db.addKey(keyName12, key12);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), keyName12);

  // Prepare test for identity deletion
  db.addKey(keyName21, key21);
  db.addKey(keyName22, key22);
  BOOST_CHECK_EQUAL(db.hasKey(keyName12), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName22), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName21), true);
  BOOST_CHECK_EQUAL(db.hasIdentity(id2), true);

  // Delete the identity.
  // All keys of the identity should also be deleted,
  // and the keyDeleted signal should be triggered twice.
  db.deleteIdentity(id1);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(id2), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName12), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName21), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName22), true);
  BOOST_CHECK_EQUAL(deletedKeys.size(), 2);
  BOOST_CHECK(std::find(deletedKeys.begin(), deletedKeys.end(), keyName11) !=
              deletedKeys.end());
  BOOST_CHECK(std::find(deletedKeys.begin(), deletedKeys.end(), keyName12) !=
              deletedKeys.end());
  BOOST_CHECK_EQUAL(deletedIds.size(), 1);
  BOOST_CHECK_EQUAL(deletedIds[0], id1);
}

BOOST_AUTO_TEST_CASE(CertTest)
{
  db.identityDeleted.connect([this] (const Name& id) {
      this->deletedIds.push_back(id);
    });

  db.keyDeleted.connect([this] (const Name& key) {
      this->deletedKeys.push_back(key);
    });

  db.certificateDeleted.connect([this] (const Name& certificate) {
      this->deletedCerts.push_back(certificate);
    });

  db.certificateInserted.connect([this] (const Name& certificate) {
      this->insertedCerts.push_back(certificate);
    });

  // Initialize id1
  Name id1("/test/identity");
  addIdentity(id1);
  Name certName111 = m_keyChain.getDefaultCertificateNameForIdentity(id1);
  shared_ptr<IdentityCertificate> cert111 = m_keyChain.getCertificate(certName111);
  Name keyName11 = cert111->getPublicKeyName();

  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert112 = m_keyChain.selfSign(keyName11);
  Name certName112 = cert112->getName();

  advanceClocks(io, time::milliseconds(100));
  Name keyName12 = m_keyChain.generateRsaKeyPairAsDefault(id1);
  shared_ptr<IdentityCertificate> cert121 = m_keyChain.selfSign(keyName12);
  Name certName121 = cert121->getName();

  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert122 = m_keyChain.selfSign(keyName12);
  Name certName122 = cert122->getName();

  // Initialize id2
  advanceClocks(io, time::milliseconds(100));
  Name id2("/test/identity2");
  addIdentity(id2);
  Name certName211 = m_keyChain.getDefaultCertificateNameForIdentity(id2);
  shared_ptr<IdentityCertificate> cert211 = m_keyChain.getCertificate(certName211);
  Name keyName21 = cert211->getPublicKeyName();

  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert212 = m_keyChain.selfSign(keyName21);
  Name certName212 = cert212->getName();

  advanceClocks(io, time::milliseconds(100));
  Name keyName22 = m_keyChain.generateRsaKeyPairAsDefault(id2);
  shared_ptr<IdentityCertificate> cert221 = m_keyChain.selfSign(keyName22);
  Name certName221 = cert221->getName();

  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert222 = m_keyChain.selfSign(keyName22);
  Name certName222 = cert222->getName();

  // Add a certificate
  // This will also add the corresponding key and identity.
  // Since there is no default setting before,
  // The certificate will be set as the default one of the key, and so be the key and identity
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), false);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), PibDb::NON_EXISTING_CERTIFICATE);
  db.addCertificate(*cert111);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), true);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), true);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), id1);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id1), keyName11);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), certName111);
  BOOST_CHECK_EQUAL(insertedCerts.size(), 1);
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName111) !=
              insertedCerts.end());
  insertedCerts.clear();

  // Add the second certificate of the same key
  // Since default certificate already exists, no default setting changes.
  BOOST_CHECK(db.getCertificate(certName112) == nullptr);
  db.addCertificate(*cert112);
  BOOST_CHECK(db.getCertificate(certName112) != nullptr);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), certName111);
  BOOST_CHECK_EQUAL(insertedCerts.size(), 1);
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName112) !=
              insertedCerts.end());
  insertedCerts.clear();

  // Explicitly set the second certificate as the default one of the key.
  db.setDefaultCertNameOfKey(certName112);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), certName112);

  // Delete the default certificate
  // This will trigger certificateDeleted signal
  // and also causes no default certificate for the key.
  db.deleteCertificate(certName112);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName112), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), true);
  BOOST_CHECK_EQUAL(deletedCerts.size(), 1);
  BOOST_CHECK_EQUAL(deletedCerts[0], certName112);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), PibDb::NON_EXISTING_CERTIFICATE);
  deletedCerts.clear();

  // Add the second certificate back
  // Since there is no default certificate of the key (though another certificate still exists),
  // the new added certificate will be set as default
  db.addCertificate(*cert112);
  BOOST_CHECK(db.getCertificate(certName112) != nullptr);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName11), certName112);
  insertedCerts.clear();

  // Add entries for delete tests
  db.addCertificate(*cert111); // already exists no certInserted signal emitted
  db.addCertificate(*cert112); // already exists no certInserted signal emitted
  db.addCertificate(*cert121);
  db.addCertificate(*cert122);
  db.addCertificate(*cert211);
  db.addCertificate(*cert212);
  db.addCertificate(*cert221);
  db.addCertificate(*cert222);
  BOOST_CHECK_EQUAL(insertedCerts.size(), 6);
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName121) !=
              insertedCerts.end());
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName122) !=
              insertedCerts.end());
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName211) !=
              insertedCerts.end());
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName212) !=
              insertedCerts.end());
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName221) !=
              insertedCerts.end());
  BOOST_CHECK(std::find(insertedCerts.begin(), insertedCerts.end(), certName222) !=
              insertedCerts.end());
  insertedCerts.clear();

  // Delete the key.
  // All the related certificates will be deleted as well.
  db.deleteKey(keyName11);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName112), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName122), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName121), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName212), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName211), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName222), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName221), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName12), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName21), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName22), true);
  BOOST_CHECK_EQUAL(deletedCerts.size(), 2);
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName111) !=
              deletedCerts.end());
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName112) !=
              deletedCerts.end());
  BOOST_CHECK_EQUAL(deletedKeys.size(), 1);
  BOOST_CHECK_EQUAL(deletedKeys[0], keyName11);
  deletedCerts.clear();
  deletedKeys.clear();

  // Recover deleted entries
  db.addCertificate(*cert111);
  db.addCertificate(*cert112);

  // Delete the identity
  // All the related certificates and keys will be deleted as well.
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName112), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), true);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), true);
  db.deleteIdentity(id1);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName112), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName111), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName122), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName121), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName212), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName211), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName222), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName221), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName12), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName21), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName22), true);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), false);
  BOOST_CHECK_EQUAL(db.hasIdentity(id2), true);
  BOOST_CHECK_EQUAL(deletedCerts.size(), 4);
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName111) !=
              deletedCerts.end());
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName112) !=
              deletedCerts.end());
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName121) !=
              deletedCerts.end());
  BOOST_CHECK(std::find(deletedCerts.begin(), deletedCerts.end(), certName122) !=
              deletedCerts.end());
  BOOST_CHECK_EQUAL(deletedKeys.size(), 2);
  BOOST_CHECK(std::find(deletedKeys.begin(), deletedKeys.end(), keyName11) !=
              deletedKeys.end());
  BOOST_CHECK(std::find(deletedKeys.begin(), deletedKeys.end(), keyName12) !=
              deletedCerts.end());
  BOOST_CHECK_EQUAL(deletedIds.size(), 1);
  BOOST_CHECK_EQUAL(deletedIds[0], id1);
}

BOOST_AUTO_TEST_SUITE_END() // TestPibDb
BOOST_AUTO_TEST_SUITE_END() // Pib

} // namespace tests
} // namespace pib
} // namespace ndn
