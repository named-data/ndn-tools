/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2024, Regents of the University of California.
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

#include "ndndump.hpp"

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <pcap/sll.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <ndn-cxx/lp/fields.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/net/ethernet.hpp>
#include <ndn-cxx/util/backports.hpp>
#include <ndn-cxx/util/exception.hpp>
#include <ndn-cxx/util/scope.hpp>
#include <ndn-cxx/util/string-helper.hpp>

#include <boost/endian/conversion.hpp>

namespace ndn::dump {

namespace endian = boost::endian;

class OutputFormatter : noncopyable
{
public:
  OutputFormatter(std::ostream& os, std::string d)
    : m_os(os)
    , m_delim(std::move(d))
  {
  }

  OutputFormatter&
  addDelimiter()
  {
    if (!m_isEmpty) {
      m_wantDelim = true;
    }
    return *this;
  }

private:
  std::ostream& m_os;
  std::string m_delim;
  bool m_isEmpty = true;
  bool m_wantDelim = false;

  template<typename T>
  friend OutputFormatter& operator<<(OutputFormatter&, const T&);
};

template<typename T>
OutputFormatter&
operator<<(OutputFormatter& out, const T& val)
{
  if (out.m_wantDelim) {
    out.m_os << out.m_delim;
    out.m_wantDelim = false;
  }
  out.m_os << val;
  out.m_isEmpty = false;
  return out;
}

NdnDump::~NdnDump()
{
  if (m_pcap)
    pcap_close(m_pcap);
}

void
NdnDump::run()
{
  char errbuf[PCAP_ERRBUF_SIZE] = {};

  if (inputFile.empty() && interface.empty()) {
    pcap_if_t* allDevs = nullptr;
    int res = pcap_findalldevs(&allDevs, errbuf);
    auto freealldevs = ndn::make_scope_exit([=] { pcap_freealldevs(allDevs); });

    if (res != 0) {
      NDN_THROW(Error(errbuf));
    }
    if (allDevs == nullptr) {
      NDN_THROW(Error("No network interface found"));
    }

    interface = allDevs->name;
  }

  std::string action;
  if (!interface.empty()) {
    m_pcap = pcap_open_live(interface.data(), 65535, wantPromisc, 1000, errbuf);
    if (m_pcap == nullptr) {
      NDN_THROW(Error("Cannot open interface " + interface + ": " + errbuf));
    }
    action = "listening on " + interface;
  }
  else {
    m_pcap = pcap_open_offline(inputFile.data(), errbuf);
    if (m_pcap == nullptr) {
      NDN_THROW(Error("Cannot open file '" + inputFile + "' for reading: " + errbuf));
    }
    action = "reading from file " + inputFile;
  }

  m_dataLinkType = pcap_datalink(m_pcap);
  const char* dltName = pcap_datalink_val_to_name(m_dataLinkType);
  const char* dltDesc = pcap_datalink_val_to_description(m_dataLinkType);
  std::string formattedDlt = dltName ? dltName : std::to_string(m_dataLinkType);
  if (dltDesc) {
    formattedDlt += " ("s + dltDesc + ")";
  }

  std::cerr << "ndndump: " << action << ", link-type " << formattedDlt << std::endl;

  switch (m_dataLinkType) {
  case DLT_EN10MB:
  case DLT_LINUX_SLL:
  case DLT_PPP:
    // we know how to handle these
    break;
  default:
    NDN_THROW(Error("Unsupported link-layer header type " + formattedDlt));
  }

  if (!pcapFilter.empty()) {
    if (wantVerbose) {
      std::cerr << "ndndump: using pcap filter: " << pcapFilter << std::endl;
    }

    bpf_program program;
    int res = pcap_compile(m_pcap, &program, pcapFilter.data(), 1, PCAP_NETMASK_UNKNOWN);
    if (res < 0) {
      NDN_THROW(Error("Cannot compile pcap filter '" + pcapFilter + "': " + pcap_geterr(m_pcap)));
    }
    auto freecode = ndn::make_scope_exit([&] { pcap_freecode(&program); });

    res = pcap_setfilter(m_pcap, &program);
    if (res < 0) {
      NDN_THROW(Error("Cannot set pcap filter: "s + pcap_geterr(m_pcap)));
    }
  }

  auto callback = [] (uint8_t* user, const pcap_pkthdr* pkthdr, const uint8_t* payload) {
    reinterpret_cast<const NdnDump*>(user)->printPacket(pkthdr, payload);
  };

  if (pcap_loop(m_pcap, -1, callback, reinterpret_cast<uint8_t*>(this)) < 0) {
    NDN_THROW(Error("pcap_loop: "s + pcap_geterr(m_pcap)));
  }
}

void
NdnDump::printPacket(const pcap_pkthdr* pkthdr, const uint8_t* payload) const
{
  // sanity checks
  if (pkthdr->caplen == 0) {
    std::cout << "[Invalid header: caplen=0]" << std::endl;
    return;
  }
  if (pkthdr->len == 0) {
    std::cout << "[Invalid header: len=0]" << std::endl;
    return;
  }
  else if (pkthdr->len < pkthdr->caplen) {
    std::cout << "[Invalid header: len(" << pkthdr->len
              << ") < caplen(" << pkthdr->caplen << ")]" << std::endl;
    return;
  }

  std::ostringstream os;
  OutputFormatter out(os, ", ");

  bool shouldPrint = false;
  switch (m_dataLinkType) {
  case DLT_EN10MB:
    shouldPrint = printEther(out, payload, pkthdr->len);
    break;
  case DLT_LINUX_SLL:
    shouldPrint = printLinuxSll(out, payload, pkthdr->len);
    break;
  case DLT_PPP:
    shouldPrint = printPpp(out, payload, pkthdr->len);
    break;
  default:
    NDN_CXX_UNREACHABLE;
  }

  if (shouldPrint) {
    if (wantTimestamp) {
      printTimestamp(std::cout, pkthdr->ts);
    }
    std::cout << os.str() << std::endl;
  }
}

void
NdnDump::printTimestamp(std::ostream& os, const timeval& tv)
{
  // TODO: Add more timestamp formats (time since previous packet, time since first packet, ...)
  os << tv.tv_sec
     << "."
     << std::setfill('0') << std::setw(6) << tv.tv_usec
     << " ";
}

bool
NdnDump::dispatchByEtherType(OutputFormatter& out, const uint8_t* pkt, size_t len, uint16_t etherType) const
{
  out.addDelimiter();

  switch (etherType) {
  case ETHERTYPE_IP:
    return printIp4(out, pkt, len);
  case ETHERTYPE_IPV6:
    return printIp6(out, pkt, len);
  case ethernet::ETHERTYPE_NDN:
  case 0x7777: // NDN ethertype used in ndnSIM
    out << "Ethernet";
    return printNdn(out, pkt, len);
  default:
    out << "[Unsupported ethertype " << AsHex{etherType} << "]";
    return true;
  }
}

bool
NdnDump::dispatchByIpProto(OutputFormatter& out, const uint8_t* pkt, size_t len, uint8_t ipProto) const
{
  out.addDelimiter();

  switch (ipProto) {
  case IPPROTO_NONE:
    out << "[No next header]";
    return true;
  case IPPROTO_TCP:
    return printTcp(out, pkt, len);
  case IPPROTO_UDP:
    return printUdp(out, pkt, len);
  default:
    out << "[Unsupported IP proto " << static_cast<int>(ipProto) << "]";
    return true;
  }
}

bool
NdnDump::printEther(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  // IEEE 802.3 Ethernet

  if (len < ethernet::HDR_LEN) {
    out << "Truncated Ethernet frame, length " << len;
    return true;
  }

  auto ether = reinterpret_cast<const ether_header*>(pkt);
  pkt += ethernet::HDR_LEN;
  len -= ethernet::HDR_LEN;

  return dispatchByEtherType(out, pkt, len, endian::big_to_native(ether->ether_type));
}

bool
NdnDump::printLinuxSll(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  // Linux "cooked" capture encapsulation
  // https://www.tcpdump.org/linktypes/LINKTYPE_LINUX_SLL.html

  if (len < SLL_HDR_LEN) {
    out << "Truncated LINUX_SLL frame, length " << len;
    return true;
  }

  auto sll = reinterpret_cast<const sll_header*>(pkt);
  pkt += SLL_HDR_LEN;
  len -= SLL_HDR_LEN;

  return dispatchByEtherType(out, pkt, len, endian::big_to_native(sll->sll_protocol));
}

bool
NdnDump::printPpp(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  // PPP, as per RFC 1661 and RFC 1662; if the first 2 bytes are 0xff and 0x03,
  // it's PPP in HDLC-like framing, with the PPP header following those two bytes,
  // otherwise it's PPP without framing, and the packet begins with the PPP header.
  // The data in the frame is not octet-stuffed or bit-stuffed.

  if (len < 2) {
    out << "Truncated PPP frame, length " << len;
    return true;
  }

  if (pkt[0] == 0xff && pkt[1] == 0x03) {
    // PPP in HDLC-like framing, skip the Address and Control fields
    if (len < 4) {
      out << "Truncated PPP frame, length " << len;
      return true;
    }
    pkt += 2;
    len -= 2;
  }

  unsigned int proto = pkt[0];
  if (proto & 0x1) {
    // Protocol field is compressed in 1 byte
    pkt += 1;
    len -= 1;
  }
  else {
    // Protocol field is 2 bytes, in network byte order
    proto = (proto << 8) | pkt[1];
    pkt += 2;
    len -= 2;
  }

  switch (proto) {
  case 0x0077: // NDN in PPP frame, used by ndnSIM pcap traces
    // For some reason, ndnSIM produces PPP frames with 2 extra bytes
    // between the protocol field and the beginning of the NDN packet
    if (len < 2) {
      out << "Truncated NDN/PPP frame";
      return true;
    }
    pkt += 2;
    len -= 2;
    out << "PPP";
    return printNdn(out, pkt, len);
  default:
    out << "[Unsupported PPP proto " << AsHex{proto} << "]";
    return true;
  }
}

static void
printIpAddress(OutputFormatter& out, int af, const void* addr)
{
  char addrStr[1 + std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)] = {};
  if (inet_ntop(af, addr, addrStr, sizeof(addrStr))) {
    out << addrStr;
  }
  else {
    out << "???";
  }
}

