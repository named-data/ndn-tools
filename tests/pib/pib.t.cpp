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

#include "tools/pib/pib.hpp"
#include "tools/pib/encoding/pib-encoding.hpp"

#include "tests/identity-management-fixture.hpp"

#include <ndn-cxx/security/sec-tpm-file.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <boost/filesystem.hpp>

namespace ndn {
namespace pib {
namespace tests {

class PibTestFixture : public ndn::tests::IdentityManagementTimeFixture
{
public:
  PibTestFixture()
    : tmpPath(boost::filesystem::path(TMP_TESTS_PATH) / "PibTest")
    , face(io, m_keyChain, {true, true})
  {
  }

  ~PibTestFixture()
  {
    boost::filesystem::remove_all(tmpPath);
  }

  template<class Param>
  shared_ptr<Interest>
  generateUnsignedInterest(Param& param, const std::string& user)
  {
    Name command("/localhost/pib");
    command.append(user).append(Param::VERB).append(param.wireEncode());
    shared_ptr<Interest> interest = make_shared<Interest>(command);

    return interest;
  }

  template<class Param>
  shared_ptr<Interest>
  generateSignedInterest(Param& param, const std::string& user, const Name& certName)
  {
    shared_ptr<Interest> interest = generateUnsignedInterest(param, user);
    m_keyChain.sign(*interest, certName);

    return interest;
  }

  boost::asio::io_service io;
  std::string owner;
  boost::filesystem::path tmpPath;
  util::DummyClientFace face;
};

BOOST_AUTO_TEST_SUITE(Pib)
BOOST_FIXTURE_TEST_SUITE(TestPib, PibTestFixture)

using ndn::pib::Pib;

BOOST_AUTO_TEST_CASE(InitCertTest1)
{
  // Create a PIB with full parameters
  owner = "testUser";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK_EQUAL(pib.getOwner(), owner);
  BOOST_CHECK_EQUAL(pib.getDb().getOwnerName(), owner);

  auto mgmtCert = pib.getMgmtCert();
  BOOST_CHECK_EQUAL(mgmtCert.getName().getPrefix(-3),
                    Name("/localhost/pib/testUser/mgmt/KEY"));
  BOOST_CHECK_EQUAL(mgmtCert.getName().get(5).toUri().substr(0, 4), "dsk-");

  auto mgmtCert2 = pib.getDb().getMgmtCertificate();
  BOOST_REQUIRE(mgmtCert2 != nullptr);
  BOOST_CHECK(mgmtCert.wireEncode() == mgmtCert2->wireEncode());

  BOOST_CHECK_EQUAL(pib.getDb().getTpmLocator(), m_keyChain.getTpm().getTpmLocator());

  GetParam param01;
  shared_ptr<Interest> interest01 = generateUnsignedInterest(param01, owner);

  face.receive(*interest01);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibUser result01;
  BOOST_REQUIRE_NO_THROW(result01.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK(result01.getMgmtCert().wireEncode() == mgmtCert.wireEncode());
  BOOST_CHECK_EQUAL(result01.getTpmLocator(), m_keyChain.getTpm().getTpmLocator());
}

BOOST_AUTO_TEST_CASE(InitCertTest2)
{
  // Create a PIB from a database (assume that the database is configured)
  std::string dbDir = tmpPath.string();
  std::string tpmLocator = m_keyChain.getTpm().getTpmLocator();
  owner = "testUser";

  Name testUser("/localhost/pib/testUser/mgmt");

  addIdentity(testUser);
  Name testUserCertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser);
  shared_ptr<IdentityCertificate> testUserCert = m_keyChain.getCertificate(testUserCertName);

  PibDb db(tmpPath.string());
  BOOST_CHECK_NO_THROW(Pib(face, dbDir, tpmLocator, owner));

  db.updateMgmtCertificate(*testUserCert);
  BOOST_CHECK_NO_THROW(Pib(face, dbDir, tpmLocator, owner));
  BOOST_CHECK_THROW(Pib(face, dbDir, tpmLocator, "wrongUser"), Pib::Error);

  db.setTpmLocator(m_keyChain.getTpm().getTpmLocator());
  BOOST_CHECK_NO_THROW(Pib(face, dbDir, tpmLocator, owner));
  BOOST_CHECK_THROW(Pib(face, dbDir, "tpm-file:wrong", owner), Pib::Error);

  advanceClocks(io, time::milliseconds(10));
  m_keyChain.deleteIdentity(testUser);
  BOOST_CHECK_NO_THROW(Pib(face, dbDir, tpmLocator, owner));
}

BOOST_AUTO_TEST_CASE(InitCertTest3)
{
  std::string dbDir = tmpPath.string();
  std::string tpmLocator = m_keyChain.getTpm().getTpmLocator();
  owner = "testUser";

  Name testUser("/localhost/pib/testUser/mgmt");
  addIdentity(testUser);
  Name testUserCertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser);
  shared_ptr<IdentityCertificate> testUserCert = m_keyChain.getCertificate(testUserCertName);

  Pib pib1(face, dbDir, tpmLocator, owner);
  BOOST_CHECK_EQUAL(pib1.getMgmtCert().getName().getPrefix(-3),
                    Name("/localhost/pib/testUser/mgmt/KEY"));

