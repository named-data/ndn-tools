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

#include "get-query-processor.hpp"
#include "encoding/pib-encoding.hpp"
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

const Name GetQueryProcessor::PIB_PREFIX("/localhost/pib");
const size_t GetQueryProcessor::GET_QUERY_LENGTH = 5;

GetQueryProcessor::GetQueryProcessor(PibDb& db)
  : m_db(db)
{
}

std::pair<bool, Block>
GetQueryProcessor::operator()(const Interest& interest)
{
  const Name& interestName = interest.getName();

  // handle pib query: /localhost/pib/[UserName]/get/param
  if (interestName.size() != GET_QUERY_LENGTH) {
    // malformed interest, discard
    return std::make_pair(false, Block());
  }

  GetParam param;

  try {
    param.wireDecode(interestName.get(OFFSET_PARAM).blockFromValue());
  }
  catch (const tlv::Error& e) {
    PibError error(ERR_WRONG_PARAM, "error in parsing param: " + std::string(e.what()));
    return std::make_pair(true, error.wireEncode());
  }

  switch (param.getTargetType()) {
  case TYPE_ID:
    return processGetIdQuery(param);
  case TYPE_KEY:
    return processGetKeyQuery(param);
  case TYPE_CERT:
    return processGetCertQuery(param);
  case TYPE_USER:
    if (interest.getName().get(2).toUri() != m_db.getOwnerName())
      return std::make_pair(false, Block());
    else
      return processGetUserQuery(param);
  default:
    {
      PibError error(ERR_WRONG_PARAM, "requested type is not supported:" +
                     boost::lexical_cast<std::string>(param.getTargetType()));
      return std::make_pair(true, error.wireEncode());
    }
  }
}

std::pair<bool, Block>
GetQueryProcessor::processGetIdQuery(const GetParam& param)
{
  if (!m_db.hasIdentity(param.getTargetName())) {
    PibError error(ERR_NON_EXISTING_ID, "requested id does not exist");
    return std::make_pair(true, error.wireEncode());
  }

  PibIdentity result(param.getTargetName());
  return std::make_pair(true, result.wireEncode());
}

std::pair<bool, Block>
GetQueryProcessor::processGetKeyQuery(const GetParam& param)
{
  if (param.getTargetName().empty()) {
    PibError error(ERR_WRONG_PARAM, "key name does not have id-component");
    return std::make_pair(true, error.wireEncode());
  }

  shared_ptr<PublicKey> key = m_db.getKey(param.getTargetName());
  if (key == nullptr) {
    PibError error(ERR_NON_EXISTING_KEY, "requested key does not exist");
    return std::make_pair(true, error.wireEncode());
  }

  PibPublicKey result(param.getTargetName(), *key);
  return std::make_pair(true, result.wireEncode());
}

std::pair<bool, Block>
GetQueryProcessor::processGetCertQuery(const GetParam& param)
{
  shared_ptr<IdentityCertificate> certificate = m_db.getCertificate(param.getTargetName());
  if (certificate == nullptr) {
    PibError error(ERR_NON_EXISTING_CERT, "requested certificate does not exist");
    return std::make_pair(true, error.wireEncode());
  }

  PibCertificate result(*certificate);
  return std::make_pair(true, result.wireEncode());
}

std::pair<bool, Block>
GetQueryProcessor::processGetUserQuery(const GetParam& param)
{
  PibUser result;
  result.setMgmtCert(*m_db.getMgmtCertificate());
  result.setTpmLocator(m_db.getTpmLocator());
  return std::make_pair(true, result.wireEncode());
}

} // namespace pib
} // namespace ndn
