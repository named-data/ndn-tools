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

#include "tools/chunks/putchunks/producer.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/security/validator-null.hpp>

#include <cmath>

namespace ndn {
namespace chunks {
namespace tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_AUTO_TEST_SUITE(TestProducer)

BOOST_AUTO_TEST_CASE(InputData)
{
  util::DummyClientFace face;
  KeyChain keyChain;
  security::SigningInfo signingInfo;
  Name prefix("/ndn/chunks/test");
  int maxSegmentSize = 40;
  std::vector<std::string> testStrings {
      "",

      "a1b2c3%^&(#$&%^$$/><",

      "123456789123456789123456789123456789123456789123456789123456789"
      "123456789123456789123456789123456789123456789123456789123456789",

      "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. "
      "Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur "
      "ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla "
      "consequat massa Donec pede justo,"
  };

  for (size_t i = 0; i <  testStrings.size(); ++i) {
    std::istringstream str(testStrings[i]);
    Producer prod(prefix, face, keyChain, signingInfo, time::seconds(4), maxSegmentSize, false,
                  false, str);

    size_t expectedSize = std::ceil(static_cast<double>(testStrings[i].size()) / maxSegmentSize);
    if (testStrings[i].size() == 0)
      expectedSize = 1;

    BOOST_CHECK_EQUAL(prod.m_store.size(), expectedSize);
  }
}

BOOST_AUTO_TEST_CASE(RequestSegmentUnspecifiedVersion)
{
  boost::asio::io_service io;
  util::DummyClientFace face(io, {true, true});
  KeyChain keyChain;
  security::SigningInfo signingInfo;
  Name prefix("/ndn/chunks/test");
  time::milliseconds freshnessPeriod(time::seconds(10));
  size_t maxSegmentSize(40);
  std::istringstream testString(std::string(
          "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
          "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, "
          "nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, "
          "sem. Nulla consequat massa Donec pede justo,"));

  Name keyLocatorName = keyChain.getDefaultCertificateName().getPrefix(-1);

  Producer producer(prefix, face, keyChain, signingInfo, freshnessPeriod, maxSegmentSize,
                    false, false, testString);
  io.poll();

  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_REQUIRE(!lastData.getFinalBlockId().empty());
  BOOST_CHECK_EQUAL(lastData.getFinalBlockId().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getSignature().getKeyLocator().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  nameWithVersion.append(lastData.getName()[-2]);
  size_t requestSegmentNo = 1;

  face.receive(*makeInterest(nameWithVersion.appendSegment(requestSegmentNo)));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 2);
  lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), requestSegmentNo);
  BOOST_REQUIRE(!lastData.getFinalBlockId().empty());
  BOOST_CHECK_EQUAL(lastData.getFinalBlockId().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getSignature().getKeyLocator().getName(), keyLocatorName);
}

BOOST_AUTO_TEST_CASE(RequestSegmentSpecifiedVersion)
{
  boost::asio::io_service io;
  util::DummyClientFace face(io, {true, true});
  KeyChain keyChain;
  security::SigningInfo signingInfo;
  Name prefix("/ndn/chunks/test");
  time::milliseconds freshnessPeriod(time::seconds(10));
  size_t maxSegmentSize(40);
  std::istringstream testString(std::string(
          "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
          "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, "
          "nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, "
          "sem. Nulla consequat massa Donec pede justo,"));

  uint64_t version = 1449227841747;
  Name keyLocatorName = keyChain.getDefaultCertificateName().getPrefix(-1);

  Producer producer(prefix.appendVersion(version), face, keyChain, signingInfo, freshnessPeriod,
                    maxSegmentSize, false, false, testString);
  io.poll();

  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 1);
  BOOST_CHECK_EQUAL(lastData.getName()[-2].toVersion(), version);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_REQUIRE(!lastData.getFinalBlockId().empty());
  BOOST_CHECK_EQUAL(lastData.getFinalBlockId().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getSignature().getKeyLocator().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  size_t requestSegmentNo = 1;

  face.receive(*makeInterest(nameWithVersion.appendSegment(requestSegmentNo)));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 2);
  lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 1);
  BOOST_CHECK_EQUAL(lastData.getName()[-2].toVersion(), version);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), requestSegmentNo);
  BOOST_REQUIRE(!lastData.getFinalBlockId().empty());
  BOOST_CHECK_EQUAL(lastData.getFinalBlockId().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getSignature().getKeyLocator().getName(), keyLocatorName);
}

BOOST_AUTO_TEST_CASE(RequestNotExistingSegment)
{
  boost::asio::io_service io;
  util::DummyClientFace face(io, {true, true});
  KeyChain keyChain;
  security::SigningInfo signingInfo;
  Name prefix("/ndn/chunks/test");
  time::milliseconds freshnessPeriod(time::seconds(10));
  size_t maxSegmentSize(40);
  std::istringstream testString(std::string(
          "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget "
          "dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, "
          "nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, "
          "sem. Nulla consequat massa Donec pede justo,"));

  Name keyLocatorName = keyChain.getDefaultCertificateName().getPrefix(-1);

  Producer producer(prefix, face, keyChain, signingInfo, freshnessPeriod, maxSegmentSize,
                    false, false, testString);
  io.poll();

  size_t nSegments = std::ceil(static_cast<double>(testString.str().size()) / maxSegmentSize);

  // version request
  face.receive(*makeInterest(prefix));
  face.processEvents();

  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  auto lastData = face.sentData.back();
  BOOST_REQUIRE_EQUAL(lastData.getName().size(), prefix.size() + 2);
  BOOST_CHECK_EQUAL(lastData.getName()[-1].toSegment(), 0);
  BOOST_REQUIRE(!lastData.getFinalBlockId().empty());
  BOOST_CHECK_EQUAL(lastData.getFinalBlockId().toSegment(), nSegments - 1);
  BOOST_CHECK_EQUAL(lastData.getSignature().getKeyLocator().getName(), keyLocatorName);

  // segment request
  Name nameWithVersion(prefix);
  nameWithVersion.append(lastData.getName()[-2]);
  face.receive(*makeInterest(nameWithVersion.appendSegment(nSegments)));
  face.processEvents();

  // no new data
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END() // TestProducer
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace chunks
} // namespace ndn
