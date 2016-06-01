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

#include "tools/chunks/catchunks/discover-version-iterative.hpp"

#include "discover-version-fixture.hpp"

namespace ndn {
namespace chunks {
namespace tests {

class DiscoverVersionIterativeFixture : public DiscoverVersionFixture,
                                        protected DiscoverVersionIterativeOptions
{
public:
  typedef DiscoverVersionIterativeOptions Options;

public:
  explicit
  DiscoverVersionIterativeFixture(const Options& opt = makeOptionsIterative())
    : chunks::Options(opt)
    , DiscoverVersionFixture(opt)
    , Options(opt)
  {
    setDiscover(make_unique<DiscoverVersionIterative>(Name(name), face, opt));
  }

protected:
  static Options
  makeOptionsIterative()
  {
    Options options;
    options.isVerbose = false;
    options.maxRetriesOnTimeoutOrNack = 3;
    options.maxRetriesAfterVersionFound = 1;
    return options;
  }
};


BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_AUTO_TEST_SUITE(TestDiscoverVersionIterative)

BOOST_FIXTURE_TEST_CASE(SingleVersionAvailable, DiscoverVersionIterativeFixture)
{
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  uint64_t version = 1449241767037;

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
  BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
  BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
  BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);

  // Send first segment for the right version
  face.receive(*makeDataWithVersion(version));
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2);
  lastInterest = face.sentInterests.back();
  Exclude expectedExclude;
  expectedExclude.excludeBefore(name::Component::fromVersion(version));
  BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
  BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
  BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
  BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);

  // Generate the timeout
  for (int retries = 0; retries < maxRetriesAfterVersionFound; ++retries) {
    advanceClocks(io, interestLifetime, 1);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);

    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), retries + 3);
    lastInterest = face.sentInterests.back();
    Exclude expectedExclude;
    expectedExclude.excludeBefore(name::Component::fromVersion(version));
    BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
    BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);
  }

  advanceClocks(io, interestLifetime, 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredVersion, version);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), maxRetriesAfterVersionFound + 2);
}


BOOST_FIXTURE_TEST_CASE(NoVersionsAvailable, DiscoverVersionIterativeFixture)
{
  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);

  for (int retries = 0; retries < maxRetriesOnTimeoutOrNack; ++retries) {
    advanceClocks(io, interestLifetime, 1);
    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2 + retries);
  }

  for (auto& lastInterest : face.sentInterests) {
    BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
    BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);
  }

  advanceClocks(io, interestLifetime, 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), maxRetriesOnTimeoutOrNack + 1);
}

BOOST_FIXTURE_TEST_CASE(MultipleVersionsAvailable, DiscoverVersionIterativeFixture)
{
  // nVersions must be positive
  const uint64_t nVersions = 5;

  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
  BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
  BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
  BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);

  for (uint64_t nSentVersions = 0; nSentVersions < nVersions; ++nSentVersions) {
    face.receive(*makeDataWithVersion(nSentVersions));
    advanceClocks(io, time::nanoseconds(1), 1);

    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2 + nSentVersions);
    lastInterest = face.sentInterests.back();
    Exclude expectedExclude;
    expectedExclude.excludeBefore(name::Component::fromVersion(nSentVersions));
    BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
    BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);
  }

  for (int retries = 0; retries < maxRetriesAfterVersionFound; ++retries) {
    advanceClocks(io, interestLifetime, 1);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);

    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2 + retries + nVersions);
    lastInterest = face.sentInterests.back();
    Exclude expectedExclude;
    expectedExclude.excludeBefore(name::Component::fromVersion(nVersions - 1));
    BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
    BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);
  }

  advanceClocks(io, interestLifetime, 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredVersion, nVersions - 1);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), nVersions + maxRetriesAfterVersionFound + 1);
}

BOOST_FIXTURE_TEST_CASE(MultipleVersionsAvailableDescendent, DiscoverVersionIterativeFixture)
{
  // nVersions must be positive
  const uint64_t nVersions = 5;

  discover->run();
  advanceClocks(io, time::nanoseconds(1), 1);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  auto lastInterest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(isDiscoveryFinished, false);
  BOOST_CHECK_EQUAL(lastInterest.getExclude().size(), 0);
  BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
  BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
  BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);

  for (uint64_t nVersionsToSend = nVersions; nVersionsToSend > 0; --nVersionsToSend) {
    face.receive(*makeDataWithVersion(nVersionsToSend - 1));
    advanceClocks(io, time::nanoseconds(1), 1);

    BOOST_CHECK_EQUAL(isDiscoveryFinished, false);

    BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2);
    lastInterest = face.sentInterests.back();
    Exclude expectedExclude;
    expectedExclude.excludeBefore(name::Component::fromVersion(nVersions - 1));
    BOOST_CHECK_EQUAL(lastInterest.getExclude(), expectedExclude);
    BOOST_CHECK_EQUAL(lastInterest.getChildSelector(), 1);
    BOOST_CHECK_EQUAL(lastInterest.getMustBeFresh(), mustBeFresh);
    BOOST_CHECK_EQUAL(lastInterest.getName().equals(name), true);
  }

  advanceClocks(io, interestLifetime, maxRetriesAfterVersionFound + 1);
  BOOST_CHECK_EQUAL(isDiscoveryFinished, true);
  BOOST_CHECK_EQUAL(discoveredVersion, nVersions - 1);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), maxRetriesAfterVersionFound + 2);
}

BOOST_AUTO_TEST_SUITE_END() // TestDiscoverVersionIterative
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
