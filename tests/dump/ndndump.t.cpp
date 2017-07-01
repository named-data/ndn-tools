/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  University of Memphis.
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

#include "tools/dump/ndndump.hpp"

#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/net/ethernet.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"
#include <boost/test/output_test_stream.hpp>

namespace ndn {
namespace dump {
namespace tests {

using namespace ndn::tests;

class StdCoutRedirector
{
public:
  StdCoutRedirector(std::ostream& os)
  {
    // Redirect std::cout to the specified output stream
    originalBuffer = std::cout.rdbuf(os.rdbuf());
  }

  ~StdCoutRedirector()
  {
    // Revert state for std::cout
    std::cout.rdbuf(originalBuffer);
  }

private:
  std::streambuf* originalBuffer;
};

class NdnDumpFixture : public IdentityManagementFixture
{
protected:
  NdnDumpFixture()
  {
    dump.m_dataLinkType = DLT_EN10MB;
  }

  template<typename Packet>
  void
  receive(const Packet& packet)
  {
    EncodingBuffer buffer(packet.wireEncode());
    receive(buffer);
  }

  void
  receive(EncodingBuffer& buffer)
  {
    ethernet::Address host;

    // Ethernet header
    uint16_t frameType = htons(ethernet::ETHERTYPE_NDN);
    buffer.prependByteArray(reinterpret_cast<const uint8_t*>(&frameType), ethernet::TYPE_LEN);
    buffer.prependByteArray(host.data(), host.size());
    buffer.prependByteArray(host.data(), host.size());

    pcap_pkthdr header{};
    header.len = buffer.size();

    {
      StdCoutRedirector redirect(output);
      dump.onCapturedPacket(&header, buffer.buf());
    }
  }

protected:
  Ndndump dump;
  boost::test_tools::output_test_stream output;
};

BOOST_FIXTURE_TEST_SUITE(NdnDump, NdnDumpFixture)

BOOST_AUTO_TEST_CASE(CaptureInterest)
{
  Interest interest("/test");
  interest.setNonce(0);

  this->receive(interest);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, INTEREST: /test?ndn.Nonce=0\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureData)
{
  Data data("/test");
  m_keyChain.sign(data);

  this->receive(data);

  const std::string expectedOutput = "0.000000 Tunnel Type: EthernetFrame, DATA: /test\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureNack)
{
  Interest interest("/test");
  interest.setNonce(0);

  lp::Nack nack(interest);
  nack.setReason(lp::NackReason::DUPLICATE);

  lp::Packet lpPacket(interest.wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());

  this->receive(lpPacket);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, NACK: Duplicate, /test?ndn.Nonce=0\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureLpFragment)
{
  const uint8_t data[10] = {
    0x06, 0x08, // Data packet
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  };

  Buffer buffer(data, 4);

  lp::Packet lpPacket;
  lpPacket.add<lp::FragmentField>(std::make_pair(buffer.begin(), buffer.end()));
  lpPacket.add<lp::FragIndexField>(0);
  lpPacket.add<lp::FragCountField>(2);
  lpPacket.add<lp::SequenceField>(1000);

  this->receive(lpPacket);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, NDNLPv2-FRAGMENT\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureIdlePacket)
{
  lp::Packet lpPacket;

  this->receive(lpPacket);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, NDNLPv2-IDLE\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureIncompletePacket)
{
  const uint8_t interest[] = {
  0x05, 0x0E, // Interest
    0x07, 0x06, // Name
      0x08, 0x04, // NameComponent
        0x74, 0x65, 0x73, 0x74,
    0x0a, 0x04, // Nonce
      0x00, 0x00, 0x00, 0x01
  };

  EncodingBuffer buffer;
  buffer.prependByteArray(interest, 4);

  this->receive(buffer);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, INCOMPLETE-PACKET, size: 4\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(CaptureUnknownNetworkPacket)
{
  EncodingBuffer buffer(encoding::makeEmptyBlock(tlv::Name));

  this->receive(buffer);

  const std::string expectedOutput =
    "0.000000 Tunnel Type: EthernetFrame, UNKNOWN-NETWORK-PACKET\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}

BOOST_AUTO_TEST_CASE(DumpPcapTrace)
{
  dump.inputFile = "tests/dump/nack.pcap";
  dump.pcapProgram = "";

  {
    StdCoutRedirector redirect(output);
    dump.run();
  }

  const std::string expectedOutput =
    "1456768916.467099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: UDP, INTEREST: /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=2581361680\n"
    "1456768916.567099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: UDP, INTEREST: /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=4138343109\n"
    "1456768916.667099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: UDP, INTEREST: /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=4034910304\n"
    "1456768916.767099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: UDP, NACK: Congestion, /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=2581361680\n"
    "1456768916.867099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: UDP, NACK: Duplicate, /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=4138343109\n"
    "1456768916.967099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: UDP, NACK: None, /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=4034910304\n"
    "1456768917.067099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: TCP, INTEREST: /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=3192497423\n"
    "1456768917.267099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: TCP, NACK: Congestion, /producer/nack/congestion?ndn.MustBeFresh=1&ndn.Nonce=3192497423\n"
    "1456768917.367099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: TCP, INTEREST: /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=522390724\n"
    "1456768917.567099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: TCP, NACK: Duplicate, /producer/nack/duplicate?ndn.MustBeFresh=1&ndn.Nonce=522390724\n"
    "1456768917.767099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: TCP, NACK: None, /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=2002441365\n"
    "1456768917.967099 From: 1.0.0.1, To: 1.0.0.2, Tunnel Type: TCP, INTEREST: /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=3776824408\n"
    "1456768918.067099 From: 1.0.0.2, To: 1.0.0.1, Tunnel Type: TCP, NACK: None, /producer/nack/no-reason?ndn.MustBeFresh=1&ndn.Nonce=3776824408\n";

  BOOST_CHECK(output.is_equal(expectedOutput));
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace dump
} // namespace ndn
