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

#include "identity-management-time-fixture.hpp"

namespace ndn {
namespace tests {

IdentityManagementTimeFixture::IdentityManagementTimeFixture()
  : m_keyChainTmpPath(boost::filesystem::path(TMP_TESTS_PATH) / "PibIdMgmtTimeTest")
  , m_keyChain(std::string("pib-sqlite3:").append(m_keyChainTmpPath.string()),
               std::string("tpm-file:").append(m_keyChainTmpPath.string()))
{
}

IdentityManagementTimeFixture::~IdentityManagementTimeFixture()
{
  for (const auto& identity : m_identities) {
    m_keyChain.deleteIdentity(identity);
  }

  boost::filesystem::remove_all(m_keyChainTmpPath);
}

bool
IdentityManagementTimeFixture::addIdentity(const Name& identity, const KeyParams& params)
{
  try {
    m_keyChain.createIdentity(identity, params);
    m_identities.push_back(identity);
    return true;
  }
  catch (std::runtime_error&) {
    return false;
  }
}


} // namespace tests
} // namespace ndn
