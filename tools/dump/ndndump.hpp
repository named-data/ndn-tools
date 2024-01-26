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

#ifndef NDN_TOOLS_DUMP_NDNDUMP_HPP
#define NDN_TOOLS_DUMP_NDNDUMP_HPP

#include "core/common.hpp"

#include <pcap.h>

#include <optional>
#include <regex>

#ifdef HAVE_BSD_TCPHDR
#define TH_OFF th_off
#else
#define TH_OFF doff
#endif

#ifdef HAVE_BSD_UDPHDR
#define UH_LEN uh_ulen
#else
#define UH_LEN len
#endif

namespace ndn::dump {

class OutputFormatter;

class NdnDump : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  ~NdnDump();

  void
  run();

  void
  printPacket(const pcap_pkthdr* pkthdr, const uint8_t* payload) const;

  static constexpr std::string_view
  getDefaultPcapFilter() noexcept
  {
    return "(ether proto 0x8624) or (tcp port 6363) or (udp port 6363) or (udp port 56363)";
  }

private:
  static void
  printTimestamp(std::ostream& os, const timeval& tv);

  bool
  dispatchByEtherType(OutputFormatter& out, const uint8_t* pkt, size_t len, uint16_t etherType) const;

  bool
  dispatchByIpProto(OutputFormatter& out, const uint8_t* pkt, size_t len, uint8_t ipProto) const;

  bool
  printEther(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printLinuxSll(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printPpp(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printIp4(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printIp6(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printTcp(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printUdp(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  bool
  printNdn(OutputFormatter& out, const uint8_t* pkt, size_t len) const;

  [[nodiscard]] bool
  matchesFilter(const Name& name) const;

public: // options
  std::string interface;
  std::string inputFile;
  std::string pcapFilter{getDefaultPcapFilter()};
  std::optional<std::regex> nameFilter;
  bool wantPromisc = true;
  bool wantTimestamp = true;
  bool wantVerbose = false;

private:
  pcap_t* m_pcap = nullptr;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  int m_dataLinkType = -1;
};

} // namespace ndn::dump

#endif // NDN_TOOLS_DUMP_NDNDUMP_HPP
