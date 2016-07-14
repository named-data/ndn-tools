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

#include "tools/pib/cert-publisher.hpp"
#include "../identity-management-time-fixture.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

#include <boost/filesystem.hpp>

#include "tests/test-common.hpp"

namespace ndn {
namespace pib {
namespace tests {

class CertPublisherFixture : public ndn::tests::IdentityManagementTimeFixture
{
public:
  CertPublisherFixture()
    : tmpPath(boost::filesystem::path(TMP_TESTS_PATH) / "DbTest")
    , db(tmpPath.c_str())
    , face(io, m_keyChain, {true, true})
  {
  }

  ~CertPublisherFixture()
  {
    boost::filesystem::remove_all(tmpPath);
  }

  boost::asio::io_service io;
  boost::filesystem::path tmpPath;
  PibDb db;
  util::DummyClientFace face;
};

BOOST_FIXTURE_TEST_SUITE(PibCertPublisher, CertPublisherFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  // Initialize id1
  Name id1("/test/identity");
  addIdentity(id1);
  Name certName111 = m_keyChain.getDefaultCertificateNameForIdentity(id1);
  shared_ptr<IdentityCertificate> cert111 = m_keyChain.getCertificate(certName111);
  Name keyName11 = cert111->getPublicKeyName();

  advanceClocks(io, time::milliseconds(100));
  shared_ptr<IdentityCertificate> cert112 = m_keyChain.selfSign(keyName11);
  Name certName112 = cert112->getName();

  CertPublisher certPublisher(face, db);

  // Add a certificate
  db.addCertificate(*cert111);
  advanceClocks(io, time::milliseconds(2), 50);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);
  auto interest111 = make_shared<Interest>(cert111->getName().getPrefix(-1));
  face.receive(*interest111);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert111->wireEncode());
  face.sentData.clear();

  // Add another certificate
  db.addCertificate(*cert112);
  advanceClocks(io, time::milliseconds(2), 50);

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);
  auto interest112 = make_shared<Interest>(cert112->getName().getPrefix(-1));
  face.receive(*interest112);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert111->wireEncode());
  face.sentData.clear();

  Exclude exclude;
  exclude.excludeOne(cert111->getName().get(-1));
  interest112->setExclude(exclude);
  face.receive(*interest112);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert112->wireEncode());
  face.sentData.clear();

  // delete a certificate
  db.deleteCertificate(certName112);
  face.receive(*interest112);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);

  face.receive(*interest111);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK(face.sentData[0].wireEncode() == cert111->wireEncode());
  face.sentData.clear();

  // delete another certificate
  db.deleteCertificate(certName111);
  face.receive(*interest112);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);

  face.receive(*interest111);
  advanceClocks(io, time::milliseconds(2), 50);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace pib
} // namespace ndn
