/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2024, Regents of the University of California.
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
 * @author Alexander Afanasyev
 * @author Davide Pesavento
 */

#include "dissector.hpp"

#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/encoding/tlv-security.hpp>
#include <ndn-cxx/util/string-helper.hpp>

#include <iostream>
#include <map>

namespace ndn::dissect {

Dissector::Dissector(std::istream& input, std::ostream& output, const Options& options)
  : m_options(options)
  , m_in(input)
  , m_out(output)
{
}

void
Dissector::dissect()
{
  size_t offset = 0;
  try {
    while (m_in.peek() != std::istream::traits_type::eof()) {
      auto block = Block::fromStream(m_in);
      printBlock(block);
      offset += block.size();
    }
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << " at offset " << offset << "\n";
  }
}

// http://git.altlinux.org/people/legion/packages/kbd.git?p=kbd.git;a=blob;f=data/consolefonts/README.eurlatgr
static const char GLYPH_VERTICAL[]           = "\u2502 ";      // "│ "
static const char GLYPH_VERTICAL_AND_RIGHT[] = "\u251c\u2500"; // "├─"
static const char GLYPH_UP_AND_RIGHT[]       = "\u2514\u2500"; // "└─"
static const char GLYPH_SPACE[]              = "  ";

void
Dissector::printBranches()
{
  for (size_t i = 0; i < m_branches.size(); ++i) {
    if (i == m_branches.size() - 1) {
      m_out << (m_branches[i] ? GLYPH_VERTICAL_AND_RIGHT : GLYPH_UP_AND_RIGHT);
    }
    else {
      m_out << (m_branches[i] ? GLYPH_VERTICAL : GLYPH_SPACE);
    }
  }
}

// https://docs.named-data.net/NDN-packet-spec/current/types.html
static const std::map<uint32_t, std::string_view> TLV_DICT = {
  {tlv::Interest                        , "Interest"},
  {tlv::Data                            , "Data"},
  {tlv::Name                            , "Name"},
  // Name components
  {tlv::GenericNameComponent            , "GenericNameComponent"},
  {tlv::ImplicitSha256DigestComponent   , "ImplicitSha256DigestComponent"},
  {tlv::ParametersSha256DigestComponent , "ParametersSha256DigestComponent"},
  {tlv::KeywordNameComponent            , "KeywordNameComponent"},
  {tlv::SegmentNameComponent            , "SegmentNameComponent"},
  {tlv::ByteOffsetNameComponent         , "ByteOffsetNameComponent"},
  {tlv::VersionNameComponent            , "VersionNameComponent"},
  {tlv::TimestampNameComponent          , "TimestampNameComponent"},
  {tlv::SequenceNumNameComponent        , "SequenceNumNameComponent"},
  // Interest packet
  {tlv::CanBePrefix                     , "CanBePrefix"},
  {tlv::MustBeFresh                     , "MustBeFresh"},
  {tlv::ForwardingHint                  , "ForwardingHint"},
  {tlv::Nonce                           , "Nonce"},
  {tlv::InterestLifetime                , "InterestLifetime"},
  {tlv::HopLimit                        , "HopLimit"},
  {tlv::ApplicationParameters           , "ApplicationParameters"},
  {tlv::InterestSignatureInfo           , "InterestSignatureInfo"},
  {tlv::InterestSignatureValue          , "InterestSignatureValue"},
  // Data packet
  {tlv::MetaInfo                        , "MetaInfo"},
  {tlv::Content                         , "Content"},
  {tlv::SignatureInfo                   , "SignatureInfo"},
  {tlv::SignatureValue                  , "SignatureValue"},
  {tlv::ContentType                     , "ContentType"},
  {tlv::FreshnessPeriod                 , "FreshnessPeriod"},
  {tlv::FinalBlockId                    , "FinalBlockId"},
  // (Interest)SignatureInfo
  {tlv::SignatureType                   , "SignatureType"},
  {tlv::KeyLocator                      , "KeyLocator"},
  {tlv::KeyDigest                       , "KeyDigest"},
  {tlv::SignatureNonce                  , "SignatureNonce"},
  {tlv::SignatureTime                   , "SignatureTime"},
  {tlv::SignatureSeqNum                 , "SignatureSeqNum"},
  // Certificate
  {tlv::ValidityPeriod                  , "ValidityPeriod"},
  {tlv::NotBefore                       , "NotBefore"},
  {tlv::NotAfter                        , "NotAfter"},
  {tlv::AdditionalDescription           , "AdditionalDescription"},
  {tlv::DescriptionEntry                , "DescriptionEntry"},
  {tlv::DescriptionKey                  , "DescriptionKey"},
  {tlv::DescriptionValue                , "DescriptionValue"},
  // SafeBag
  {tlv::security::SafeBag               , "SafeBag"},
  {tlv::security::EncryptedKey          , "EncryptedKey"},
};

void
Dissector::printType(uint32_t type)
{
  m_out << type << " (";

  auto it = TLV_DICT.find(type);
  if (it != TLV_DICT.end()) {
    m_out << it->second;
  }
  else if ((type >= tlv::AppPrivateBlock1 && type <= 252) ||
           type >= tlv::AppPrivateBlock2) {
    m_out << "UNKNOWN_APP";
  }
  else {
    m_out << "RESERVED";
  }

  m_out << ")";
}

void
Dissector::printBlock(const Block& block)
{
  printBranches();
  printType(block.type());
  m_out << " (size: " << block.value_size() << ")";

  try {
    if (block.type() != tlv::SignatureValue &&
        block.type() != tlv::InterestSignatureValue &&
        (block.type() != tlv::Content || m_options.dissectContent)) {
      block.parse();
    }
  }
  catch (const tlv::Error&) {
    // pass (e.g., leaf block reached)
  }

  const auto& elements = block.elements();
  if (elements.empty()) {
    m_out << " [[";
    escape(m_out, {reinterpret_cast<const char*>(block.value()), block.value_size()});
    m_out << "]]";
  }
  m_out << "\n";

  m_branches.push_back(true);
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i == elements.size() - 1) {
      // no more branches to draw at this level of the tree
      m_branches.back() = false;
    }
    printBlock(elements[i]);
  }
  m_branches.pop_back();
}

} // namespace ndn::dissect