  PibDb db(tmpPath.string());
  db.updateMgmtCertificate(*testUserCert);
  Pib pib2(face, dbDir, tpmLocator, owner);
  BOOST_CHECK_EQUAL(pib2.getMgmtCert().getName(), testUserCertName);

  advanceClocks(io, time::milliseconds(10));
  m_keyChain.deleteIdentity(testUser);
  Pib pib3(face, dbDir, tpmLocator, owner);
  BOOST_CHECK(pib3.getMgmtCert().getName() != testUserCertName);
  BOOST_CHECK_EQUAL(pib3.getMgmtCert().getName().getPrefix(-3),
                    Name("/localhost/pib/testUser/mgmt/KEY"));
}

BOOST_AUTO_TEST_CASE(GetCommandTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb db(tmpPath.string());

  Name testId("/test/identity");
  Name testIdCertName00 = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> cert00 = m_keyChain.getCertificate(testIdCertName00);
  Name testIdKeyName0 = cert00->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert01 = m_keyChain.selfSign(testIdKeyName0);
  Name testIdCertName01 = cert01->getName();

  advanceClocks(io, time::milliseconds(100));
  Name testIdKeyName1 = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> cert10 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName10 = cert10->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert11 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName11 = cert11->getName();

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), false);

  db.addCertificate(*cert00);
  db.addCertificate(*cert01);
  db.addCertificate(*cert10);
  db.addCertificate(*cert11);
  db.setDefaultIdentity(testId);
  db.setDefaultKeyNameOfIdentity(testIdKeyName0);
  db.setDefaultCertNameOfKey(testIdCertName00);

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), true);

  // Get Param
  GetParam param01;
  shared_ptr<Interest> interest01 = generateUnsignedInterest(param01, owner);

  face.sentData.clear();
  face.receive(*interest01);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest01->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibUser result01;
  BOOST_REQUIRE_NO_THROW(result01.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK(result01.getMgmtCert().wireEncode() == ownerMgmtCert.wireEncode());


  GetParam param02;
  shared_ptr<Interest> interest02 = generateUnsignedInterest(param02, "non-existing");

  face.sentData.clear();
  face.receive(*interest02);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest02->getName()) == nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);


  GetParam param03(TYPE_ID, testId);
  shared_ptr<Interest> interest03 = generateUnsignedInterest(param03, owner);

  face.sentData.clear();
  face.receive(*interest03);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest03->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibIdentity result03;
  BOOST_REQUIRE_NO_THROW(result03.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result03.getIdentity(), testId);


  Name wrongId("/wrong/id");
  GetParam param04(TYPE_ID, wrongId);
  shared_ptr<Interest> interest04 = generateUnsignedInterest(param04, owner);

  face.sentData.clear();
  face.receive(*interest04);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest04->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result04;
  BOOST_REQUIRE_NO_THROW(result04.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result04.getErrorCode(), ERR_NON_EXISTING_ID);


  GetParam param05(TYPE_KEY, testIdKeyName1);
  shared_ptr<Interest> interest05 = generateUnsignedInterest(param05, owner);

  face.sentData.clear();
  face.receive(*interest05);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest05->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibPublicKey result05;
  BOOST_REQUIRE_NO_THROW(result05.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result05.getKeyName(), testIdKeyName1);


  Name wrongKeyName1("/wrong/key/name1");
  GetParam param06(TYPE_KEY, wrongKeyName1);
  shared_ptr<Interest> interest06 = generateUnsignedInterest(param06, owner);

  face.sentData.clear();
  face.receive(*interest06);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest06->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result06;
  BOOST_REQUIRE_NO_THROW(result06.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result06.getErrorCode(), ERR_NON_EXISTING_KEY);


  GetParam param07(TYPE_CERT, testIdCertName00);
  shared_ptr<Interest> interest07 = generateUnsignedInterest(param07, owner);

  face.sentData.clear();
  face.receive(*interest07);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest07->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibCertificate result07;
  BOOST_REQUIRE_NO_THROW(result07.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result07.getCertificate().getName(), testIdCertName00);


  Name wrongCertName1("/wrong/cert/name1");
  GetParam param08(TYPE_CERT, wrongCertName1);
  shared_ptr<Interest> interest08 = generateUnsignedInterest(param08, owner);

  face.sentData.clear();
  face.receive(*interest08);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest08->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result08;
  BOOST_REQUIRE_NO_THROW(result08.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result08.getErrorCode(), ERR_NON_EXISTING_CERT);


  Name wrongKeyName2;
  GetParam param09(TYPE_KEY, wrongKeyName2);
  shared_ptr<Interest> interest09 = generateUnsignedInterest(param09, owner);

  face.sentData.clear();
  face.receive(*interest09);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest09->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result09;
  BOOST_REQUIRE_NO_THROW(result09.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result09.getErrorCode(), ERR_WRONG_PARAM);
}

