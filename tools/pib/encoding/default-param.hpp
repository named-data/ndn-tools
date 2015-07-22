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

#ifndef NDN_TOOLS_PIB_DEFAULT_PARAM_HPP
#define NDN_TOOLS_PIB_DEFAULT_PARAM_HPP

#include "pib-common.hpp"
#include <ndn-cxx/name.hpp>

namespace ndn {
namespace pib {

/**
 * @brief DefaultParam is the abstraction of PIB Default parameter.
 *
 *  PibDefaultParam := PIB-DEFAULT-PARAM-TYPE TLV-LENGTH
 *                     PibType    // target type
 *                     PibType    // origin type
 *                     Name?      // origin name
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base#Default-Parameters
 */

class DefaultParam : noncopyable
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

  DefaultParam();

  DefaultParam(const pib::Type targetType,
               const pib::Type originType,
               const Name& originName = Name());

  explicit
  DefaultParam(const Block& wire);

  tlv::pib::ParamType
  getParamType() const
  {
    return tlv::pib::DefaultParam;
  }

  pib::Type
  getTargetType() const
  {
    return m_targetType;
  }

  pib::Type
  getOriginType() const
  {
    return m_originType;
  }

  /**
   * @brief Get target name
   *
   * @throws Error if origin name does not exist
   */
  const Name&
  getOriginName() const;

  /// @brief Encode to a wire format or estimate wire format
  template<bool T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  /**
   * @brief Encode to a wire format
   *
   * @throws Error if encoding fails
   */
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
  pib::Type m_targetType;
  pib::Type m_originType;
  Name m_originName;

  mutable Block m_wire;
};

} // namespace pib
} // namespace ndn



#endif // NDN_TOOLS_PIB_DEFAULT_PARAM_HPP
