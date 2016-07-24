/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California.
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
/**
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

#include <ndn-cxx/name.hpp>
#include <boost/regex.hpp>

namespace ndn {
namespace dump {

class Ndndump : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  Ndndump();

  void
  run();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  onCapturedPacket(const pcap_pkthdr* header, const uint8_t* packet) const;

private:
  static void
  onCapturedPacket(uint8_t* userData, const pcap_pkthdr* header, const uint8_t* packet)
  {
    reinterpret_cast<const Ndndump*>(userData)->onCapturedPacket(header, packet);
  }

  void
  printInterceptTime(std::ostream& os, const pcap_pkthdr* header) const;

  int
  skipDataLinkHeaderAndGetFrameType(const uint8_t*& payload, ssize_t& payloadSize) const;

  int
  skipAndProcessFrameHeader(int frameType,
                            const uint8_t*& payload, ssize_t& payloadSize,
                            std::ostream& os) const;

  bool
  matchesFilter(const Name& name) const;

public:
  bool isVerbose;
  // bool isSuccinct;
  // bool isMatchInverted;
  // bool shouldPrintStructure;
  // bool isTcpOnly;
  // bool isUdpOnly;

  std::string pcapProgram;
  std::string interface;
  boost::regex nameFilter;
  std::string inputFile;
  // std::string outputFile;

private:
  pcap_t* m_pcap;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  int m_dataLinkType;
};


} // namespace dump
} // namespace ndn

#endif // NDN_TOOLS_DUMP_NDNDUMP_HPP