bool
NdnDump::printIp4(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  out.addDelimiter() << "IP ";

  if (len < sizeof(ip)) {
    out << "truncated header, length " << len;
    return true;
  }

  auto ih = reinterpret_cast<const ip*>(pkt);
  if (ih->ip_v != 4) {
    // huh? link layer said this was an IPv4 packet but IP header says otherwise
    out << "bad version " << ih->ip_v;
    return true;
  }

  size_t ipHdrLen = ih->ip_hl * 4;
  if (ipHdrLen < sizeof(ip)) {
    out << "bad header length " << ipHdrLen;
    return true;
  }

  size_t ipLen = endian::big_to_native(ih->ip_len);
  if (ipLen < ipHdrLen) {
    out << "bad length " << ipLen;
    return true;
  }
  if (ipLen > len) {
    out << "truncated packet, " << ipLen - len << " bytes missing";
    return true;
  }
  // if we reached this point, the following is true:
  //     sizeof(ip) <= ipHdrLen <= ipLen <= len

  printIpAddress(out, AF_INET, &ih->ip_src);
  out << " > ";
  printIpAddress(out, AF_INET, &ih->ip_dst);

  pkt += ipHdrLen;
  len -= ipHdrLen;

  return dispatchByIpProto(out, pkt, len, ih->ip_p);
}

