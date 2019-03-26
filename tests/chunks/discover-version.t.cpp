/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2019, Regents of the University of California,
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
 * @author Chavoosh Ghasemi
 */

#include "tools/chunks/catchunks/discover-version.hpp"

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"

#include <ndn-cxx/metadata-object.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace chunks {
namespace tests {

using namespace ndn::tests;

class DiscoverVersionFixture : public UnitTestTimeFixture,
                               public IdentityManagementFixture
{
public:
  DiscoverVersionFixture()
    : face(io)
    , name("/ndn/chunks/test")
    , isDiscoveryFinished(false)
    , version(1449227841747)
  {
    opt.interestLifetime = ndn::DEFAULT_INTEREST_LIFETIME;
    opt.maxRetriesOnTimeoutOrNack = 15;
    opt.isVerbose = false;
  }

  void
  run(const Name& prefix)
  {
    BOOST_REQUIRE(!prefix.empty());
    discover = make_unique<DiscoverVersion>(prefix, face, opt);
    discover->onDiscoverySuccess.connect([this] (const Name& versionedName) {
      isDiscoveryFinished = true;
      BOOST_REQUIRE(!versionedName.empty() && versionedName[-1].isVersion());
      discoveredVersion = versionedName[-1].toVersion();
    });
    discover->onDiscoveryFailure.connect([this] (const std::string& reason) {
      isDiscoveryFinished = true;
    });

    discover->run();
    advanceClocks(io, time::nanoseconds(1));
  }

protected:
  boost::asio::io_service io;
  util::DummyClientFace face;
  Name name;
  unique_ptr<DiscoverVersion> discover;
  optional<uint64_t> discoveredVersion;
  bool isDiscoveryFinished;
  uint64_t version;
  Options opt;
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestDiscoverVersion, DiscoverVersionFixture)

BOOST_AUTO_TEST_CASE(VersionNumberIsProvided)
{
  run(Name(name).appendVersion(version));

  // no version discovery interest is expressed
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_CASE(DiscoverySuccess)
{
  // express a discovery Interest to learn Data version
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  Interest discoveryInterest = MetadataObject::makeDiscoveryInterest(name);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getName(), discoveryInterest.getName());

  // Send back a metadata packet with a valid versioned name
  MetadataObject mobject;
  mobject.setVersionedName(Name(name).appendVersion(version));
  face.receive(mobject.makeData(lastInterest.getName(), m_keyChain));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_CASE(InvalidDiscoveredVersionedName)
{
  // issue a discovery Interest to learn Data version
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // Send back a metadata packet with an invalid versioned name
  MetadataObject mobject;
  mobject.setVersionedName(name);
  face.receive(mobject.makeData(face.sentInterests.back().getName(), m_keyChain));

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK(!discoveredVersion.has_value());
}

BOOST_AUTO_TEST_CASE(InvalidMetadataPacket)
{
  // issue a discovery Interest to learn Data version
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // Send back an invalid metadata packet
  Data data(face.sentInterests.back().getName());
  data.setContentType(tlv::ContentType_Key);
  face.receive(signData(data));

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK(!discoveredVersion.has_value());
}

BOOST_AUTO_TEST_CASE(Timeout1)
{
  // issue a discovery Interest to learn Data version
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // timeout discovery Interests
  for (int retries = 0; retries < opt.maxRetriesOnTimeoutOrNack; ++retries) {
    advanceClocks(io, opt.interestLifetime);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 2);
  }

  // timeout the last sent Interest
  advanceClocks(io, opt.interestLifetime);

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK(!discoveredVersion.has_value());
}

BOOST_AUTO_TEST_CASE(Timeout2)
{
  // issue a discovery Interest to learn Data version
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // timeout discovery Interests
  for (int retries = 0; retries < opt.maxRetriesOnTimeoutOrNack; ++retries) {
    advanceClocks(io, opt.interestLifetime);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 2);
  }

  // satisfy the last Interest with a valid metadata packet
  MetadataObject mobject;
  mobject.setVersionedName(Name(name).appendVersion(version));
  face.receive(mobject.makeData(face.sentInterests.back().getName(), m_keyChain));
  advanceClocks(io, time::nanoseconds(1));

  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_SUITE_END() // TestDiscoverVersion
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
