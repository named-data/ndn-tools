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

#include "tools/chunks/catchunks/discover-version-realtime.hpp"

#include "discover-version-fixture.hpp"
#include "tests/identity-management-fixture.hpp"

#include <ndn-cxx/metadata-object.hpp>


namespace ndn {
namespace chunks {
namespace tests {

class DiscoverVersionRealtimeFixture : public DiscoverVersionFixture,
                                       public IdentityManagementFixture,
                                       protected DiscoverVersionRealtimeOptions
{
public:
  typedef DiscoverVersionRealtimeOptions Options;

public:
  explicit
  DiscoverVersionRealtimeFixture(const Options& opt = makeOptionsRealtime())
    : chunks::Options(opt)
    , DiscoverVersionFixture(opt)
    , Options(opt)
  {
    setDiscover(make_unique<DiscoverVersionRealtime>(Name(name), face, opt));
  }

protected:
  static Options
  makeOptionsRealtime()
  {
    Options options;
    options.isVerbose = false;
    options.maxRetriesOnTimeoutOrNack = 15;
    return options;
  }
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestDiscoverVersionRealtime, DiscoverVersionRealtimeFixture)

BOOST_AUTO_TEST_CASE(Success)
{
  // issue a discovery Interest to learn Data version
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  Interest discoveryInterest = MetadataObject::makeDiscoveryInterest(name);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getName(), discoveryInterest.getName());

  // Send back a metadata packet with a valid versioned name
  uint64_t version = 1449241767037;

  MetadataObject mobject;
  mobject.setVersionedName(Name(name).appendVersion(version));
  face.receive(mobject.makeData(lastInterest.getName(), m_keyChain));
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion, version);
}

BOOST_AUTO_TEST_CASE(InvalidVersionedName)
{
  // issue a discovery Interest to learn Data version
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // Send back a metadata packet with an invalid versioned name
  MetadataObject mobject;
  mobject.setVersionedName(name);
  face.receive(mobject.makeData(face.sentInterests.back().getName(), m_keyChain));

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion, 0);
}

BOOST_AUTO_TEST_CASE(InvalidMetadataPacket)
{
  // issue a discovery Interest to learn Data version
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // Send back an invalid metadata packet
  Data data(face.sentInterests.back().getName());
  data.setContentType(tlv::ContentType_Key);
  face.receive(signData(data));

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion, 0);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  // issue a discovery Interest to learn Data version
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  // timeout discovery Interests
  for (int retries = 0; retries < maxRetriesOnTimeoutOrNack; ++retries) {
    advanceClocks(io, interestLifetime, 1);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 2);
  }

  // timeout the last sent Interest
  advanceClocks(io, interestLifetime, 1);

  // finish discovery process without a resolved version number
  BOOST_CHECK(isDiscoveryFinished);
  BOOST_CHECK_EQUAL(discoveredVersion, 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestDiscoverVersionRealtime
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