bool
NdnDump::printIp6(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  out.addDelimiter() << "IP6 ";

  if (len < sizeof(ip6_hdr)) {
    out << "truncated header, length " << len;
    return true;
  }

  auto ip6 = reinterpret_cast<const ip6_hdr*>(pkt);
  unsigned int ipVer = (ip6->ip6_vfc & 0xf0) >> 4;
  if (ipVer != 6) {
    // huh? link layer said this was an IPv6 packet but IP header says otherwise
    out << "bad version " << ipVer;
    return true;
  }

  pkt += sizeof(ip6_hdr);
  len -= sizeof(ip6_hdr);

  size_t payloadLen = endian::big_to_native(ip6->ip6_plen);
  if (len < payloadLen) {
    out << "truncated payload, " << payloadLen - len << " bytes missing";
    return true;
  }

  printIpAddress(out, AF_INET6, &ip6->ip6_src);
  out << " > ";
  printIpAddress(out, AF_INET6, &ip6->ip6_dst);

  // we assume no extension headers are present
  return dispatchByIpProto(out, pkt, len, ip6->ip6_nxt);
}

bool
NdnDump::printTcp(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  out.addDelimiter() << "TCP";

  if (len < sizeof(tcphdr)) {
    out << " truncated header, length " << len;
    return true;
  }

  auto th = reinterpret_cast<const tcphdr*>(pkt);
  size_t tcpHdrLen = th->TH_OFF * 4;
  if (tcpHdrLen < sizeof(tcphdr)) {
    out << " bad header length " << tcpHdrLen;
    return true;
  }
  if (tcpHdrLen > len) {
    out << " truncated header, " << tcpHdrLen - len << " bytes missing";
    return true;
  }

  pkt += tcpHdrLen;
  len -= tcpHdrLen;

  out.addDelimiter() << "length " << len;

  return printNdn(out, pkt, len);
}

