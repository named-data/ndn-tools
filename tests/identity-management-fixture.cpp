/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California.
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

#include "identity-management-fixture.hpp"
#include <ndn-cxx/util/io.hpp>
#include <boost/filesystem.hpp>

namespace ndn {
namespace tests {

IdentityManagementFixture::IdentityManagementFixture()
  : m_keyChainPath((boost::filesystem::path(TMP_TESTS_PATH) / "IdentityManagementFixture").string())
  , m_keyChain("pib-sqlite3:" + m_keyChainPath, "tpm-file:" + m_keyChainPath)
{
}

IdentityManagementFixture::~IdentityManagementFixture()
{
  for (const auto& id : m_identities) {
    m_keyChain.deleteIdentity(id);
  }

  boost::system::error_code ec; // ignore error

  boost::filesystem::remove_all(m_keyChainPath, ec);

  for (const auto& certFile : m_certFiles) {
    boost::filesystem::remove(certFile, ec);
  }
}

bool
IdentityManagementFixture::addIdentity(const Name& identity, const KeyParams& params)
{
  try {
    m_keyChain.createIdentity(identity, params);
    m_identities.push_back(identity);
    return true;
  }
  catch (const std::runtime_error&) {
    return false;
  }
}

bool
IdentityManagementFixture::saveIdentityCertificate(const Name& identity, const std::string& filename, bool wantAdd)
{
  shared_ptr<IdentityCertificate> cert;
  try {
    cert = m_keyChain.getCertificate(m_keyChain.getDefaultCertificateNameForIdentity(identity));
  }
  catch (const SecPublicInfo::Error&) {
    if (wantAdd && this->addIdentity(identity)) {
      return this->saveIdentityCertificate(identity, filename, false);
    }
    return false;
  }

  m_certFiles.push_back(filename);
  try {
    io::save(*cert, filename);
    return true;
  }
  catch (const io::Error&) {
    return false;
  }
}

} // namespace tests
} // namespace ndn
