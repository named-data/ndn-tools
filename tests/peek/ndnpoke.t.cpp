/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
#include "tests/io-fixture.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn::peek::tests {

using namespace ndn::tests;

template<bool WANT_PREFIX_REG_REPLY = true>
class NdnPokeFixture : public IoFixture, public KeyChainFixture
{
protected:
  NdnPokeFixture()
  {
    m_keyChain.createIdentity("/test-id");
  }

  void
  initialize(const PokeOptions& opts = makeDefaultOptions())
  {
    poke = std::make_unique<NdnPoke>(face, m_keyChain, payload, opts);
  }

  static PokeOptions
  makeDefaultOptions()
  {
    PokeOptions opt;
    opt.name = "/poke/test";
    return opt;
  }

protected:
  DummyClientFace face{m_io, m_keyChain, {true, WANT_PREFIX_REG_REPLY}};
  std::stringstream payload{"Hello, world!\n"};
  std::unique_ptr<NdnPoke> poke;
};

BOOST_AUTO_TEST_SUITE(Peek)
BOOST_FIXTURE_TEST_SUITE(TestNdnPoke, NdnPokeFixture<>)

BOOST_AUTO_TEST_CASE(Basic)
{
  initialize();

  poke->start();
  this->advanceClocks(1_ms, 10);

  // Check for prefix registration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.front().getName().getPrefix(4), "/localhost/nfd/rib/register");

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getContentType(), tlv::ContentType_Blob);
  BOOST_CHECK_EQUAL(face.sentData.back().getContent(), "150E48656C6C6F2C20776F726C64210A"_block);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignatureType(), tlv::SignatureSha256WithEcdsa);

  // Check for prefix unregistration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2); // One for registration, one for unregistration
  BOOST_CHECK_EQUAL(face.sentInterests.back().getName().getPrefix(4), "/localhost/nfd/rib/unregister");
}

BOOST_AUTO_TEST_CASE(NoMatch)
{
  initialize();

  poke->start();
  this->advanceClocks(1_ms, 10);

  face.receive(*makeInterest("/poke/test/foo"));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::UNKNOWN);
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
}

BOOST_AUTO_TEST_CASE(FreshnessPeriod)
{
  auto options = makeDefaultOptions();
  options.freshnessPeriod = 1_s;
  initialize(options);

  poke->start();
  this->advanceClocks(1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 1_s);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignatureType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(FinalBlockId)
{
  auto options = makeDefaultOptions();
  options.name = "/poke/test/123";
  options.wantFinalBlockId = true;
  initialize(options);

  poke->start();
  this->advanceClocks(1_ms, 10);

  face.receive(*makeInterest(options.name));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), options.name);
  BOOST_REQUIRE(face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(*(face.sentData.back().getFinalBlock()), name::Component("123"));
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignatureType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(DigestSha256)
{
  auto options = makeDefaultOptions();
  options.signingInfo.setSha256Signing();
  initialize(options);

  poke->start();
  this->advanceClocks(1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignatureType(), tlv::DigestSha256);
}

BOOST_AUTO_TEST_CASE(Unsolicited)
{
  auto options = makeDefaultOptions();
  options.wantUnsolicited = true;
  initialize(options);

  poke->start();
  this->advanceClocks(1_ms, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::DATA_SENT);
  BOOST_REQUIRE_EQUAL(face.sentData.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.back().getName(), "/poke/test");
  BOOST_CHECK(!face.sentData.back().getFinalBlock());
  BOOST_CHECK_EQUAL(face.sentData.back().getFreshnessPeriod(), 0_ms);
  BOOST_CHECK_EQUAL(face.sentData.back().getSignatureType(), tlv::SignatureSha256WithEcdsa);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  auto options = makeDefaultOptions();
  options.timeout = 4_s;
  initialize(options);

  poke->start();
  this->advanceClocks(1_ms, 10);

  // Check for prefix registration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.front().getName().getPrefix(4), "/localhost/nfd/rib/register");

  this->advanceClocks(1_s, 4);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::TIMEOUT);
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);

  // Check for prefix unregistration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 2); // One for registration, one for unregistration
  BOOST_CHECK_EQUAL(face.sentInterests.back().getName().getPrefix(4), "/localhost/nfd/rib/unregister");
}

BOOST_FIXTURE_TEST_CASE(PrefixRegTimeout, NdnPokeFixture<false>)
{
  initialize();

  poke->start();
  this->advanceClocks(1_ms, 10);

  // Check for prefix registration
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.front().getName().getPrefix(4), "/localhost/nfd/rib/register");

  this->advanceClocks(1_s, 10);

  BOOST_CHECK(poke->getResult() == NdnPoke::Result::PREFIX_REG_FAIL);
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_CASE(OversizedPacket)
{
  payload << std::string(MAX_NDN_PACKET_SIZE, 'A');
  initialize();

  poke->start();
  this->advanceClocks(1_ms, 10);

  face.receive(*makeInterest("/poke/test"));
  BOOST_CHECK_THROW(face.processEvents(), Face::OversizedPacketError);

  // No point in checking getResult() here. The exception is thrown from processEvents(),
  // not from put(), so the result is still DATA_SENT even though no packets were sent.
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestNdnPoke
BOOST_AUTO_TEST_SUITE_END() // Peek

} // namespace ndn::peek::tests
