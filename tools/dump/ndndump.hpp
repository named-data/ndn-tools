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

#ifndef NDN_TOOLS_DUMP_NDNDUMP_HPP
#define NDN_TOOLS_DUMP_NDNDUMP_HPP

#include "core/common.hpp"

#include <pcap.h>
#include <regex>

namespace ndn {
namespace dump {

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

  static constexpr const char*
  getDefaultPcapFilter() noexcept
  {
    return "(ether proto 0x8624) or (tcp port 6363) or (udp port 6363)";
  }

private:
  void
  printTimestamp(std::ostream& os, const timeval& tv) const;

  int
  skipDataLinkHeaderAndGetFrameType(const uint8_t*& payload, ssize_t& payloadSize) const;

  int
  skipAndProcessFrameHeader(int frameType, const uint8_t*& payload, ssize_t& payloadSize,
                            std::ostream& os) const;

  bool
  matchesFilter(const Name& name) const;

public: // options
  std::string interface;
  std::string inputFile;
  std::string pcapFilter = getDefaultPcapFilter();
  optional<std::regex> nameFilter;
  bool isVerbose = false;

private:
  pcap_t* m_pcap = nullptr;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  int m_dataLinkType = -1;
};

} // namespace dump
} // namespace ndn

#endif // NDN_TOOLS_DUMP_NDNDUMP_HPP