BOOST_AUTO_TEST_CASE(DefaultCommandTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb db(tmpPath.string());

  Name testId("/test/identity");
  Name testIdCertName00 = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> cert00 = m_keyChain.getCertificate(testIdCertName00);
  Name testIdKeyName0 = cert00->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert01 = m_keyChain.selfSign(testIdKeyName0);
  Name testIdCertName01 = cert01->getName();

  advanceClocks(io, time::milliseconds(100));
  Name testIdKeyName1 = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> cert10 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName10 = cert10->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert11 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName11 = cert11->getName();

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), false);

  db.addCertificate(*cert00);
  db.addCertificate(*cert01);
  db.addCertificate(*cert10);
  db.addCertificate(*cert11);
  db.setDefaultIdentity(testId);
  db.setDefaultKeyNameOfIdentity(testIdKeyName0);
  db.setDefaultCertNameOfKey(testIdCertName00);

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), true);

  // Default Param
  DefaultParam param11(TYPE_ID, TYPE_USER);
  shared_ptr<Interest> interest11 = generateUnsignedInterest(param11, owner);

  face.sentData.clear();
  face.receive(*interest11);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest11->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibIdentity result11;
  BOOST_REQUIRE_NO_THROW(result11.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result11.getIdentity(), testId);


  DefaultParam param13(TYPE_ID, TYPE_ID);
  shared_ptr<Interest> interest13 = generateUnsignedInterest(param13, owner);

  face.sentData.clear();
  face.receive(*interest13);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest13->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result13;
  BOOST_REQUIRE_NO_THROW(result13.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result13.getErrorCode(), ERR_WRONG_PARAM);


  DefaultParam param14(TYPE_KEY, TYPE_ID, testId);
  shared_ptr<Interest> interest14 = generateUnsignedInterest(param14, owner);

  face.sentData.clear();
  face.receive(*interest14);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest14->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibPublicKey result14;
  BOOST_REQUIRE_NO_THROW(result14.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result14.getKeyName(), testIdKeyName0);


  DefaultParam param15(TYPE_CERT, TYPE_ID, testId);
  shared_ptr<Interest> interest15 = generateUnsignedInterest(param15, owner);

  face.sentData.clear();
  face.receive(*interest15);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest15->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibCertificate result15;
  BOOST_REQUIRE_NO_THROW(result15.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result15.getCertificate().getName(), testIdCertName00);


  DefaultParam param16(TYPE_CERT, TYPE_USER);
  shared_ptr<Interest> interest16 = generateUnsignedInterest(param16, owner);

  face.sentData.clear();
  face.receive(*interest16);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest16->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibCertificate result16;
  BOOST_REQUIRE_NO_THROW(result16.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result16.getCertificate().getName(), testIdCertName00);


  DefaultParam param17(TYPE_CERT, TYPE_KEY, testIdKeyName1);
  shared_ptr<Interest> interest17 = generateUnsignedInterest(param17, owner);

  face.sentData.clear();
  face.receive(*interest17);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest17->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibCertificate result17;
  BOOST_REQUIRE_NO_THROW(result17.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result17.getCertificate().getName(), testIdCertName10);
}

BOOST_AUTO_TEST_CASE(ListCommandTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb db(tmpPath.string());

  Name testId("/test/identity");
  Name testIdCertName00 = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> cert00 = m_keyChain.getCertificate(testIdCertName00);
  Name testIdKeyName0 = cert00->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert01 = m_keyChain.selfSign(testIdKeyName0);
  Name testIdCertName01 = cert01->getName();

  advanceClocks(io, time::milliseconds(100));
  Name testIdKeyName1 = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> cert10 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName10 = cert10->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert11 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName11 = cert11->getName();

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), false);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), false);

  db.addCertificate(*cert00);
  db.addCertificate(*cert01);
  db.addCertificate(*cert10);
  db.addCertificate(*cert11);
  db.setDefaultIdentity(testId);
  db.setDefaultKeyNameOfIdentity(testIdKeyName0);
  db.setDefaultCertNameOfKey(testIdCertName00);

  BOOST_CHECK_EQUAL(db.hasIdentity(testId), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName0), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName00), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName01), true);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName10), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), true);

  Name wrongId("/wrong/id");

  // List Param
  ListParam param21;
  shared_ptr<Interest> interest21 = generateUnsignedInterest(param21, owner);

  face.sentData.clear();
  face.receive(*interest21);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest21->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibNameList result21;
  BOOST_REQUIRE_NO_THROW(result21.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result21.getNameList().size(), 1);


  ListParam param22(TYPE_ID, testId);
  shared_ptr<Interest> interest22 = generateUnsignedInterest(param22, owner);

  face.sentData.clear();
  face.receive(*interest22);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest22->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibNameList result22;
  BOOST_REQUIRE_NO_THROW(result22.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result22.getNameList().size(), 2);


  ListParam param23(TYPE_ID, wrongId);
  shared_ptr<Interest> interest23 = generateUnsignedInterest(param23, owner);

  face.sentData.clear();
  face.receive(*interest23);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest23->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibNameList result23;
  BOOST_REQUIRE_NO_THROW(result23.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result23.getNameList().size(), 0);
}

