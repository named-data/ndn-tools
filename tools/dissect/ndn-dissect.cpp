/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California.
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
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#include "ndn-dissect.hpp"

#include <algorithm>
#include <map>

#include <ndn-cxx/name-component.hpp>
#include <ndn-cxx/util/indented-stream.hpp>

namespace ndn {
namespace dissect {

std::map<uint32_t, std::string> TLV_DICT = {
  {tlv::Interest                     , "Interest"},
  {tlv::Data                         , "Data"},
  {tlv::Name                         , "Name"},
  {tlv::NameComponent                , "NameComponent"},
  {tlv::ImplicitSha256DigestComponent, "ImplicitSha256DigestComponent"},
  {tlv::Selectors                    , "Selectors"},
  {tlv::Nonce                        , "Nonce"},
  {tlv::InterestLifetime             , "InterestLifetime"},
  {tlv::MinSuffixComponents          , "MinSuffixComponents"},
  {tlv::MaxSuffixComponents          , "MaxSuffixComponents"},
  {tlv::PublisherPublicKeyLocator    , "PublisherPublicKeyLocator"},
  {tlv::Exclude                      , "Exclude"},
  {tlv::ChildSelector                , "ChildSelector"},
  {tlv::MustBeFresh                  , "MustBeFresh"},
  {tlv::Any                          , "Any"},
  {tlv::MetaInfo                     , "MetaInfo"},
  {tlv::Content                      , "Content"},
  {tlv::SignatureInfo                , "SignatureInfo"},
  {tlv::SignatureValue               , "SignatureValue"},
  {tlv::ContentType                  , "ContentType"},
  {tlv::FreshnessPeriod              , "FreshnessPeriod"},
  {tlv::FinalBlockId                 , "FinalBlockId"},
  {tlv::SignatureType                , "SignatureType"},
  {tlv::KeyLocator                   , "KeyLocator"},
  {tlv::KeyDigest                    , "KeyDigest"},
};

void
NdnDissect::printType(std::ostream& os, uint32_t type)
{
  os << type << " (";

  if (TLV_DICT.count(type) != 0) {
    os << TLV_DICT[type];
  }
  else if (type < tlv::AppPrivateBlock1) {
    os << "RESERVED_1";
  }
  else if (tlv::AppPrivateBlock1 <= type && type < 253) {
    os << "APP_TAG_1";
  }
  else if (253 <= type && type < tlv::AppPrivateBlock2) {
    os << "RESERVED_3";
  }
  else {
    os << "APP_TAG_3";
  }
  os << ")";
}

void
NdnDissect::printBlock(std::ostream& os, const Block& block)
{
  this->printType(os, block.type());
  os << " (size: " << block.value_size() << ")";

  try {
    // if (block.type() != tlv::Content && block.type() != tlv::SignatureValue)
    block.parse();
  }
  catch (tlv::Error& e) {
    // pass (e.g., leaf block reached)

    // @todo: Figure how to deterministically figure out that value is not recursive TLV block
  }

  if (block.elements().empty()) {
    os << " [[";
    name::Component(block.value(), block.value_size()).toUri(os);
    os<< "]]";
  }
  os << std::endl;

  util::IndentedStream os2(os, "  ");
  std::for_each(block.elements_begin(), block.elements_end(),
    [this, &os2] (const Block& element) {
      this->printBlock(os2, element);
    });
}

void
NdnDissect::dissect(std::ostream& os, std::istream& is)
{
  while (is.peek() != std::char_traits<char>::eof()) {
    try {
      Block block = Block::fromStream(is);
      this->printBlock(os, block);
    }
    catch (std::exception& e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
    }
  }
}

} // namespace dissect
} // namespace ndn
