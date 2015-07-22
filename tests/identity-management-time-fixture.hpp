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

#ifndef NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_TIME_FIXTURE_HPP
#define NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_TIME_FIXTURE_HPP

#include <ndn-cxx/security/key-chain.hpp>
#include <vector>
#include <boost/filesystem.hpp>

#include "tests/test-common.hpp"

namespace ndn {
namespace tests {

/**
 * @brief IdentityManagementTimeFixture is a test suite level fixture.
 * Test cases in the suite can use this fixture to create identities.
 * Identities added via addIdentity method are automatically deleted
 * during test teardown.
 */
class IdentityManagementTimeFixture : public tests::UnitTestTimeFixture
{
public:
  IdentityManagementTimeFixture();

  ~IdentityManagementTimeFixture();

  /// @brief add identity, return true if succeed.
  bool
  addIdentity(const Name& identity, const KeyParams& params = KeyChain::DEFAULT_KEY_PARAMS);

protected:
  boost::filesystem::path m_keyChainTmpPath;

  KeyChain m_keyChain;
  std::vector<Name> m_identities;
};

} // namespace tests
} // namespace ndn

#endif // NDN_TOOLS_TESTS_IDENTITY_MANAGEMENT_TIME_FIXTURE_HPP
