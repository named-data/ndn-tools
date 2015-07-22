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

#include "delete-param.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

static_assert(std::is_base_of<tlv::Error, DeleteParam::Error>::value,
              "DeleteParam::Error must inherit from tlv::Error");

const std::string DeleteParam::VERB("delete");

DeleteParam::DeleteParam()
  : m_targetType(TYPE_DEFAULT)
{
}

DeleteParam::DeleteParam(const Name& name, pib::Type type)
  : m_targetType(type)
  , m_targetName(name)
{
}

DeleteParam::DeleteParam(const Block& wire)
{
  wireDecode(wire);
}

template<bool T>
size_t
DeleteParam::wireEncode(EncodingImpl<T>& block) const
{
  if (m_targetType != TYPE_ID &&
      m_targetType != TYPE_KEY &&
      m_targetType != TYPE_CERT &&
      m_targetType != TYPE_USER)
    throw Error("DeleteParam::wireEncode: Wrong Type value: " +
                boost::lexical_cast<std::string>(m_targetType));

  size_t totalLength = 0;

  totalLength += m_targetName.wireEncode(block);
  totalLength += prependNonNegativeIntegerBlock(block, tlv::pib::Type, m_targetType);
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::pib::DeleteParam);

  return totalLength;
}

template size_t
DeleteParam::wireEncode<true>(EncodingImpl<true>& block) const;

template size_t
DeleteParam::wireEncode<false>(EncodingImpl<false>& block) const;

const Block&
DeleteParam::wireEncode() const
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
DeleteParam::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Error("The supplied block does not contain wire format");
  }

  m_wire = wire;
  m_wire.parse();

  if (m_wire.type() != tlv::pib::DeleteParam)
    throw Error("Unexpected TLV type when decoding DeleteParam");

  Block::element_const_iterator it = m_wire.elements_begin();

  // the first block must be Type
  if (it != m_wire.elements_end() && it->type() == tlv::pib::Type) {
    m_targetType = static_cast<pib::Type>(readNonNegativeInteger(*it));
    it++;
  }
  else
    throw Error("DeleteParam requires the first sub-TLV to be Type");

  // the second block must be Name
  if (it != m_wire.elements_end() && it->type() == tlv::Name) {
    m_targetName.wireDecode(*it);
    it++;
  }
  else
    throw Error("DeleteParam requires the second sub-TLV to be Name");

  if (it != m_wire.elements_end())
    throw Error("DeleteParam must not contain more than two sub-TLVs");
}

} // namespace pib
} // namespace ndn