bool
NdnDump::printUdp(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  out.addDelimiter() << "UDP";

  if (len < sizeof(udphdr)) {
    out << " truncated header, length " << len;
    return true;
  }

  auto uh = reinterpret_cast<const udphdr*>(pkt);
  size_t udpLen = endian::big_to_native(uh->UH_LEN);
  if (udpLen < sizeof(udphdr)) {
    out << " bad length " << udpLen;
    return true;
  }
  if (udpLen > len) {
    out << " truncated packet, " << udpLen - len << " bytes missing";
    return true;
  }
  else {
    len = udpLen;
  }

  pkt += sizeof(udphdr);
  len -= sizeof(udphdr);

  out.addDelimiter() << "length " << len;

  return printNdn(out, pkt, len);
}

bool
NdnDump::printNdn(OutputFormatter& out, const uint8_t* pkt, size_t len) const
{
  if (len == 0) {
    return false;
  }
  out.addDelimiter();

  auto [isOk, block] = Block::fromBuffer({pkt, len});
  if (!isOk) {
    // if packet is incomplete, we will not be able to process it
    out << "NDN truncated packet, length " << len;
    return true;
  }

  lp::Packet lpPacket;
  Block netPacket;

  if (block.type() == lp::tlv::LpPacket) {
    out << "NDNLPv2";
    try {
      lpPacket.wireDecode(block);
    }
    catch (const tlv::Error& e) {
      out << " invalid packet: " << e.what();
      return true;
    }

    if (!lpPacket.has<lp::FragmentField>()) {
      out << " idle";
      return true;
    }

    auto [begin, end] = lpPacket.get<lp::FragmentField>();
    std::tie(isOk, netPacket) = Block::fromBuffer({begin, end});
    if (!isOk) {
      // if network packet is fragmented, we will not be able to process it
      out << " fragment";
      return true;
    }
  }
  else {
    netPacket = std::move(block);
  }
  out.addDelimiter();

  try {
    switch (netPacket.type()) {
      case tlv::Interest: {
        Interest interest(netPacket);
        if (!matchesFilter(interest.getName())) {
          return false;
        }

        if (lpPacket.has<lp::NackField>()) {
          lp::Nack nack(interest);
          nack.setHeader(lpPacket.get<lp::NackField>());
          out << "NACK (" << nack.getReason() << "): " << interest;
        }
        else {
          out << "INTEREST: " << interest;
        }
        break;
      }
      case tlv::Data: {
        Data data(netPacket);
        if (!matchesFilter(data.getName())) {
          return false;
        }

        out << "DATA: " << data.getName();
        break;
      }
      default: {
        out << "[Unsupported NDN packet type " << netPacket.type() << "]";
        break;
      }
    }
  }
  catch (const tlv::Error& e) {
    out << "invalid network packet: " << e.what();
  }

  return true;
}

bool
NdnDump::matchesFilter(const Name& name) const
{
  if (!nameFilter)
    return true;

  /// \todo Switch to NDN regular expressions
  return std::regex_match(name.toUri(), *nameFilter);
}

} // namespace ndn::dump
