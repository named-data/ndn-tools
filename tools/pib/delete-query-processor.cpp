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

#include "delete-query-processor.hpp"
#include "encoding/pib-encoding.hpp"
#include "pib.hpp"

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

using std::string;

const size_t DeleteQueryProcessor::DELETE_QUERY_LENGTH = 9;
const Name DeleteQueryProcessor::PIB_PREFIX("/localhost/pib");

DeleteQueryProcessor::DeleteQueryProcessor(PibDb& db)
  : m_db(db)
{
}

std::pair<bool, Block>
DeleteQueryProcessor::operator()(const Interest& interest)
{
  const Name& interestName = interest.getName();

  // handle pib query: /localhost/pib/[UserName]/delete/param/<signed_interest_related_components>
  if (interestName.size() != DELETE_QUERY_LENGTH) {
    // malformed interest, discard
    return std::make_pair(false, Block());
  }

  try {
    DeleteParam param;
    param.wireDecode(interestName.get(OFFSET_PARAM).blockFromValue());

    SignatureInfo sigInfo;
    sigInfo.wireDecode(interestName.get(OFFSET_SIG_INFO).blockFromValue());

    // sigInfo must have KeyLocator.Name if the interest passed validation.
    Name signerName;
    signerName = sigInfo.getKeyLocator().getName();

    switch (param.getTargetType()) {
    case TYPE_USER:
      return std::make_pair(true, PibError(ERR_WRONG_PARAM, "not allowed to delete user").wireEncode());
    case TYPE_ID:
      return processDeleteIdQuery(param, signerName);
    case TYPE_KEY:
      return processDeleteKeyQuery(param, signerName);
    case TYPE_CERT:
      return processDeleteCertQuery(param, signerName);
    default:
      {
        PibError error(ERR_WRONG_PARAM,
                       "target type is not supported: " +
                       boost::lexical_cast<string>(param.getTargetType()));
        return std::make_pair(true, error.wireEncode());
      }
    }
  }
  catch (const PibDb::Error& e) {
    PibError error(ERR_INTERNAL_ERROR, e.what());
    return std::make_pair(true, error.wireEncode());
  }
  catch (const tlv::Error& e) {
    PibError error(ERR_WRONG_PARAM, "error in parsing param: " + string(e.what()));
    return std::make_pair(true, error.wireEncode());
  }
}

std::pair<bool, Block>
DeleteQueryProcessor::processDeleteIdQuery(const DeleteParam& param, const Name& signerName)
{
  Name identity = param.getTargetName();
  if (!isDeleteAllowed(TYPE_ID, identity, signerName)) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  m_db.deleteIdentity(identity);

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

std::pair<bool, Block>
DeleteQueryProcessor::processDeleteKeyQuery(const DeleteParam& param, const Name& signerName)
{
  const Name& keyName = param.getTargetName();
  if (!isDeleteAllowed(TYPE_KEY, keyName, signerName)) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  m_db.deleteKey(keyName);

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

std::pair<bool, Block>
DeleteQueryProcessor::processDeleteCertQuery(const DeleteParam& param, const Name& signerName)
{
  const Name& certName = param.getTargetName();
  if (!isDeleteAllowed(TYPE_CERT, certName, signerName)) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  m_db.deleteCertificate(certName);

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

bool
DeleteQueryProcessor::isDeleteAllowed(const pib::Type targetType,
                                      const Name& targetName,
                                      const Name& signer) const
{
  // sanity checking, user cannot be deleted.
  if (targetType == TYPE_USER)
    return false;

  // Any identity with prefix /localhost/pib is reserved. Any operation with a targetName under
  // that prefix will be rejected.
  if (PIB_PREFIX.isPrefixOf(targetName))
    return false;

  // Rules for other items:
  //
  //    signer                   | deleting privilege
  //    =========================+===============================================
  //    local mgmt key           | any id, key, and cert
  //    -------------------------+-----------------------------------------------
  //    default key of an id     | any id, key, and cert under the id's namespace
  //    -------------------------+-----------------------------------------------
  //    non-default key of an id | any cert of the key

  const Name& signerKeyName = IdentityCertificate::certificateNameToPublicKeyName(signer);
  const Name& signerId = signerKeyName.getPrefix(-1);

  bool hasSignerDefaultKey = true;
  Name signerDefaultKeyName;
  try {
    signerDefaultKeyName = m_db.getDefaultKeyNameOfIdentity(signerId);
  }
  catch (PibDb::Error&) {
    hasSignerDefaultKey = false;
  }

  Name mgmtCertName;
  try {
    mgmtCertName = m_db.getMgmtCertificate()->getName().getPrefix(-1);
  }
  catch (PibDb::Error&) {
    return false;
  }

  if (signer == mgmtCertName) {
    // signer is current management key, anything is allowed.
    return true;
  }
  else if (hasSignerDefaultKey && signerDefaultKeyName == signerKeyName) {
    // signer is an identity's default key
    if (!signerId.isPrefixOf(targetName))
      return false;
    else
      return true;
  }
  else {
    // non-default key
    if (targetType == TYPE_CERT) {
      // check if it is for the key's cert
      if (IdentityCertificate::certificateNameToPublicKeyName(targetName) == signerKeyName)
        return true;
    }
    else if (targetType == TYPE_KEY) {
      // check if it is for the key itself
      if (targetName == signerKeyName)
        return true;
    }
    return false;
  }
}

} // namespace pib
} // namespace ndn