BOOST_AUTO_TEST_CASE(IsUpdateAllowedTest1)
{
  // This test case is to check the access control of local management key
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);

  UpdateQueryProcessor& pro = pib.m_updateProcessor;

  Name target01("/localhost/pib");
  Name target02("/localhost/pib/alice/mgmt");
  Name target03("/localhost/pib/alice/mgmt/ok");
  Name target04("/localhost/pib/alice");
  Name target05("/test/id");
  Name target06("/test/id/ksk-123");
  Name target07("/test/id/KEY/ksk-123/ID-CERT/version");
  Name signer01 = pib.getMgmtCert().getName().getPrefix(-1);
  Name signer02("/localhost/pib/bob/mgmt/KEY/ksk-1234/ID-CERT");

  // TYPE_USER is handled separately, isUpdatedAllowed simply returns false
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_USER, target02, signer01, DEFAULT_OPT_NO), false);

  // Test access control of local management key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target01, signer01, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target02, signer01, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target03, signer01, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target04, signer01, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target05, signer01, DEFAULT_OPT_NO), true);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, target05, signer02, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, target06, signer01, DEFAULT_OPT_NO), true);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, target06, signer02, DEFAULT_OPT_NO), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, target07, signer01, DEFAULT_OPT_NO), true);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, target07, signer02, DEFAULT_OPT_NO), false);
}

BOOST_AUTO_TEST_CASE(IsUpdateAllowedTest2)
{
  // This test case is to check the access control of regular key

  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  PibDb db(tmpPath.string());

  UpdateQueryProcessor& pro = pib.m_updateProcessor;

  Name parent("/test");
  addIdentity(parent);
  Name parentCertName = m_keyChain.getDefaultCertificateNameForIdentity(parent);
  shared_ptr<IdentityCertificate> parentCert = m_keyChain.getCertificate(parentCertName);
  Name parentSigner = parentCertName.getPrefix(-1);

  advanceClocks(io, time::milliseconds(100));
  Name parentKeyName2 = m_keyChain.generateRsaKeyPair(parent);
  shared_ptr<IdentityCertificate> parentCert2 = m_keyChain.selfSign(parentKeyName2);
  Name parentSigner2 = parentCert2->getName().getPrefix(-1);

  db.addIdentity(parent);
  db.addKey(parentCert->getPublicKeyName(), parentCert->getPublicKeyInfo());
  db.addKey(parentCert2->getPublicKeyName(), parentCert2->getPublicKeyInfo());
  db.setDefaultKeyNameOfIdentity(parentCert->getPublicKeyName());
  db.addCertificate(*parentCert);
  db.setDefaultCertNameOfKey(parentCert->getName());
  db.addCertificate(*parentCert2);
  db.setDefaultCertNameOfKey(parentCert2->getName());

  Name testId("/test/id");
  addIdentity(testId);
  Name certName = m_keyChain.getDefaultCertificateNameForIdentity(testId);
  shared_ptr<IdentityCertificate> testCert = m_keyChain.getCertificate(certName);
  Name testKeyName = testCert->getPublicKeyName();
  Name testSigner = certName.getPrefix(-1);

  advanceClocks(io, time::milliseconds(100));
  Name secondKeyName = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> secondCert = m_keyChain.selfSign(secondKeyName);
  Name secondCertName = secondCert->getName();
  Name secondSigner = secondCertName.getPrefix(-1);

  db.addIdentity(testId);
  db.addKey(testKeyName, testCert->getPublicKeyInfo());
  db.addKey(secondKeyName, secondCert->getPublicKeyInfo());
  db.setDefaultKeyNameOfIdentity(testKeyName);
  db.addCertificate(*testCert);
  db.setDefaultCertNameOfKey(testCert->getName());
  db.addCertificate(*secondCert);
  db.setDefaultCertNameOfKey(secondCert->getName());

  Name nonSigner("/non-signer/KEY/ksk-123/ID-CERT");

  // for target type = TYPE_ID
  // one cannot add non-child
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, nonSigner, DEFAULT_OPT_NO), false);
  // parent can add child
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, parentSigner, DEFAULT_OPT_NO), true);
  // non-default parent key cannot add a child
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, parentSigner2, DEFAULT_OPT_NO), false);
  // only DEFAULT_OPT_NO is allowed if target type is TYPE_ID
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, parentSigner, DEFAULT_OPT_ID), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, parentSigner, DEFAULT_OPT_KEY), false);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_ID, testId, parentSigner, DEFAULT_OPT_USER), false);

  // for target type = TYPE_KEY
  // one can add its own key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, testSigner, DEFAULT_OPT_NO),
                    true);
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, secondKeyName, testSigner, DEFAULT_OPT_NO),
                    true);
  // one can set its default key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, testSigner, DEFAULT_OPT_ID),
                    true);
  // non-default key cannot add its own key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, secondKeyName, secondSigner, DEFAULT_OPT_NO),
                    false);
  // non-default key cannot set its default key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, secondSigner, DEFAULT_OPT_ID),
                    false);
  // one can add its child's key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, secondKeyName, parentSigner, DEFAULT_OPT_NO),
                    true);
  // one can set its child's default key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, parentSigner, DEFAULT_OPT_ID),
                    true);
  // non-default key cannot add its child's key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, secondKeyName, parentSigner2, DEFAULT_OPT_NO),
                    false);
  // non-default parent key cannot set its child's default key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, parentSigner2, DEFAULT_OPT_ID),
                    false);
  // DEFAULT_OPT_KEY is not allowed if target type is TYPE_KEY
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, testSigner, DEFAULT_OPT_KEY),
                    false);
  // DEFAULT_OPT_USER is not allowed if signer is no local management key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_KEY, testKeyName, testSigner, DEFAULT_OPT_USER),
                    false);

  // for target type = TYPE_CERT
  // one can add its own certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, testSigner, DEFAULT_OPT_NO),
                    true);
  // one can set its own default certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, testSigner, DEFAULT_OPT_ID),
                    true);
  // one can set its own key's default certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, testSigner, DEFAULT_OPT_KEY),
                    true);
  // DEFAULT_OPT_USER is not allowed if signer is no local management key
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, testSigner, DEFAULT_OPT_USER),
                    false);
  // non-default key can add other key's certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, secondSigner, DEFAULT_OPT_NO),
                    false);
  // non-default key can add its own certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, secondCertName, secondSigner, DEFAULT_OPT_NO),
                    true);
  // one can add its child's certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, parentSigner, DEFAULT_OPT_NO),
                    true);
  // non-default key cannot add its child's certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, certName, parentSigner2, DEFAULT_OPT_NO),
                    false);
  // non-default key cannot set add its identity default certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, secondCertName, secondSigner, DEFAULT_OPT_ID),
                    false);
  // non-default key can set add its own default certificate
  BOOST_CHECK_EQUAL(pro.isUpdateAllowed(TYPE_CERT, secondCertName, secondSigner, DEFAULT_OPT_KEY),
                    true);
}

