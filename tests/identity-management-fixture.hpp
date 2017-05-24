/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California.
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
 */

#ifndef NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP
#define NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP

#include "tests/test-common.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <vector>

namespace ndn {
namespace tests {

/** \brief a fixture providing an in-memory KeyChain
 */
class IdentityManagementFixture
{
public:
  IdentityManagementFixture();

  /** \brief deletes saved certificate files
   */
  ~IdentityManagementFixture();

  /** \brief add identity
   *  \return whether successful
   */
  bool
  addIdentity(const Name& identity, const KeyParams& params = KeyChain::getDefaultKeyParams());

  /** \brief save identity certificate to a file
   *  \param identity identity name
   *  \param filename file name, should be writable
   *  \param wantAdd if true, add new identity when necessary
   *  \return whether successful
   */
  bool
  saveIdentityCertificate(const Name& identity, const std::string& filename, bool wantAdd = false);

protected:
  KeyChain m_keyChain;

private:
  std::vector<std::string> m_certFiles;
};

/** \brief convenience base class for inheriting from both UnitTestTimeFixture
 *         and IdentityManagementFixture
 */
class IdentityManagementTimeFixture : public UnitTestTimeFixture
                                    , public IdentityManagementFixture
{
};

} // namespace tests
} // namespace ndn

#endif // NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP
