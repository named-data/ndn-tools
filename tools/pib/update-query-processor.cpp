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


#include "update-query-processor.hpp"
#include "encoding/pib-encoding.hpp"
#include "pib.hpp"

#include <boost/lexical_cast.hpp>

namespace ndn {
namespace pib {

using std::string;

const size_t UpdateQueryProcessor::UPDATE_QUERY_LENGTH = 9;
const Name UpdateQueryProcessor::PIB_PREFIX("/localhost/pib");

UpdateQueryProcessor::UpdateQueryProcessor(PibDb& db, Pib& pib)
  : m_db(db)
  , m_pib(pib)
{
}

std::pair<bool, Block>
UpdateQueryProcessor::operator()(const Interest& interest)
{
  const Name& interestName = interest.getName();

  // handle pib query: /localhost/pib/[UserName]/update/param/<signed_interest_related_components>
  if (interestName.size() != UPDATE_QUERY_LENGTH) {
    // malformed interest, discard
    return std::make_pair(false, Block());
  }

  try {
    UpdateParam param;
    param.wireDecode(interestName.get(OFFSET_PARAM).blockFromValue());

    SignatureInfo sigInfo;
    sigInfo.wireDecode(interestName.get(OFFSET_SIG_INFO).blockFromValue());

    // sigInfo must have KeyLocator.Name if the interest passed validation.
    Name signerName;
    signerName = sigInfo.getKeyLocator().getName();

    switch (param.getEntityType()) {
    case tlv::pib::User:
      return processUpdateUserQuery(param, signerName);
    case tlv::pib::Identity:
      return processUpdateIdQuery(param, signerName);
    case tlv::pib::PublicKey:
      return processUpdateKeyQuery(param, signerName);
    case tlv::pib::Certificate:
      return processUpdateCertQuery(param, signerName);
    default:
      {
        PibError error(ERR_WRONG_PARAM,
                       "entity type is not supported: " +
                       boost::lexical_cast<string>(param.getEntityType()));
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
UpdateQueryProcessor::processUpdateUserQuery(const UpdateParam& param, const Name& signerName)
{
  Name expectedId = m_db.getMgmtCertificate()->getPublicKeyName().getPrefix(-1);
  Name targetId = param.getUser().getMgmtCert().getPublicKeyName().getPrefix(-1);
  Name signerId = IdentityCertificate::certificateNameToPublicKeyName(signerName).getPrefix(-1);

  if (expectedId == targetId && expectedId == signerId) {
    m_db.updateMgmtCertificate(param.getUser().getMgmtCert());
    m_pib.setMgmtCert(make_shared<IdentityCertificate>(param.getUser().getMgmtCert()));
    return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
  }

  PibError error(ERR_WRONG_PARAM, "not allowed to update user");
  return std::make_pair(true, error.wireEncode());
}

std::pair<bool, Block>
UpdateQueryProcessor::processUpdateIdQuery(const UpdateParam& param, const Name& signerName)
{
  const Name& identity = param.getIdentity().getIdentity();
  if (!isUpdateAllowed(TYPE_ID, identity, signerName, param.getDefaultOpt())) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  // add the identity
  m_db.addIdentity(identity);

  // set the identity as user default if it is requested.
  const pib::DefaultOpt& defaultOpt = param.getDefaultOpt();
  if (DEFAULT_OPT_USER_MASK & defaultOpt) {
    m_db.setDefaultIdentity(identity);
  }

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

std::pair<bool, Block>
UpdateQueryProcessor::processUpdateKeyQuery(const UpdateParam& param, const Name& signerName)
{
  const Name& keyName = param.getPublicKey().getKeyName();
  if (!isUpdateAllowed(TYPE_KEY, keyName, signerName, param.getDefaultOpt())) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  Name identity = keyName.getPrefix(-1);

  // add the key
  m_db.addKey(keyName, param.getPublicKey().getPublicKey());

  const pib::DefaultOpt& defaultOpt = param.getDefaultOpt();
  if (DEFAULT_OPT_ID_MASK & defaultOpt) // set the key as identity default if requested.
    m_db.setDefaultKeyNameOfIdentity(keyName);
  if (DEFAULT_OPT_USER_MASK & defaultOpt) // set the identity as user default if requested.
    m_db.setDefaultIdentity(identity);

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

std::pair<bool, Block>
UpdateQueryProcessor::processUpdateCertQuery(const UpdateParam& param, const Name& signerName)
{
  const IdentityCertificate& cert = param.getCertificate().getCertificate();
  const Name& certName = cert.getName();
  if (!isUpdateAllowed(TYPE_CERT, certName, signerName, param.getDefaultOpt())) {
    PibError error(ERR_WRONG_SIGNER, "signer is not trusted for this command");
    return std::make_pair(true, error.wireEncode());
  }

  const Name& keyName = cert.getPublicKeyName();
  Name identity = keyName.getPrefix(-1);

  // add certificate
  m_db.addCertificate(cert);

  const pib::DefaultOpt& defaultOpt = param.getDefaultOpt();
  if (DEFAULT_OPT_KEY_MASK & defaultOpt) // set the cert as key default if requested.
    m_db.setDefaultCertNameOfKey(certName);
  if (DEFAULT_OPT_ID_MASK & defaultOpt) // set the key as identity default if requested.
    m_db.setDefaultKeyNameOfIdentity(keyName);
  if (DEFAULT_OPT_USER_MASK & defaultOpt) // set the identity as user default if requested.
    m_db.setDefaultIdentity(identity);

  return std::make_pair(true, PibError(ERR_SUCCESS).wireEncode());
}

bool
UpdateQueryProcessor::isUpdateAllowed(const pib::Type targetType,
                                      const Name& targetName,
                                      const Name& signer,
                                      const pib::DefaultOpt defaultOpt) const
{
  // Any identity with prefix /localhost/pib is reserved. Any operation with a targetName under
  // that prefix will be rejected.
  if (PIB_PREFIX.isPrefixOf(targetName))
    return false;

  // A request is wrong if
  // 1) it wants to change the default setting of an identity, but the target is not key nor cert
  // 2) it wants to change the default setting of a key, but the target is not cert
  if ((defaultOpt == DEFAULT_OPT_ID && (targetType != TYPE_KEY && targetType != TYPE_CERT)) ||
      (defaultOpt == DEFAULT_OPT_KEY && targetType != TYPE_CERT))
    return false;


  // Rules for other items:
  //
  //    signer          | adding privilege         | default setting privilege
  //    ================+==========================+============================================
  //    local mgmt key  | any id, key, and cert    | the default id,
  //                    |                          | the default key of any id,
  //                    |                          | the default cert of any key of any id
  //    ----------------+--------------------------+--------------------------------------------
  //    default key of  | any id, key, and cert    | the default key of the id and its sub ids,
  //    an id           | under the id's namespace | the default cert of any key of the id
  //                    |                          | and its sub ids
  //    ----------------+--------------------------+--------------------------------------------
  //    non-default key | any cert of the key      | the default cert of the key
  //    of an id

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

    // check default setting
    // user default setting is not allowed
    if (defaultOpt == DEFAULT_OPT_USER)
      return false;
    else
      return true;
  }
  else {
    // non-default key
    if (targetType != TYPE_CERT)
      return false;

    // check if it is for the key's cert
    if (IdentityCertificate::certificateNameToPublicKeyName(targetName) != signerKeyName)
      return false;

    if (defaultOpt == DEFAULT_OPT_USER || defaultOpt == DEFAULT_OPT_ID)
      return false;
    else
      return true;
  }
}

} // namespace pib
} // namespace ndn