BOOST_AUTO_TEST_CASE(IsDeleteAllowedTest1)
{
  // This test case is to check the access control of local management key

  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);

  DeleteQueryProcessor& pro = pib.m_deleteProcessor;

  Name target01("/localhost/pib");
  Name target02("/localhost/pib/alice/Mgmt");
  Name target03("/localhost/pib/alice/Mgmt/ok");
  Name target04("/localhost/pib/alice");
  Name target05("/test/id");
  Name target06("/test/id/ksk-123");
  Name target07("/test/id/KEY/ksk-123/ID-CERT/version");
  Name signer01 = pib.getMgmtCert().getName().getPrefix(-1);
  Name signer02("/localhost/pib/bob/Mgmt/KEY/ksk-1234/ID-CERT");

  // TYPE_USER is handled separately
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_USER, target02, signer01), false);

  // Test access control of local management key
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target01, signer01), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target02, signer01), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target03, signer01), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target04, signer01), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target05, signer01), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, target06, signer01), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, target07, signer01), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, target05, signer02), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, target06, signer02), false);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, target07, signer02), false);
}

BOOST_AUTO_TEST_CASE(IsDeleteAllowedTest2)
{
  // This test case is to check the access control of regular key
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  PibDb db(tmpPath.string());
  DeleteQueryProcessor& pro = pib.m_deleteProcessor;

  Name parent("/test");
  addIdentity(parent);
  Name parentCertName = m_keyChain.getDefaultCertificateNameForIdentity(parent);
  shared_ptr<IdentityCertificate> parentCert = m_keyChain.getCertificate(parentCertName);
  Name parentSigner = parentCertName.getPrefix(-1);

  advanceClocks(io, time::milliseconds(100));
  Name parentKeyName2 = m_keyChain.generateRsaKeyPair(parent);
  shared_ptr<IdentityCertificate> parentCert2 = m_keyChain.selfSign(parentKeyName2);
  Name parentSigner2 = parentCert2->getName().getPrefix(-1);

  db.addIdentity(parent);
  db.addKey(parentCert->getPublicKeyName(), parentCert->getPublicKeyInfo());
  db.addKey(parentCert2->getPublicKeyName(), parentCert2->getPublicKeyInfo());
  db.setDefaultKeyNameOfIdentity(parentCert->getPublicKeyName());
  db.addCertificate(*parentCert);
  db.setDefaultCertNameOfKey(parentCert->getName());
  db.addCertificate(*parentCert2);
  db.setDefaultCertNameOfKey(parentCert2->getName());

  Name testId("/test/id");
  addIdentity(testId);
  Name certName = m_keyChain.getDefaultCertificateNameForIdentity(testId);
  shared_ptr<IdentityCertificate> testCert = m_keyChain.getCertificate(certName);
  Name testKeyName = testCert->getPublicKeyName();
  Name testSigner = certName.getPrefix(-1);

  advanceClocks(io, time::milliseconds(100));
  Name secondKeyName = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> secondCert = m_keyChain.selfSign(secondKeyName);
  Name secondCertName = secondCert->getName();
  Name secondSigner = secondCertName.getPrefix(-1);

  db.addIdentity(testId);
  db.addKey(testKeyName, testCert->getPublicKeyInfo());
  db.addKey(secondKeyName, secondCert->getPublicKeyInfo());
  db.setDefaultKeyNameOfIdentity(testKeyName);
  db.addCertificate(*testCert);
  db.setDefaultCertNameOfKey(testCert->getName());
  db.addCertificate(*secondCert);
  db.setDefaultCertNameOfKey(secondCert->getName());

  Name nonSigner("/non-signer/KEY/ksk-123/ID-CERT");

  // one can delete itself
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, testId, testSigner), true);
  // parent can delete its child
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, testId, parentSigner), true);
  // non-default key cannot delete its identity
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, testId, secondSigner), false);
  // non-default key cannot delete its child
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, testId, parentSigner2), false);
  // one cannot delete its parent
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_ID, parent, testSigner), false);

  // one can delete its own key
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, testKeyName, testSigner), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, secondKeyName, testSigner), true);
  // parent can delete its child's key
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, testKeyName, parentSigner), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, secondKeyName, parentSigner), true);
  // non-default key cannot delete other key
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, testKeyName, secondSigner), false);
  // non-default key can delete itself
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, secondKeyName, secondSigner), true);
  // non-default key cannot delete its child's key
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_KEY, testKeyName, parentSigner2), false);

  // one can delete its own certificate
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, certName, testSigner), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, secondCertName, testSigner), true);
  // non-default key cannot delete other's certificate
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, certName, secondSigner), false);
  // non-default key can delete its own certificate
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, secondCertName, secondSigner), true);
  // parent can delete its child's certificate
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, certName, parentSigner), true);
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, secondCertName, parentSigner), true);
  // non-default parent cannot delete its child's certificate
  BOOST_CHECK_EQUAL(pro.isDeleteAllowed(TYPE_CERT, certName, parentSigner2), false);
}


