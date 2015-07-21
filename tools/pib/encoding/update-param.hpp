/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 */

#ifndef NDN_PIB_UPDATE_PARAM_HPP
#define NDN_PIB_UPDATE_PARAM_HPP

#include "pib-common.hpp"
#include "pib-encoding.hpp"
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/identity-certificate.hpp>

namespace ndn {
namespace pib {

/**
 * @brief UpdateParam is the abstraction of PIB Update parameter.
 *
 * PibUpdateParam := PIB-UPDATE-PARAM-TYPE TLV-LENGTH
 *                   (PibIdentity | PibPublicKey | PibCertificate)
 *                   PibDefaultOpt
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Update-Parameters
 */

class UpdateParam : noncopyable
{
public:
  class Error : public tlv::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : tlv::Error(what)
    {
    }
  };

  UpdateParam();

  explicit
  UpdateParam(const PibUser& user);

  explicit
  UpdateParam(const Name& identity, DefaultOpt defaultOpt = DEFAULT_OPT_NO);

  UpdateParam(const Name& keyName, const PublicKey& key, DefaultOpt defaultOpt = DEFAULT_OPT_NO);

  explicit
  UpdateParam(const IdentityCertificate& certificate, DefaultOpt defaultOpt = DEFAULT_OPT_NO);

  explicit
  UpdateParam(const Block& wire);

  tlv::pib::ParamType
  getParamType() const
  {
    return tlv::pib::UpdateParam;
  }

  tlv::pib::EntityType
  getEntityType() const
  {
    return m_entityType;
  }

  const PibUser&
  getUser() const;

  const PibIdentity&
  getIdentity() const;

  const PibPublicKey&
  getPublicKey() const;

  const PibCertificate&
  getCertificate() const;

  pib::DefaultOpt
  getDefaultOpt() const
  {
    return m_defaultOpt;
  }

  /// @brief Encode to a wire format or estimate wire format
  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  /// @brief Encode to a wire format
  const Block&
  wireEncode() const;

  /**
   * @brief Decode GetParam from a wire encoded block
   *
   * @throws Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

public:
  static const std::string VERB;

private:
  tlv::pib::EntityType m_entityType;
  PibUser m_user;
  PibIdentity m_identity;
  PibPublicKey m_key;
  PibCertificate m_certificate;
  pib::DefaultOpt m_defaultOpt;

  mutable Block m_wire;
};

} // namespace pib
} // namespace ndn

#endif // NDN_PIB_UPDATE_PARAM_HPP
