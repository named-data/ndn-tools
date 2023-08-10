/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2023,  Regents of the University of California,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University.
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

#include "tools/chunks/putchunks/producer.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/metadata-object.hpp>
#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/pib/key.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include <cmath>
#include <sstream>

namespace ndn::chunks::tests {

using namespace ndn::tests;

class ProducerFixture : public IoFixture, public KeyChainFixture
{
protected:
  ProducerFixture()
  {
    options.maxSegmentSize = 40;
    options.isQuiet = true;
  }

protected:
  DummyClientFace face{m_io, {true, true}};
  Name prefix = "/ndn/chunks/test";
  Producer::Options options;
  uint64_t version = 1449227841747;
  Name keyLocatorName = m_keyChain.createIdentity("/putchunks/producer")
                        .getDefaultKey().getDefaultCertificate().getName();
  std::istringstream testString{
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
    "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, "
    "nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, "
    "sem. Nulla consequat massa Donec pede justo,"s};
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestProducer, ProducerFixture)

BOOST_AUTO_TEST_CASE(InputData)
{
  const std::vector<std::string> testStrings{
      "",

      "a1b2c3%^&(#$&%^$$/><",

      "123456789123456789123456789123456789123456789123456789123456789"
      "123456789123456789123456789123456789123456789123456789123456789",

      "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. "
      "Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur "
      "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla "
      "consequat massa Donec pede justo,"
  };

  for (const auto& str : testStrings) {
    std::istringstream input(str);
    Producer prod(prefix, face, m_keyChain, input, options);
    size_t expectedSize = str.empty() ? 1 : std::ceil(static_cast<double>(str.size()) / options.maxSegmentSize);
    BOOST_CHECK_EQUAL(prod.m_store.size(), expectedSize);
  }
}

BOOST_AUTO_TEST_CASE(RequestSegmentUnspecifiedVersion)
{
  Producer producer(prefix, face, m_keyChain, testString, options);
  m_io.poll();
  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / options.maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix, true));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_CHECK_EQUAL(lastData.getFinalBlock().value().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getKeyLocator().value().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  nameWithVersion.append(lastData.getName()[-2]);
  size_t requestSegmentNo = 1;

  face.receive(*makeInterest(nameWithVersion.appendSegment(requestSegmentNo), true));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 2);
  lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), requestSegmentNo);
  BOOST_CHECK_EQUAL(lastData.getFinalBlock().value().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getKeyLocator().value().getName(), keyLocatorName);
}

BOOST_AUTO_TEST_CASE(RequestSegmentSpecifiedVersion)
{
  Producer producer(prefix.appendVersion(version), face, m_keyChain, testString, options);
  m_io.poll();
  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / options.maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix, true));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 1);
  BOOST_CHECK_EQUAL(lastData.getName()[-2].toVersion(), version);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_CHECK_EQUAL(lastData.getFinalBlock().value().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getKeyLocator().value().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  size_t requestSegmentNo = 1;

  face.receive(*makeInterest(nameWithVersion.appendSegment(requestSegmentNo), true));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 2);
  lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 1);
  BOOST_CHECK_EQUAL(lastData.getName()[-2].toVersion(), version);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), requestSegmentNo);
  BOOST_CHECK_EQUAL(lastData.getFinalBlock().value().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getKeyLocator().value().getName(), keyLocatorName);
}

BOOST_AUTO_TEST_CASE(RequestNotExistingSegment)
{
  Producer producer(prefix, face, m_keyChain, testString, options);
  m_io.poll();
  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / options.maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix, true));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_CHECK_EQUAL(lastData.getFinalBlock().value().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getKeyLocator().value().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  nameWithVersion.append(lastData.getName()[-2]);
  face.receive(*makeInterest(nameWithVersion.appendSegment(nSegments), true));
  face.processEvents();

  // no new data
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
}

BOOST_AUTO_TEST_CASE(RequestMetadata)
{
  Producer producer(prefix.appendVersion(version), face, m_keyChain, testString, options);
  m_io.poll();

  // ask for metadata with a valid discovery interest
  face.receive(MetadataObject::makeDiscoveryInterest(Name(prefix).getPrefix(-1)));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();

  // check the name of metadata packet
  BOOST_CHECK(MetadataObject::isValidName(lastData.getName()));

  // make metadata object from metadata packet
  MetadataObject mobject(lastData);
  BOOST_CHECK_EQUAL(mobject.getVersionedName(), prefix);

  // ask for metadata with an invalid discovery interest
  face.receive(MetadataObject::makeDiscoveryInterest(Name(prefix).getPrefix(-1))
               .setCanBePrefix(false));
  face.processEvents();

  // we expect Nack in response to a discovery interest without CanBePrefix
  BOOST_CHECK_EQUAL(face.sentNacks.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END() // TestProducer
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace ndn::chunks::tests
