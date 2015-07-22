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

#ifndef NDN_TOOLS_PIB_PIB_ENCODING_HPP
#define NDN_TOOLS_PIB_PIB_ENCODING_HPP

#include "pib-common.hpp"
#include <ndn-cxx/security/identity-certificate.hpp>

namespace ndn {
namespace pib {

/**
 * @brief Abstraction of pib::Identity TLV.
 *
 * This class is copyable since it is used by a variety of pib parameters
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Query-Responses
 */
class PibIdentity
{
public:
  PibIdentity();

  explicit
  PibIdentity(const Name& identity);

  explicit
  PibIdentity(const Block& wire);

  const Name&
  getIdentity() const
  {
    return m_identity;
  }

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibIdentity from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  Name m_identity;

  mutable Block m_wire;
};

/**
 * @brief Abstraction of pib::PublicKey TLV.
 *
 * This class is copyable since it is used by a variety of pib parameters
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Query-Responses
 */
class PibPublicKey
{
public:
  PibPublicKey();

  PibPublicKey(const Name& keyName, const PublicKey& key);

  explicit
  PibPublicKey(const Block& wire);

  const Name&
  getKeyName() const;

  const PublicKey&
  getPublicKey() const;

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibPublicKey from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  bool m_isValueSet;
  Name m_keyName;
  PublicKey m_key;

  mutable Block m_wire;
};

/**
 * @brief Abstraction of pib::Certificate TLV.
 *
 * This class is copyable since it is used by a variety of pib parameters
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Query-Responses
 */
class PibCertificate
{
public:
  PibCertificate();

  explicit
  PibCertificate(const IdentityCertificate& certificate);

  explicit
  PibCertificate(const Block& wire);

  const IdentityCertificate&
  getCertificate() const;

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibCertificate from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  bool m_isValueSet;
  IdentityCertificate m_certificate;

  mutable Block m_wire;
};

/**
 * @brief Abstraction of pib::NameList TLV.
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#List-Parameters
 */
class PibNameList : noncopyable
{
public:
  PibNameList();

  explicit
  PibNameList(const std::vector<Name>& names);

  explicit
  PibNameList(const Block& wire);

  const std::vector<Name>&
  getNameList() const
  {
    return m_names;
  }

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibCertificate from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  std::vector<Name> m_names;

  mutable Block m_wire;
};

/**
 * @brief Abstraction of pib::Error TLV.
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Query-Responses
 */
class PibError : noncopyable
{
public:
  PibError();

  explicit
  PibError(const pib::ErrCode errCode, const std::string& msg = "");

  explicit
  PibError(const Block& wire);

  pib::ErrCode
  getErrorCode() const
  {
    return m_errCode;
  }

  const std::string&
  getErrorMsg() const
  {
    return m_msg;
  }

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibCertificate from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  pib::ErrCode m_errCode;
  std::string m_msg;

  mutable Block m_wire;
};

/**
 * @brief Abstraction of pib::User TLV.
 *
 * This class is copyable since it is used by a variety of pib parameters
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Query-Responses
 */
class PibUser
{
public:
  PibUser();

  explicit
  PibUser(const Block& wire);

  void
  setMgmtCert(const IdentityCertificate& mgmtCert);

  const IdentityCertificate&
  getMgmtCert() const
  {
    return m_mgmtCert;
  }

  void
  setTpmLocator(const std::string& tpmLocator);

  const std::string&
  getTpmLocator() const
  {
    return m_tpmLocator;
  }

  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  const Block&
  wireEncode() const;

  /**
   * @brief Decode PibCertificate from a wire encoded block
   *
   * @throws tlv::Error if decoding fails
   */
  void
  wireDecode(const Block& wire);

private:
  IdentityCertificate m_mgmtCert;
  std::string m_tpmLocator;

  mutable Block m_wire;
};

} // namespace pib
} // namespace ndn

#endif // NDN_TOOLS_PIB_PIB_ENCODING_HPP