BOOST_AUTO_TEST_CASE(UpdateUserTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);

  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();

  m_keyChain.addCertificate(pib.getMgmtCert());

  PibDb db(tmpPath.string());

  Name bob("/localhost/pib/bob/mgmt");
  addIdentity(bob);
  Name bobCertName = m_keyChain.getDefaultCertificateNameForIdentity(bob);
  shared_ptr<IdentityCertificate> bobCert = m_keyChain.getCertificate(bobCertName);

  // signer is correct, but user name is wrong, should fall
  PibUser pibUser1;
  pibUser1.setMgmtCert(*bobCert);
  UpdateParam param1(pibUser1);
  auto interest1 = generateSignedInterest(param1, owner, db.getMgmtCertificate()->getName());

  face.sentData.clear();
  face.receive(*interest1);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest1->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result;
  BOOST_REQUIRE_NO_THROW(result.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result.getErrorCode(), ERR_WRONG_PARAM);

  // user name is correct, but signer is wrong, should fail
  PibUser pibUser2;
  pibUser2.setMgmtCert(pib.getMgmtCert());
  UpdateParam param2(pibUser2);
  auto interest2 = generateSignedInterest(param2, owner, bobCertName);

  face.sentData.clear();
  face.receive(*interest2);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest2->getName()) == nullptr); // verification should fail, no response
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);

  // update an existing user with a new mgmt key, signed by the old mgmt key.
  advanceClocks(io, time::milliseconds(100));
  Name ownerSecondKeyName =
    m_keyChain.generateRsaKeyPair(Name("/localhost/pib/alice/mgmt"), false);
  shared_ptr<IdentityCertificate> ownerSecondCert = m_keyChain.selfSign(ownerSecondKeyName);
  m_keyChain.addCertificate(*ownerSecondCert);

  PibUser pibUser3;
  pibUser3.setMgmtCert(*ownerSecondCert);
  UpdateParam param3(pibUser3);
  auto interest3 = generateSignedInterest(param3, owner, db.getMgmtCertificate()->getName());

  face.sentData.clear();
  face.receive(*interest3);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest3->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result3;
  BOOST_REQUIRE_NO_THROW(result3.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result3.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK(db.getMgmtCertificate()->wireEncode() == ownerSecondCert->wireEncode());

  // Add an cert and set it as user default cert.
  Name testId("/test/id");
  Name testIdCertName = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> testIdCert = m_keyChain.getCertificate(testIdCertName);
  Name testIdKeyName = testIdCert->getPublicKeyName();
  UpdateParam updateParam(*testIdCert, DEFAULT_OPT_USER);
  auto interest4 = generateSignedInterest(updateParam, owner, ownerSecondCert->getName());

  face.sentData.clear();
  face.receive(*interest4);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_REQUIRE(cache.find(interest4->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result4;
  BOOST_REQUIRE_NO_THROW(result4.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result4.getErrorCode(), ERR_SUCCESS);

  BOOST_CHECK(pib.getDb().hasCertificate(testIdCertName));
  BOOST_CHECK(pib.getDb().hasKey(testIdKeyName));
  BOOST_CHECK(pib.getDb().hasIdentity(testId));

  BOOST_REQUIRE_NO_THROW(pib.getDb().getDefaultCertNameOfKey(testIdKeyName));
  BOOST_REQUIRE_NO_THROW(pib.getDb().getDefaultKeyNameOfIdentity(testId));
  BOOST_REQUIRE_NO_THROW(pib.getDb().getDefaultIdentity());

  BOOST_CHECK_EQUAL(pib.getDb().getDefaultCertNameOfKey(testIdKeyName), testIdCertName);
  BOOST_CHECK_EQUAL(pib.getDb().getDefaultKeyNameOfIdentity(testId), testIdKeyName);
  BOOST_CHECK_EQUAL(pib.getDb().getDefaultIdentity(), testId);
}

BOOST_AUTO_TEST_CASE(UpdateRegularKeyTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);

  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb db(tmpPath.string());

  Name id0("/test/identity0");
  Name certName000 = m_keyChain.createIdentity(id0);
  shared_ptr<IdentityCertificate> cert000 = m_keyChain.getCertificate(certName000);
  Name keyName00 = cert000->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert001 = m_keyChain.selfSign(keyName00);
  Name certName001 = cert001->getName();

  advanceClocks(io, time::milliseconds(100));
  Name keyName01 = m_keyChain.generateRsaKeyPair(id0);
  shared_ptr<IdentityCertificate> cert010 = m_keyChain.selfSign(keyName01);
  Name certName010 = cert010->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert011 = m_keyChain.selfSign(keyName01);
  Name certName011 = cert011->getName();
  m_keyChain.addCertificate(*cert010);

  advanceClocks(io, time::milliseconds(100));
  Name id1("/test/identity1");
  Name certName100 = m_keyChain.createIdentity(id1);
  shared_ptr<IdentityCertificate> cert100 = m_keyChain.getCertificate(certName100);
  Name keyName10 = cert100->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert101 = m_keyChain.selfSign(keyName10);
  Name certName101 = cert101->getName();

  advanceClocks(io, time::milliseconds(100));
  Name keyName11 = m_keyChain.generateRsaKeyPair(id1);
  shared_ptr<IdentityCertificate> cert110 = m_keyChain.selfSign(keyName11);
  Name certName110 = cert110->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert111 = m_keyChain.selfSign(keyName11);
  Name certName111 = cert111->getName();
  m_keyChain.addCertificate(*cert111);


  // Add a cert
  BOOST_CHECK_EQUAL(db.hasIdentity(id0), false);
  BOOST_CHECK_EQUAL(db.hasKey(keyName00), false);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName000), false);
  UpdateParam param1(*cert000);
  auto interest1 = generateSignedInterest(param1, owner, ownerMgmtCert.getName());

  face.sentData.clear();
  face.receive(*interest1);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest1->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result1;
  BOOST_REQUIRE_NO_THROW(result1.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result1.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasIdentity(id0), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName00), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName000), true);

  db.addCertificate(*cert100);
  BOOST_CHECK_EQUAL(db.hasIdentity(id1), true);
  BOOST_CHECK_EQUAL(db.hasKey(keyName10), true);
  BOOST_CHECK_EQUAL(db.hasCertificate(certName100), true);

  // Set default
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), id0);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id0), keyName00);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName00), certName000);

  UpdateParam param2(id1, DEFAULT_OPT_USER);
  auto interest2 = generateSignedInterest(param2, owner, ownerMgmtCert.getName());

  face.sentData.clear();
  face.receive(*interest2);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest2->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result2;
  BOOST_REQUIRE_NO_THROW(result2.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result2.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.getDefaultIdentity(), id1);

  db.addCertificate(*cert010);
  UpdateParam param3(keyName01, cert010->getPublicKeyInfo(), DEFAULT_OPT_ID);
  auto interest3 = generateSignedInterest(param3, owner, ownerMgmtCert.getName());

  face.sentData.clear();
  face.receive(*interest3);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest3->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result3;
  BOOST_REQUIRE_NO_THROW(result3.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result3.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.getDefaultKeyNameOfIdentity(id0), keyName01);

  db.addCertificate(*cert011);
  UpdateParam param4(*cert011, DEFAULT_OPT_KEY);
  auto interest4 = generateSignedInterest(param4, owner, ownerMgmtCert.getName());

  face.sentData.clear();
  face.receive(*interest4);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest4->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result4;
  BOOST_REQUIRE_NO_THROW(result4.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result4.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.getDefaultCertNameOfKey(keyName01), certName011);

  // add key and certificate using regular keys.
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), false);
  UpdateParam param5(keyName11, cert110->getPublicKeyInfo());
  auto interest5 = generateSignedInterest(param5, owner, cert100->getName());

  face.sentData.clear();
  face.receive(*interest5);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest5->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result5;
  BOOST_REQUIRE_NO_THROW(result5.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result5.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasKey(keyName11), true);

  // add cert using its own key which has been added before
  BOOST_CHECK_EQUAL(db.hasCertificate(cert101->getName()), false);
  UpdateParam param6(*cert101);
  auto interest6 = generateSignedInterest(param6, owner, cert100->getName());

  face.sentData.clear();
  face.receive(*interest6);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest6->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result6;
  BOOST_REQUIRE_NO_THROW(result6.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result6.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasCertificate(cert101->getName()), true);
}

