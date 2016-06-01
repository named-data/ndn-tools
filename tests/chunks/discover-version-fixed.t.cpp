/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2016,  Regents of the University of California,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University.
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
 * @author Andrea Tosatto
 */

#include "tools/chunks/catchunks/discover-version-fixed.hpp"

#include "discover-version-fixture.hpp"

namespace ndn {
namespace chunks {
namespace tests {

class DiscoverVersionFixedFixture : public DiscoverVersionFixture
{
public:
  DiscoverVersionFixedFixture()
    : DiscoverVersionFixture(makeOptions())
    , version(1449227841747)
  {
    setDiscover(make_unique<DiscoverVersionFixed>(Name(name).appendVersion(version),
                                                  face, makeOptions()));
  }

protected:
  uint64_t version; //Version to find
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_AUTO_TEST_SUITE(TestDiscoverVersionFixed)

BOOST_FIXTURE_TEST_CASE(RequestedVersionAvailable, DiscoverVersionFixedFixture)
{
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  face.receive(*makeDataWithVersion(version));

  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredVersion, version);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
  BOOST_CHECK_EQUAL(lastInterest.getMaxSuffixComponents(), 2);
  BOOST_CHECK_EQUAL(lastInterest.getMinSuffixComponents(), 2);
  BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
}

BOOST_FIXTURE_TEST_CASE(NoVersionsAvailable, DiscoverVersionFixedFixture)
{
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  for (int retries = 0; retries < maxRetriesOnTimeoutOrNack; ++retries) {
    advanceClocks(io, interestLifetime, 1);
    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2 + retries);
  }

  for (const auto& lastInterest : face.sentInterests) {
    BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
    BOOST_CHECK_EQUAL(lastInterest.getMaxSuffixComponents(), 2);
    BOOST_CHECK_EQUAL(lastInterest.getMinSuffixComponents(), 2);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(Name(name).appendVersion(version)), true);
  }

  advanceClocks(io, interestLifetime, 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  // check if discovered version is the default value
  BOOST_CHECK_EQUAL(discoveredVersion, 0);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), maxRetriesOnTimeoutOrNack + 1);
}

BOOST_FIXTURE_TEST_CASE(DataNotSegment, DiscoverVersionFixedFixture)
{
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  std::vector<std::string> randomStrings {
      "",
      "abcdefg",
      "12345",
      "qr%67a3%4e"
  };

  Exclude expectedExclude;
  for (size_t retries = 0; retries < randomStrings.size(); ++retries) {
    auto data = make_shared<Data>(Name(name).appendVersion(version).append(randomStrings[retries]));
    data->setFinalBlockId(name::Component::fromSegment(0));
    data = signData(data);

    face.receive(*data);
    advanceClocks(io, time::nanoseconds(1), 1);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);

    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1 + retries);
    auto lastInterest = face.sentInterests.back();
    if (randomStrings[retries] != "")
      expectedExclude.excludeOne(name::Component::fromEscapedString(randomStrings[retries]));
    BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
    BOOST_CHECK_EQUAL(lastInterest.getMaxSuffixComponents(), 2);
    BOOST_CHECK_EQUAL(lastInterest.getMinSuffixComponents(), 2);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
  }

  advanceClocks(io, interestLifetime, maxRetriesOnTimeoutOrNack + 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  // check if discovered version is the default value
  BOOST_CHECK_EQUAL(discoveredVersion, 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestDiscoverVersionFixed
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
