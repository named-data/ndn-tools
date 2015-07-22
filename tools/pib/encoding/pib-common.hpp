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

#ifndef NDN_TOOLS_PIB_PIB_COMMON_HPP
#define NDN_TOOLS_PIB_PIB_COMMON_HPP

namespace ndn {
namespace pib {

/// @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base

enum {
  OFFSET_USER              = 2,
  OFFSET_VERB              = 3,
  OFFSET_PARAM             = 4,
  OFFSET_TIMESTAMP         = 5,
  OFFSET_NONCE             = 6,
  OFFSET_SIG_INFO          = 7,
  OFFSET_SIG_VALUE         = 8,

  MIN_PIB_INTEREST_SIZE    = 5,
  SIGNED_PIB_INTEREST_SIZE = 9
};

enum Type {
  TYPE_USER    = 0,
  TYPE_ID      = 1,
  TYPE_KEY     = 2,
  TYPE_CERT    = 3,
  TYPE_DEFAULT = 255
};

enum DefaultOpt {
  DEFAULT_OPT_NO   = 0,
  DEFAULT_OPT_KEY  = 1, // 0x01
  DEFAULT_OPT_ID   = 3, // 0x03
  DEFAULT_OPT_USER = 7 // 0x07
};

enum DefaultOptMask {
  DEFAULT_OPT_KEY_MASK  = 0x1,
  DEFAULT_OPT_ID_MASK   = 0x2,
  DEFAULT_OPT_USER_MASK = 0x4
};

enum ErrCode {
  ERR_SUCCESS = 0,

  // Invalid query
  ERR_INCOMPLETE_COMMAND = 1,
  ERR_WRONG_VERB         = 2,
  ERR_WRONG_PARAM        = 3,
  ERR_WRONG_SIGNER       = 4,
  ERR_INTERNAL_ERROR     = 5,

  // Non existing entity
  ERR_NON_EXISTING_USER = 128,
  ERR_NON_EXISTING_ID   = 129,
  ERR_NON_EXISTING_KEY  = 130,
  ERR_NON_EXISTING_CERT = 131,

  // No default setting
  ERR_NO_DEFAULT_ID     = 256,
  ERR_NO_DEFAULT_KEY    = 257,
  ERR_NO_DEFAULT_CERT   = 258,

  // Delete default setting
  ERR_DELETE_DEFAULT_SETTING = 384
};

} // namespace pib


namespace tlv {
namespace pib {

enum ParamType {
  GetParam          = 128, // 0x80
  DefaultParam      = 129, // 0x81
  ListParam         = 130, // 0x82
  UpdateParam       = 131, // 0x83
  DeleteParam       = 132  // 0x84
};

enum EntityType {
  User              = 144, // 0x90
  Identity          = 145, // 0x91
  PublicKey         = 146, // 0x92
  Certificate       = 147  // 0x93
};

// Other
enum {
  Bytes             = 148, // 0x94
  DefaultOpt        = 149, // 0x95
  NameList          = 150, // 0x96
  Type              = 151, // 0x97
  Error             = 152, // 0x98
  TpmLocator        = 153, // 0x99

  // ErrorCode
  ErrorCode         = 252  // 0xfc
};

} // namespace pib
} // namespace tlv
} // namespace ndn


#endif // NDN_TOOLS_PIB_PIB_COMMON_HPP
