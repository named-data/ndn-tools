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

#ifndef NDN_TOOLS_PIB_PIB_VALIDATOR_HPP
#define NDN_TOOLS_PIB_PIB_VALIDATOR_HPP

#include "pib-db.hpp"
#include "key-cache.hpp"
#include <ndn-cxx/security/validator.hpp>
#include <unordered_map>

namespace ndn {
namespace pib {


/*
 * @brief The validator to verify the command interests to PIB service
 *
 * @sa http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base
 */
class PibValidator : public Validator
{
  struct UserKeyCache;
public:
  explicit
  PibValidator(const PibDb& pibDb,
               size_t maxCacheSize = 1000);

  ~PibValidator();

protected:
  virtual void
  checkPolicy(const Interest& interest,
              int nSteps,
              const OnInterestValidated& onValidated,
              const OnInterestValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ValidationRequest>>& nextSteps);

  virtual void
  checkPolicy(const Data& data,
              int nSteps,
              const OnDataValidated& onValidated,
              const OnDataValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ValidationRequest>>& nextSteps);

private:

  const PibDb& m_db;

  bool m_isMgmtReady;
  std::string m_owner;
  shared_ptr<IdentityCertificate> m_mgmtCert;

  KeyCache m_keyCache;

  util::signal::ScopedConnection m_mgmtChangeConnection;
  util::signal::ScopedConnection m_keyDeletedConnection;
};

} // namespace pib
} // namespace ndn

#endif // NDN_TOOLS_PIB_PIB_VALIDATOR_HPP
