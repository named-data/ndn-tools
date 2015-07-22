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

#include "tools/pib/pib-validator.hpp"
#include "tools/pib/encoding/update-param.hpp"
#include "tools/pib/encoding/delete-param.hpp"
#include <ndn-cxx/security/key-chain.hpp>

#include "../identity-management-time-fixture.hpp"
#include <boost/filesystem.hpp>
#include "tests/test-common.hpp"

namespace ndn {
namespace pib {
namespace tests {

class PibValidatorFixture : public ndn::tests::IdentityManagementTimeFixture
{
public:
  PibValidatorFixture()
    : tmpPath(boost::filesystem::path(TMP_TESTS_PATH) / "DbTest")
    , db(tmpPath.c_str())
  {
  }

  ~PibValidatorFixture()
  {
    boost::filesystem::remove_all(tmpPath);
  }

  boost::asio::io_service io;
  boost::filesystem::path tmpPath;
  PibDb db;
  bool isProcessed;
};

BOOST_FIXTURE_TEST_SUITE(PibPibValidator, PibValidatorFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  PibValidator validator(db);

  Name testUser("/localhost/pib/test/mgmt");
  BOOST_REQUIRE(addIdentity(testUser, RsaKeyParams()));
  Name testUserCertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser);
  shared_ptr<IdentityCertificate> testUserCert = m_keyChain.getCertificate(testUserCertName);

  advanceClocks(io, time::milliseconds(100));
  Name testUser2("/localhost/pib/test2/mgmt");
  BOOST_REQUIRE(addIdentity(testUser2, RsaKeyParams()));

  db.updateMgmtCertificate(*testUserCert);

  advanceClocks(io, time::milliseconds(100));
  Name normalId("/normal/id");
  BOOST_REQUIRE(addIdentity(normalId, RsaKeyParams()));
  Name normalIdCertName = m_keyChain.getDefaultCertificateNameForIdentity(normalId);
  shared_ptr<IdentityCertificate> normalIdCert = m_keyChain.getCertificate(normalIdCertName);

  db.addIdentity(normalId);
  db.addKey(normalIdCert->getPublicKeyName(), normalIdCert->getPublicKeyInfo());
  db.addCertificate(*normalIdCert);

  Name command1("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest1 = make_shared<Interest>(command1);
  m_keyChain.signByIdentity(*interest1, testUser);
  // "test" user is trusted for any command about itself, OK.
  isProcessed = false;
  validator.validate(*interest1,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(true);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(false);
    });
  BOOST_CHECK(isProcessed);

  Name command2("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest2 = make_shared<Interest>(command2);
  m_keyChain.signByIdentity(*interest2, testUser2);
  // "test2" user is NOT trusted for any command about other user, MUST fail
  isProcessed = false;
  validator.validate(*interest2,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(false);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(true);
    });
  BOOST_CHECK(isProcessed);

  Name command3("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest3 = make_shared<Interest>(command3);
  m_keyChain.signByIdentity(*interest3, normalId);
  // "normalId" is in "test" pib, can be trusted for some commands about "test".
  // Detail checking is needed, but it is not the job of Validator, OK.
  isProcessed = false;
  validator.validate(*interest3,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(true);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(false);
    });
  BOOST_CHECK(isProcessed);

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace pib
} // namespace ndn
