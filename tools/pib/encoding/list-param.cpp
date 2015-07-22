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
 *
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "list-param.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

static_assert(std::is_base_of<tlv::Error, ListParam::Error>::value,
              "ListParam::Error must inherit from tlv::Error");

const std::string ListParam::VERB("list");

ListParam::ListParam()
  : m_originType(TYPE_USER)
{
}

ListParam::ListParam(const pib::Type originType, const Name& originName)
  : m_originType(originType)
  , m_originName(originName)
{
}

ListParam::ListParam(const Block& wire)
{
  wireDecode(wire);
}

const Name&
ListParam::getOriginName() const
{
  if (m_originType == TYPE_ID || m_originType == TYPE_KEY || m_originType == TYPE_CERT)
    return m_originName;
  else
    throw Error("ListParam::getOriginName: origin name does not exist");
}

template<bool T>
size_t
ListParam::wireEncode(EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  switch (m_originType) {
  case TYPE_ID:
  case TYPE_KEY:
  case TYPE_CERT:
    {
      totalLength += m_originName.wireEncode(block);
      break;
    }
  case TYPE_USER:
    break;
  default:
    throw Error("ListParam::wireEncode: unsupported PibType: " +
                boost::lexical_cast<std::string>(m_originType));
  }

  totalLength += prependNonNegativeIntegerBlock(block, tlv::pib::Type, m_originType);
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::pib::ListParam);

  return totalLength;
}

template size_t
ListParam::wireEncode<true>(EncodingImpl<true>& block) const;

template size_t
ListParam::wireEncode<false>(EncodingImpl<false>& block) const;

const Block&
ListParam::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  return m_wire;
}

void
ListParam::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Error("The supplied block does not contain wire format");
  }

  m_wire = wire;
  m_wire.parse();

  if (m_wire.type() != tlv::pib::ListParam)
    throw Error("Unexpected TLV type when decoding ListParam");

  Block::element_const_iterator it = m_wire.elements_begin();

  // the first block must be PibType
  if (it != m_wire.elements_end() && it->type() == tlv::pib::Type) {
    m_originType = static_cast<pib::Type>(readNonNegativeInteger(*it));
    it++;
  }
  else
    throw Error("ListParam requires the first sub-TLV to be PibType");

  switch (m_originType) {
  case TYPE_ID:
  case TYPE_KEY:
  case TYPE_CERT:
    {
      if (it != m_wire.elements_end()) {
        // the second block, if exists, must be Name
        m_originName.wireDecode(*it);
        return;
      }
      else {
        throw Error("ListParam requires the second sub-TLV to be Name");
      }
    }
  case TYPE_USER:
    return;
  default:
    throw Error("ListParam::wireDecode: unsupported PibType: " +
                boost::lexical_cast<std::string>(m_originType));
  }
}

} // namespace pib
} // namespace ndn
