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
 * @author Chavoosh Ghasemi
 */

#include "tools/chunks/catchunks/discover-version.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/metadata-object.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn::chunks::tests {

using namespace ndn::tests;

class DiscoverVersionFixture : public IoFixture, public KeyChainFixture
{
public:
  void
  run(const Name& prefix)
  {
    discover = std::make_unique<DiscoverVersion>(face, prefix, opt);
    discover->onDiscoverySuccess.connect([this] (const Name& versionedName) {
      isDiscoveryFinished = true;
      discoveredName = versionedName;
      if (!versionedName.empty() && versionedName[-1].isVersion())
        discoveredVersion = versionedName[-1].toVersion();
    });
    discover->onDiscoveryFailure.connect([this] (const std::string&) {
      isDiscoveryFinished = true;
    });

    discover->run();
    advanceClocks(1_ns);
  }

protected:
  const Name name = "/ndn/chunks/test";
  const uint64_t version = 1449227841747;
  DummyClientFace face{m_io};
  Options opt;
  std::unique_ptr<DiscoverVersion> discover;
  std::optional<Name> discoveredName;
  std::optional<uint64_t> discoveredVersion;
  bool isDiscoveryFinished = false;
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestDiscoverVersion, DiscoverVersionFixture)

BOOST_AUTO_TEST_CASE(Disabled)
{
  opt.disableVersionDiscovery = true;
  run(name);

  // no version discovery Interest is expressed
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  BOOST_CHECK_EQUAL(discoveredName.value(), name);
  BOOST_CHECK_EQUAL(discoveredVersion.has_value(), false);
}

BOOST_AUTO_TEST_CASE(NameWithVersion)
{
  // start with a name that already contains a version component
  Name versionedName = Name(name).appendVersion(version);
  run(versionedName);

  // no version discovery Interest is expressed
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  BOOST_CHECK_EQUAL(discoveredName.value(), versionedName);
  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_CASE(Success)
{
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  Interest discoveryInterest = MetadataObject::makeDiscoveryInterest(name);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getName(), discoveryInterest.getName());

  // send back a metadata packet with a valid versioned name
  MetadataObject mobject;
  mobject.setVersionedName(Name(name).appendVersion(version));
  face.receive(mobject.makeData(lastInterest.getName(), m_keyChain));
  advanceClocks(1_ns);

  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_CASE(InvalidDiscoveredVersionedName)
{
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // send back a metadata packet with an invalid versioned name
  MetadataObject mobject;
  mobject.setVersionedName(name);
  face.receive(mobject.makeData(face.sentInterests.back().getName(), m_keyChain));

  // finish discovery process without a resolved version number
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredName.has_value(), false);
  BOOST_CHECK_EQUAL(discoveredVersion.has_value(), false);
}

BOOST_AUTO_TEST_CASE(InvalidMetadataPacket)
{
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // send back an invalid metadata packet
  Data data(face.sentInterests.back().getName());
  data.setFreshnessPeriod(1_s);
  data.setContentType(tlv::ContentType_Key);
  face.receive(signData(data));

  // finish discovery process without a resolved version number
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredName.has_value(), false);
  BOOST_CHECK_EQUAL(discoveredVersion.has_value(), false);
}

BOOST_AUTO_TEST_CASE(MaxRetriesExceeded)
{
  opt.maxRetriesOnTimeoutOrNack = 3;
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // timeout or nack discovery Interests
  for (int retries = 0; retries < opt.maxRetriesOnTimeoutOrNack * 2; ++retries) {
    if (retries % 2 == 0) {
      advanceClocks(opt.interestLifetime);
    }
    else {
      face.receive(makeNack(face.sentInterests.back(), lp::NackReason::DUPLICATE));
      advanceClocks(1_ns);
    }

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 2);
  }

  // timeout the last sent Interest
  advanceClocks(opt.interestLifetime);

  // finish discovery process without a resolved version number
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredName.has_value(), false);
  BOOST_CHECK_EQUAL(discoveredVersion.has_value(), false);
}

BOOST_AUTO_TEST_CASE(SuccessAfterNackAndTimeout)
{
  opt.maxRetriesOnTimeoutOrNack = 3;
  run(name);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // timeout or nack discovery Interests
  for (int retries = 0; retries < opt.maxRetriesOnTimeoutOrNack * 2; ++retries) {
    if (retries % 2 == 0) {
      advanceClocks(opt.interestLifetime);
    }
    else {
      face.receive(makeNack(face.sentInterests.back(), lp::NackReason::DUPLICATE));
      advanceClocks(1_ns);
    }

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 2);
  }

  // satisfy the last Interest with a valid metadata packet
  MetadataObject mobject;
  mobject.setVersionedName(Name(name).appendVersion(version));
  face.receive(mobject.makeData(face.sentInterests.back().getName(), m_keyChain));
  advanceClocks(1_ns);

  BOOST_CHECK_EQUAL(discoveredVersion.value(), version);
}

BOOST_AUTO_TEST_SUITE_END() // TestDiscoverVersion
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace ndn::chunks::tests