BOOST_AUTO_TEST_CASE(DeleteUserTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb db(tmpPath.string());

  // Delete user should fail
  DeleteParam param(Name(), TYPE_USER);
  auto interest = generateSignedInterest(param, owner, ownerMgmtCert.getName());

  face.receive(*interest);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result;
  BOOST_REQUIRE_NO_THROW(result.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result.getErrorCode(), ERR_WRONG_PARAM);
}

BOOST_AUTO_TEST_CASE(DeleteRegularKeyTest)
{
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);
  advanceClocks(io, time::milliseconds(10), 10);
  util::InMemoryStoragePersistent& cache = pib.getResponseCache();
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  PibDb& db = pib.getDb();

  Name testId("/test/identity");
  Name testIdCertName00 = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> cert00 = m_keyChain.getCertificate(testIdCertName00);
  Name testIdKeyName0 = cert00->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert01 = m_keyChain.selfSign(testIdKeyName0);
  Name testIdCertName01 = cert01->getName();

  advanceClocks(io, time::milliseconds(100));
  Name testIdKeyName1 = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> cert10 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName10 = cert10->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert11 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName11 = cert11->getName();
  m_keyChain.addCertificate(*cert11);

  db.addCertificate(*cert00);
  db.addCertificate(*cert01);
  db.addCertificate(*cert10);
  db.addCertificate(*cert11);
  db.setDefaultIdentity(testId);
  db.setDefaultKeyNameOfIdentity(testIdKeyName0);
  db.setDefaultCertNameOfKey(testIdCertName00);
  db.setDefaultCertNameOfKey(testIdCertName10);

  // delete a certificate itself
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), true);
  DeleteParam param1(testIdCertName11, TYPE_CERT);
  auto interest1 = generateSignedInterest(param1, owner, testIdCertName11);

  face.sentData.clear();
  face.receive(*interest1);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest1->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result1;
  BOOST_REQUIRE_NO_THROW(result1.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result1.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasCertificate(testIdCertName11), false);

  // delete a key itself
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), true);
  DeleteParam param2(testIdKeyName1, TYPE_KEY);
  auto interest2 = generateSignedInterest(param2, owner, testIdCertName11);

  face.sentData.clear();
  face.receive(*interest2);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest2->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result2;
  BOOST_REQUIRE_NO_THROW(result2.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result2.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasKey(testIdKeyName1), false);

  // delete an identity using non-default key, should fail
  db.addCertificate(*cert11);
  BOOST_CHECK_EQUAL(db.hasIdentity(testId), true);
  DeleteParam param3(testId, TYPE_ID);
  auto interest3 = generateSignedInterest(param3, owner, testIdCertName11);

  face.sentData.clear();
  face.receive(*interest3);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest3->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result3;
  BOOST_REQUIRE_NO_THROW(result3.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result3.getErrorCode(), ERR_WRONG_SIGNER);
  BOOST_CHECK_EQUAL(db.hasIdentity(testId), true);

  // delete an identity using identity default key, should succeed
  DeleteParam param4(testId, TYPE_ID);
  auto interest4 = generateSignedInterest(param4, owner, testIdCertName00);

  face.sentData.clear();
  face.receive(*interest4);
  advanceClocks(io, time::milliseconds(10), 10);

  BOOST_CHECK(cache.find(interest4->getName()) != nullptr);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  PibError result4;
  BOOST_REQUIRE_NO_THROW(result4.wireDecode(face.sentData[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(result4.getErrorCode(), ERR_SUCCESS);
  BOOST_CHECK_EQUAL(db.hasIdentity(testId), false);
}

BOOST_AUTO_TEST_CASE(ReadCommandTest2)
{
  // Read Certificates;
  owner = "alice";

  Pib pib(face,
          tmpPath.string(),
          m_keyChain.getTpm().getTpmLocator(),
          owner);

  advanceClocks(io, time::milliseconds(10), 100);
  auto ownerMgmtCert = pib.getMgmtCert();
  m_keyChain.addCertificate(ownerMgmtCert);

  Name testId("/test/identity");
  Name testIdCertName00 = m_keyChain.createIdentity(testId);
  shared_ptr<IdentityCertificate> cert00 = m_keyChain.getCertificate(testIdCertName00);
  Name testIdKeyName0 = cert00->getPublicKeyName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert01 = m_keyChain.selfSign(testIdKeyName0);
  Name testIdCertName01 = cert01->getName();

  advanceClocks(io, time::milliseconds(100));
  Name testIdKeyName1 = m_keyChain.generateRsaKeyPair(testId);
  shared_ptr<IdentityCertificate> cert10 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName10 = cert10->getName();
  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert11 = m_keyChain.selfSign(testIdKeyName1);
  Name testIdCertName11 = cert11->getName();


  UpdateParam param00(*cert00);
  UpdateParam param01(*cert01);
  UpdateParam param10(*cert10);
  UpdateParam param11(*cert11);
  auto interest00 = generateSignedInterest(param00, owner, ownerMgmtCert.getName());
  auto interest01 = generateSignedInterest(param01, owner, ownerMgmtCert.getName());
  auto interest10 = generateSignedInterest(param10, owner, ownerMgmtCert.getName());
  auto interest11 = generateSignedInterest(param11, owner, ownerMgmtCert.getName());

  face.sentData.clear();
  face.receive(*interest00);
  advanceClocks(io, time::milliseconds(10), 10);
  face.receive(*interest01);
  advanceClocks(io, time::milliseconds(10), 10);
  face.receive(*interest10);
  advanceClocks(io, time::milliseconds(10), 10);
  face.receive(*interest11);
  advanceClocks(io, time::milliseconds(10), 10);

  auto interest1 = make_shared<Interest>(testIdCertName11);
  face.sentData.clear();
  face.receive(*interest1);
  advanceClocks(io, time::milliseconds(10), 10);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert11->wireEncode());

  auto interest2 = make_shared<Interest>(testIdCertName11.getPrefix(-1));
  face.sentData.clear();
  face.receive(*interest2);
  advanceClocks(io, time::milliseconds(10), 10);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData[0].getName().getPrefix(-1),
                    cert11->getName().getPrefix(-1));

  auto interest3 = make_shared<Interest>(testIdCertName11.getPrefix(-1));
  pib.getDb().deleteCertificate(testIdCertName11);
  face.sentData.clear();
  face.receive(*interest3);
  advanceClocks(io, time::milliseconds(10), 10);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert10->wireEncode());

  auto interest4 = make_shared<Interest>(testIdCertName11);
  face.sentData.clear();
  face.receive(*interest4);
  advanceClocks(io, time::milliseconds(10), 10);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestPib
BOOST_AUTO_TEST_SUITE_END() // Pib

} // namespace tests
} // namespace pib
} // namespace ndn
