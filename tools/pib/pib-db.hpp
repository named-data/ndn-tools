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

#ifndef NDN_TOOLS_PIB_PIB_DB_HPP
#define NDN_TOOLS_PIB_PIB_DB_HPP

#include "core/common.hpp"
#include <ndn-cxx/security/identity-certificate.hpp>
#include <ndn-cxx/util/signal.hpp>

#include <set>
#include <vector>

struct sqlite3;
struct sqlite3_context;
struct Mem;
typedef Mem sqlite3_value;

namespace ndn {
namespace pib {

/// @brief Callback to report changes on user info.
typedef function<void(const std::string&)> UserChangedEventHandler;

/// @brief Callback to report that a key is deleted.
typedef function<void(const std::string&, const Name&,
                      const name::Component&)> KeyDeletedEventHandler;

/**
 * @brief PibDb is a class to manage the database of PIB service.
 *
 * only public key related information is stored in this database.
 * Detail information can be found at:
 * http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base
 */
class PibDb : noncopyable
{
public:
  util::signal::Signal<PibDb> mgmtCertificateChanged;
  util::signal::Signal<PibDb, Name> certificateDeleted;
  util::signal::Signal<PibDb, Name> keyDeleted;
  util::signal::Signal<PibDb, Name> identityDeleted;
  util::signal::Signal<PibDb, Name> certificateInserted;

public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  explicit
  PibDb(const std::string& dbDir = "");

public: // Owner management
  /**
   * @brief Update owner's management certificate
   *
   * Since owner name is encoded in the management certificate,
   * this method can also set the owner name if it is not set.
   * If the owner name is set but does not match the one in the
   * supplied certificate, it throws @p Error.
   *
   * @throws Error if supplied certificate is wrong
   */
  void
  updateMgmtCertificate(const IdentityCertificate& certificate);

  /**
   * @brief Get owner name
   *
   * return empty string when owner name is not set.
   */
  std::string
  getOwnerName() const;

  /** @brief Get the management cert
   *
   * return nullptr when the management cert is not set
   */
  shared_ptr<IdentityCertificate>
  getMgmtCertificate() const;

  /// @brief Set TPM locator
  void
  setTpmLocator(const std::string& tpmLocator);

  /**
   * @brief Get TPM locator
   *
   * return empty string when tpmLocator is not set.
   */
  std::string
  getTpmLocator() const;

public: // Identity management

  /**
   * @brief Add an identity
   *
   * @return row id of the added identity, 0 if insert fails.
   */
  int64_t
  addIdentity(const Name& identity);

  /// @brief Delete an identity
  void
  deleteIdentity(const Name& identity);

  /// @brief Check if an identity exists
  bool
  hasIdentity(const Name& identity) const;

  /// @brief Get all identities
  std::vector<Name>
  listIdentities() const;

  /// @brief Set the default identity
  void
  setDefaultIdentity(const Name& identity);

  /**
   * @brief Get the default identity
   *
   * @return default identity or /localhost/reserved/non-existing-identity if no default identity
   */
  Name
  getDefaultIdentity() const;

public: // Key management

  /// @brief Add key
  int64_t
  addKey(const Name& keyName, const PublicKey& key);

  /// @brief Delete key
  void
  deleteKey(const Name& keyName);

  /// @brief Check if a key exists
  bool
  hasKey(const Name& keyName) const;

  /**
   * @brief Get key
   *
   * @return shared pointer to the key, nullptr if the key does not exit
   */
  shared_ptr<PublicKey>
  getKey(const Name& keyName) const;

  /// @brief Get all the key names of an identity
  std::vector<Name>
  listKeyNamesOfIdentity(const Name& identity) const;

  /// @brief Set an identity's default key name
  void
  setDefaultKeyNameOfIdentity(const Name& keyName);

  /**
   * @brief Get the default key name of an identity
   *
   * @return default key name or /localhost/reserved/non-existing-key if no default key
   */
  Name
  getDefaultKeyNameOfIdentity(const Name& identity) const;

public: // Certificate management

  /// @brief Add a certificate
  int64_t
  addCertificate(const IdentityCertificate& certificate);

  /// @brief Delete a certificate
  void
  deleteCertificate(const Name& certificateName);

  /// @brief Check if the certificate exist
  bool
  hasCertificate(const Name& certificateName) const;

  /**
   * @brief Get a certificate
   *
   * @return shared pointer to the certificate, nullptr if the certificate does not exist
   */
  shared_ptr<IdentityCertificate>
  getCertificate(const Name& certificateName) const;

  /// @brief Get all the cert names of a key
  std::vector<Name>
  listCertNamesOfKey(const Name& keyName) const;

  /// @brief Set a key's default certificate name
  void
  setDefaultCertNameOfKey(const Name& certificateName);

  /**
   * @brief Get a key's default certificate name
   *
   * @return default certificate name or /localhost/reserved/non-existing-certificate if no default
   *         certificate.
   */
  Name
  getDefaultCertNameOfKey(const Name& keyName) const;

private:
  void
  createDbDeleteTrigger();

private:
  static void
  identityDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv);

  static void
  keyDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv);

  static void
  certDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv);

  static void
  certInsertedFun(sqlite3_context* context, int argc, sqlite3_value** argv);

public:
  static const Name NON_EXISTING_IDENTITY;
  static const Name NON_EXISTING_KEY;
  static const Name NON_EXISTING_CERTIFICATE;

private:
  static const Name LOCALHOST_PIB;
  static const name::Component MGMT_LABEL;

private:
  sqlite3* m_database;

  mutable std::string m_owner;
};

} // namespace pib
} // namespace ndn


#endif // NDN_TOOLS_PIB_PIB_DB_HPP
