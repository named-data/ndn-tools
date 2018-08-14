/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 */

#include "tools/peek/ndnpoke/ndnpoke.hpp"

#include "tests/test-common.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace peek {
namespace tests {

using namespace ndn::tests;

class NdnPokeFixture : public UnitTestTimeFixture
{
protected:
  NdnPokeFixture()
    : face(io, keyChain, {true, true})
  {
    keyChain.createIdentity("/test-id");

    inData << "Hello world!";
  }

  void
  initialize(const PokeOptions& opts)
  {
    poke = make_unique<NdnPoke>(face, keyChain, inData, opts);
  }

  static PokeOptions
  makeDefaultOptions()
  {
    PokeOptions opt;
    opt.prefixName = "/poke/test";
    return opt;
  }

protected:
  boost::asio::io_service io;
  ndn::util::DummyClientFace face;
  KeyChain keyChain;
  unique_ptr<NdnPoke> poke;
  std::stringstream inData;
};

BOOST_AUTO_TEST_SUITE(Peek)
BOOST_FIXTURE_TEST_SUITE(TestNdnPoke, NdnPokeFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  auto options = makeDefaultOptions();
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  // Check for prefix registration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.front().getName().getPrefix(4), "/localhost/nfd/rib/register");

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(io, 1_ms, 10);
  io.run();

  BOOST_CHECK(poke->wasDataSent());
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignature().getType(), tlv::SignatureSha256WithEcdsa);

  // Check for prefix unregistration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2); // One for registration, one for unregistration
  BOOST_CHECK_EQUAL(face.sentInterests.back().getName().getPrefix(4), "/localhost/nfd/rib/unregister");
}

BOOST_AUTO_TEST_CASE(FinalBlockId)
{
  auto options = makeDefaultOptions();
  options.prefixName = "/poke/test/123";
  options.wantLastAsFinalBlockId = true;
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  face.receive(*makeInterest("/poke/test/123"));
  this->advanceClocks(io, 1_ms, 10);
  io.run();

  BOOST_CHECK(poke->wasDataSent());
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test/123");
  BOOST_REQUIRE(face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(*(face.sentData.back().getFinalBlock()), name::Component("123"));
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignature().getType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(FreshnessPeriod)
{
  auto options = makeDefaultOptions();
  options.freshnessPeriod = make_optional<time::milliseconds>(1_s);
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(io, 1_ms, 10);
  io.run();

  BOOST_CHECK(poke->wasDataSent());
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 1_s);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignature().getType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(DigestSha256)
{
  auto options = makeDefaultOptions();
  options.signingInfo.setSha256Signing();
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(io, 1_ms, 10);
  io.run();

  BOOST_CHECK(poke->wasDataSent());
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignature().getType(), tlv::DigestSha256);
}

BOOST_AUTO_TEST_CASE(ForceData)
{
  auto options = makeDefaultOptions();
  options.wantForceData = true;
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  BOOST_CHECK(poke->wasDataSent());
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignature().getType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(ExceedMaxPacketSize)
{
  for (size_t i = 0; i < MAX_NDN_PACKET_SIZE; i++) {
    inData << "A";
  }

  auto options = makeDefaultOptions();
  initialize(options);

  poke->start();

  this->advanceClocks(io, 1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  BOOST_CHECK_THROW(face.processEvents(), Face::OversizedPacketError);

  // We can't check wasDataSent() correctly here because it will be set to true, even if put failed
  // due to the packet being oversized.
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestNdnPoke
BOOST_AUTO_TEST_SUITE_END() // Peek

} // namespace tests
} // namespace peek
} // namespace ndn
