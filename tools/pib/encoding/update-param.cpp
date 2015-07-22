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

#include "update-param.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

static_assert(std::is_base_of<tlv::Error, UpdateParam::Error>::value,
              "UpdateParam::Error must inherit from tlv::Error");

const std::string UpdateParam::VERB("update");

UpdateParam::UpdateParam()
  : m_defaultOpt(DEFAULT_OPT_NO)
{
}

UpdateParam::UpdateParam(const PibUser& user)
  : m_entityType(tlv::pib::User)
  , m_user(user)
  , m_defaultOpt(DEFAULT_OPT_NO)
{
}

UpdateParam::UpdateParam(const Name& identity, DefaultOpt defaultOpt)
  : m_entityType(tlv::pib::Identity)
  , m_identity(identity)
  , m_defaultOpt(defaultOpt)
{
}

UpdateParam::UpdateParam(const Name& keyName, const PublicKey& key, DefaultOpt defaultOpt)
  : m_entityType(tlv::pib::PublicKey)
  , m_key(keyName, key)
  , m_defaultOpt(defaultOpt)
{
}

UpdateParam::UpdateParam(const IdentityCertificate& certificate, DefaultOpt defaultOpt)
  : m_entityType(tlv::pib::Certificate)
  , m_certificate(certificate)
  , m_defaultOpt(defaultOpt)
{
}

UpdateParam::UpdateParam(const Block& wire)
{
  wireDecode(wire);
}

const PibUser&
UpdateParam::getUser() const
{
  if (m_entityType == tlv::pib::User)
      return m_user;
  else
    throw Error("UpdateParam::getUser: entityType must be User");
}

const PibIdentity&
UpdateParam::getIdentity() const
{
  if (m_entityType == tlv::pib::Identity)
    return m_identity;
  else
    throw Error("UpdateParam::getIdentity: entityType must be Identity");
}

const PibPublicKey&
UpdateParam::getPublicKey() const
{
  if (m_entityType == tlv::pib::PublicKey)
    return m_key;
  else
    throw Error("UpdateParam::getPublicKey: entityType must be PublicKey");
}

const PibCertificate&
UpdateParam::getCertificate() const
{
  if (m_entityType == tlv::pib::Certificate)
    return m_certificate;
  else
    throw Error("UpdateParam::getCertificate: entityType must be Certificate");
}

template<bool T>
size_t
UpdateParam::wireEncode(EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  totalLength += prependNonNegativeIntegerBlock(block, tlv::pib::DefaultOpt, m_defaultOpt);

  // Encode Entity
  switch (m_entityType) {
  case tlv::pib::Identity:
    {
      totalLength += m_identity.wireEncode(block);
      break;
    }
  case tlv::pib::PublicKey:
    {
      totalLength += m_key.wireEncode(block);
      break;
    }
  case tlv::pib::Certificate:
    {
      totalLength += m_certificate.wireEncode(block);
      break;
    }
  case tlv::pib::User:
    {
      totalLength += m_user.wireEncode(block);
      break;
    }
  default:
    throw Error("UpdateParam::wireEncode: unsupported entity type: " +
                boost::lexical_cast<std::string>(m_entityType));
  }

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::pib::UpdateParam);

  return totalLength;
}

template size_t
UpdateParam::wireEncode<true>(EncodingImpl<true>& block) const;

template size_t
UpdateParam::wireEncode<false>(EncodingImpl<false>& block) const;

const Block&
UpdateParam::wireEncode() const
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
UpdateParam::wireDecode(const Block& wire)
{
  if (!wire.hasWire()) {
    throw Error("The supplied block does not contain wire format");
  }

  m_wire = wire;
  m_wire.parse();

  if (m_wire.type() != tlv::pib::UpdateParam)
    throw Error("Unexpected TLV type when decoding UpdateParam");

  Block::element_const_iterator it = m_wire.elements_begin();

  // the first block must be Entity
  if (it != m_wire.elements_end()) {
    switch (it->type()) {
    case tlv::pib::Identity:
      {
        m_entityType = tlv::pib::Identity;
        m_identity.wireDecode(*it);
        break;
      }
    case tlv::pib::PublicKey:
      {
        m_entityType = tlv::pib::PublicKey;
        m_key.wireDecode(*it);
        break;
      }
    case tlv::pib::Certificate:
      {
        m_entityType = tlv::pib::Certificate;
        m_certificate.wireDecode(*it);
        break;
      }
    case tlv::pib::User:
      {
        m_entityType = tlv::pib::User;
        m_user.wireDecode(*it);
        break;
      }
    default:
      throw Error("The first sub-TLV of UpdateParam is not an entity type");
    }

    it++;
  }
  else
    throw Error("UpdateParam requires the first sub-TLV to be an entity type");

  // the second block must be DefaultOpt
  if (it != m_wire.elements_end() && it->type() == tlv::pib::DefaultOpt) {
    m_defaultOpt = static_cast<pib::DefaultOpt>(readNonNegativeInteger(*it));
    it++;
  }
  else
    throw Error("UpdateParam requires the second sub-TLV to be DefaultOpt");

  if (it != m_wire.elements_end())
    throw Error("UpdateParam must not contain more than two sub-TLVs");
}

} // namespace pib
} // namespace ndn
