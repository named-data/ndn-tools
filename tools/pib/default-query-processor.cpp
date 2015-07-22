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

#include "default-query-processor.hpp"
#include "encoding/pib-encoding.hpp"

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

using std::string;

const size_t DefaultQueryProcessor::DEFAULT_QUERY_LENGTH = 5;

DefaultQueryProcessor::DefaultQueryProcessor(PibDb& db)
  : m_db(db)
{
}

std::pair<bool, Block>
DefaultQueryProcessor::operator()(const Interest& interest)
{
  const Name& interestName = interest.getName();

  // handle pib query: /localhost/pib/[UserName]/default/param
  if (interestName.size() != DEFAULT_QUERY_LENGTH) {
    // malformed interest, discard
    return std::make_pair(false, Block());
  }

  DefaultParam param;

  try {
    param.wireDecode(interestName.get(OFFSET_PARAM).blockFromValue());
  }
  catch (const tlv::Error& e) {
    PibError error(ERR_WRONG_PARAM, "error in parsing param: " + string(e.what()));
    return std::make_pair(true, error.wireEncode());
  }

  switch (param.getTargetType()) {
  case TYPE_ID:
    return processDefaultIdQuery(param);
  case TYPE_KEY:
    return processDefaultKeyQuery(param);
  case TYPE_CERT:
    return processDefaultCertQuery(param);
  default:
    {
      PibError error(ERR_WRONG_PARAM,
                     "target type is not supported: " +
                     boost::lexical_cast<string>(param.getTargetType()));
      return std::make_pair(true, error.wireEncode());
    }
  }
}

std::pair<bool, Block>
DefaultQueryProcessor::processDefaultIdQuery(const DefaultParam& param)
{
  if (param.getOriginType() == TYPE_USER) {
    try {
      PibIdentity result(m_db.getDefaultIdentity());
      return std::make_pair(true, result.wireEncode());
    }
    catch (PibDb::Error&) {
      PibError error(ERR_NO_DEFAULT_ID, "default identity does not exist");
      return std::make_pair(true, error.wireEncode());
    }
  }
  else {
    PibError error(ERR_WRONG_PARAM,
                   "origin type of id target must be USER(0), but gets: " +
                   boost::lexical_cast<string>(param.getOriginType()));
    return std::make_pair(true, error.wireEncode());
  }
}

std::pair<bool, Block>
DefaultQueryProcessor::processDefaultKeyQuery(const DefaultParam& param)
{
  Name identity;
  if (param.getOriginType() == TYPE_ID)
    identity = param.getOriginName();
  else if (param.getOriginType() == TYPE_USER) {
    try {
      identity = m_db.getDefaultIdentity();
    }
    catch (PibDb::Error&) {
      PibError error(ERR_NO_DEFAULT_ID, "default identity does not exist");
      return std::make_pair(true, error.wireEncode());
    }
  }
  else {
    PibError error(ERR_WRONG_PARAM,
                   "origin type of key target must be ID(1) or USER(0), but gets: " +
                   boost::lexical_cast<string>(param.getOriginType()));
    return std::make_pair(true, error.wireEncode());
  }

  try {
    Name keyName = m_db.getDefaultKeyNameOfIdentity(identity);
    shared_ptr<PublicKey> key = m_db.getKey(keyName);

    if (key == nullptr) {
      PibError error(ERR_NO_DEFAULT_KEY, "default key does not exist");
      return std::make_pair(true, error.wireEncode());
    }

    PibPublicKey result(keyName, *key);
    return std::make_pair(true, result.wireEncode());
  }
  catch (PibDb::Error& e) {
    PibError error(ERR_NO_DEFAULT_KEY,
                   "default key does not exist: " + string(e.what()));
    return std::make_pair(true, error.wireEncode());
  }
}

std::pair<bool, Block>
DefaultQueryProcessor::processDefaultCertQuery(const DefaultParam& param)
{
  Name identity;
  if (param.getOriginType() == TYPE_USER) {
    try {
      identity = m_db.getDefaultIdentity();
    }
    catch (PibDb::Error&) {
      PibError error(ERR_NO_DEFAULT_ID, "default identity does not exist");
      return std::make_pair(true, error.wireEncode());
    }
  }
  else if (param.getOriginType() == TYPE_ID)
    identity = param.getOriginName();
  else if (param.getOriginType() != TYPE_KEY) {
    PibError error(ERR_WRONG_PARAM,
                   "origin type of cert target must be KEY(2), ID(1) or USER(0), but gets: " +
                   boost::lexical_cast<string>(param.getOriginType()));
    return std::make_pair(true, error.wireEncode());
  }

  Name keyName;
  if (param.getOriginType() == TYPE_KEY) {
    keyName = param.getOriginName();
    if (keyName.size() < 1) {
      PibError error(ERR_WRONG_PARAM,
                     "key name must contain key id component");
      return std::make_pair(true, error.wireEncode());
    }
  }
  else {
    try {
      keyName = m_db.getDefaultKeyNameOfIdentity(identity);
    }
    catch (PibDb::Error&) {
      PibError error(ERR_NO_DEFAULT_KEY, "default key does not exist");
      return std::make_pair(true, error.wireEncode());
    }
  }

  try {
    Name certName = m_db.getDefaultCertNameOfKey(keyName);
    shared_ptr<IdentityCertificate> certificate = m_db.getCertificate(certName);

    if (certificate == nullptr) {
      PibError error (ERR_NO_DEFAULT_CERT, "default cert does not exist");
      return std::make_pair(true, error.wireEncode());
    }

    PibCertificate result(*certificate);
    return std::make_pair(true, result.wireEncode());
  }
  catch (PibDb::Error&) {
    PibError error(ERR_NO_DEFAULT_CERT, "default cert does not exist");
    return std::make_pair(true, error.wireEncode());
  }
}

} // namespace pib
} // namespace ndn
