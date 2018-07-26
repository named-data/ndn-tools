/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California.
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
/*
 * Copyright (c) 2011-2014, Regents of the University of California,
 *
 * This file is part of ndndump, the packet capture and analysis tool for Named Data
 * Networking (NDN).
 *
 * ndndump is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndndump is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndndump, e.g., in COPYING file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndndump.hpp"

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <pcap/sll.h>

#include <iomanip>
#include <sstream>

#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/net/ethernet.hpp>

namespace ndn {
namespace dump {

NdnDump::~NdnDump()
{
  if (m_pcap)
    pcap_close(m_pcap);
}

static void
pcapCallback(uint8_t* user, const pcap_pkthdr* pkthdr, const uint8_t* payload)
{
  reinterpret_cast<const NdnDump*>(user)->printPacket(pkthdr, payload);
}

void
NdnDump::run()
{
  char errbuf[PCAP_ERRBUF_SIZE];

  if (inputFile.empty() && interface.empty()) {
    const char* defaultDevice = pcap_lookupdev(errbuf);

    if (defaultDevice == nullptr) {
      BOOST_THROW_EXCEPTION(Error(errbuf));
    }

    interface = defaultDevice;
  }

  std::string action;
  if (!interface.empty()) {
    m_pcap = pcap_open_live(interface.data(), 65535, 0, 1000, errbuf);
    if (m_pcap == nullptr) {
      BOOST_THROW_EXCEPTION(Error("Cannot open interface " + interface + ": " + errbuf));
    }
    action = "listening on " + interface;
  }
  else {
    m_pcap = pcap_open_offline(inputFile.data(), errbuf);
    if (m_pcap == nullptr) {
      BOOST_THROW_EXCEPTION(Error("Cannot open file '" + inputFile + "' for reading: " + errbuf));
    }
    action = "reading from file " + inputFile;
  }

  m_dataLinkType = pcap_datalink(m_pcap);
  const char* dltName = pcap_datalink_val_to_name(m_dataLinkType);
  const char* dltDesc = pcap_datalink_val_to_description(m_dataLinkType);
  std::string formattedDlt = dltName ? dltName : to_string(m_dataLinkType);
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
    BOOST_THROW_EXCEPTION(Error("Unsupported link-layer header type " + formattedDlt));
  }

  if (!pcapFilter.empty()) {
    if (isVerbose) {
      std::cerr << "ndndump: using pcap filter: " << pcapFilter << std::endl;
    }

    bpf_program program;
    int res = pcap_compile(m_pcap, &program, pcapFilter.data(), 1, PCAP_NETMASK_UNKNOWN);
    if (res < 0) {
      BOOST_THROW_EXCEPTION(Error("Cannot compile pcap filter '" + pcapFilter + "': " + pcap_geterr(m_pcap)));
    }

    res = pcap_setfilter(m_pcap, &program);
    pcap_freecode(&program);
    if (res < 0) {
      BOOST_THROW_EXCEPTION(Error("Cannot set pcap filter: "s + pcap_geterr(m_pcap)));
    }
  }

  if (pcap_loop(m_pcap, -1, &pcapCallback, reinterpret_cast<uint8_t*>(this)) < 0) {
    BOOST_THROW_EXCEPTION(Error("pcap_loop: "s + pcap_geterr(m_pcap)));
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
  printTimestamp(os, pkthdr->ts);

  ssize_t payloadSize = pkthdr->len;

  int frameType = skipDataLinkHeaderAndGetFrameType(payload, payloadSize);
  if (frameType < 0) {
    return;
  }

  int res = skipAndProcessFrameHeader(frameType, payload, payloadSize, os);
  if (res < 0) {
    return;
  }

  bool isOk = false;
  Block block;
  std::tie(isOk, block) = Block::fromBuffer(payload, payloadSize);
  if (!isOk) {
    // if packet is incomplete, we will not be able to process it
    if (payloadSize > 0) {
      std::cout << os.str() << ", " << "INCOMPLETE-PACKET" << ", size: " << payloadSize << std::endl;
    }
    return;
  }

  lp::Packet lpPacket;
  Block netPacket;

  if (block.type() == lp::tlv::LpPacket) {
    lpPacket = lp::Packet(block);

    Buffer::const_iterator begin, end;
    if (lpPacket.has<lp::FragmentField>()) {
      std::tie(begin, end) = lpPacket.get<lp::FragmentField>();
    }
    else {
      std::cout << os.str() << ", " << "NDNLPv2-IDLE" << std::endl;
      return;
    }

    bool isOk = false;
    std::tie(isOk, netPacket) = Block::fromBuffer(&*begin, std::distance(begin, end));
    if (!isOk) {
      // if network packet is fragmented, we will not be able to process it
      std::cout << os.str() << ", " << "NDNLPv2-FRAGMENT" << std::endl;
      return;
    }
  }
  else {
    netPacket = block;
  }

  try {
    if (netPacket.type() == tlv::Interest) {
      Interest interest(netPacket);
      if (matchesFilter(interest.getName())) {
        if (lpPacket.has<lp::NackField>()) {
          lp::Nack nack(interest);
          nack.setHeader(lpPacket.get<lp::NackField>());
          std::cout << os.str() << ", " << "NACK: " << nack.getReason() << ", " << interest << std::endl;
        }
        else {
          std::cout << os.str() << ", " << "INTEREST: " << interest << std::endl;
        }
      }
    }
    else if (netPacket.type() == tlv::Data) {
      Data data(netPacket);
      if (matchesFilter(data.getName())) {
        std::cout << os.str() << ", " << "DATA: " << data.getName() << std::endl;
      }
    }
    else {
      std::cout << os.str() << ", " << "UNKNOWN-NETWORK-PACKET" << std::endl;
    }
  }
  catch (const tlv::Error& e) {
    std::cerr << e.what() << std::endl;
  }
}

void
NdnDump::printTimestamp(std::ostream& os, const timeval& tv) const
{
  /// \todo Add more timestamp formats (time since previous packet, time since first packet, ...)
  os << tv.tv_sec
     << "."
     << std::setfill('0') << std::setw(6) << tv.tv_usec
     << " ";
}

int
NdnDump::skipDataLinkHeaderAndGetFrameType(const uint8_t*& payload, ssize_t& payloadSize) const
{
  int frameType = -1;

  switch (m_dataLinkType) {
    case DLT_EN10MB: { // Ethernet frames can have Ethernet or 802.3 encapsulation
      const ether_header* eh = reinterpret_cast<const ether_header*>(payload);

      if (payloadSize < ETHER_HDR_LEN) {
        std::cerr << "Invalid Ethernet frame" << std::endl;
        return -1;
      }

      frameType = ntohs(eh->ether_type);
      payloadSize -= ETHER_HDR_LEN;
      payload += ETHER_HDR_LEN;

      break;
    }
    case DLT_PPP: {
      frameType = *payload;
      --payloadSize;
      ++payload;

      if (!(frameType & 1)) {
        frameType = (frameType << 8) | *payload;
        --payloadSize;
        ++payload;
      }

      if (payloadSize < 4) { // PPP_HDRLEN in linux/ppp_defs.h
        std::cerr << "Invalid PPP frame" << std::endl;
        return -1;
      }

      break;
    }
    case DLT_LINUX_SLL: {
      const sll_header* sll = reinterpret_cast<const sll_header*>(payload);

      if (payloadSize < SLL_HDR_LEN) {
        std::cerr << "Invalid LINUX_SLL frame" << std::endl;
        return -1;
      }

      frameType = ntohs(sll->sll_protocol);
      payloadSize -= SLL_HDR_LEN;
      payload += SLL_HDR_LEN;

      break;
    }
    default:
      std::cerr << "Unknown frame type" << std::endl;
      break;
  }

  return frameType;
}

int
NdnDump::skipAndProcessFrameHeader(int frameType, const uint8_t*& payload, ssize_t& payloadSize,
                                   std::ostream& os) const
{
  switch (frameType) {
    case ETHERTYPE_IP:
    case DLT_EN10MB: { // pcap encapsulation
      const ip* ipHeader = reinterpret_cast<const ip*>(payload);
      size_t ipHeaderSize = ipHeader->ip_hl * 4;
      if (ipHeaderSize < 20) {
        std::cerr << "Invalid IPv4 header len: " << ipHeaderSize << " bytes" << std::endl;
        return -1;
      }

      os << "From: " << inet_ntoa(ipHeader->ip_src) << ", ";
      os << "To: "   << inet_ntoa(ipHeader->ip_dst);

      payloadSize -= ipHeaderSize;
      payload += ipHeaderSize;

      if (payloadSize < 0) {
        std::cerr << "Invalid IPv4 packet" << std::endl;
        return -1;
      }

      switch (ipHeader->ip_p) {
        case IPPROTO_UDP: {
          payloadSize -= sizeof(udphdr);
          payload += sizeof(udphdr);

          if (payloadSize < 0) {
            std::cerr << "Invalid UDP/IP packet" << std::endl;
            return -1;
          }

          os << ", Tunnel Type: UDP";
          break;
        }
        case IPPROTO_TCP: {
          const tcphdr* tcpHeader = reinterpret_cast<const tcphdr*>(payload);
          size_t tcpHeaderSize = tcpHeader->th_off * 4;

          if (tcpHeaderSize < 20) {
            std::cerr << "Invalid TCP header len: " << tcpHeaderSize << " bytes" << std::endl;
            return -1;
          }

          payloadSize -= tcpHeaderSize;
          payload += tcpHeaderSize;

          if (payloadSize < 0) {
            std::cerr << "Invalid TCP/IP packet" << std::endl;
            return -1;
          }

          os << ", Tunnel Type: TCP";
          break;
        }
        default:
          return -1;
      }
      break;
    }
    case ethernet::ETHERTYPE_NDN:
    case 0x7777: // NDN ethertype used in ndnSIM
      os << "Tunnel Type: EthernetFrame";
      break;
    case 0x0077: // protocol field in PPP header used in ndnSIM
      os << "Tunnel Type: PPP";
      payloadSize -= 2;
      payload += 2;
      break;
    default: // do nothing if it is not a recognized type of a packet
      return -1;
  }

  return 0;
}

bool
NdnDump::matchesFilter(const Name& name) const
{
  if (!nameFilter)
    return true;

  /// \todo Switch to NDN regular expressions
  return std::regex_match(name.toUri(), *nameFilter);
}

} // namespace dump
} // namespace ndn
