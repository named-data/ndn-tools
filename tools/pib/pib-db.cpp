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
 *
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "pib-db.hpp"
#include <sqlite3.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace ndn {
namespace pib {

using std::string;
using std::vector;
using std::set;

const Name PibDb::NON_EXISTING_IDENTITY("/localhost/reserved/non-existing-identity");
const Name PibDb::NON_EXISTING_KEY("/localhost/reserved/non-existing-key");
const Name PibDb::NON_EXISTING_CERTIFICATE("/localhost/reserved/non-existing-certificate");

const Name PibDb::LOCALHOST_PIB("/localhost/pib");
const name::Component PibDb::MGMT_LABEL("mgmt");


static const string INITIALIZATION =
  "CREATE TABLE IF NOT EXISTS                    \n"
  "  mgmt(                                       \n"
  "    id                    INTEGER PRIMARY KEY,\n"
  "    owner                 BLOB NOT NULL,      \n"
  "    tpm_locator           BLOB,               \n"
  "    local_management_cert BLOB NOT NULL       \n"
  "  );                                          \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  mgmt_insert_trigger                         \n"
  "  BEFORE INSERT ON mgmt                       \n"
  "  FOR EACH ROW                                \n"
  "  BEGIN                                       \n"
  "    DELETE FROM mgmt;                         \n"
  "  END;                                        \n"
  "                                              \n"
  "CREATE TABLE IF NOT EXISTS                    \n"
  "  identities(                                 \n"
  "    id                    INTEGER PRIMARY KEY,\n"
  "    identity              BLOB NOT NULL,      \n"
  "    is_default            INTEGER DEFAULT 0   \n"
  "  );                                          \n"
  "CREATE UNIQUE INDEX IF NOT EXISTS             \n"
  "  identityIndex ON identities(identity);      \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  identity_default_before_insert_trigger      \n"
  "  BEFORE INSERT ON identities                 \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1                       \n"
  "  BEGIN                                       \n"
  "    UPDATE identities SET is_default=0;       \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  identity_default_after_insert_trigger       \n"
  "  AFTER INSERT ON identities                  \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NOT EXISTS                             \n"
  "    (SELECT id                                \n"
  "       FROM identities                        \n"
  "       WHERE is_default=1)                    \n"
  "  BEGIN                                       \n"
  "    UPDATE identities                         \n"
  "      SET is_default=1                        \n"
  "      WHERE identity=NEW.identity;            \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  identity_default_update_trigger             \n"
  "  BEFORE UPDATE ON identities                 \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1 AND OLD.is_default=0  \n"
  "  BEGIN                                       \n"
  "    UPDATE identities SET is_default=0;       \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  identity_delete_trigger                     \n"
  "  AFTER DELETE ON identities                  \n"
  "  FOR EACH ROW                                \n"
  "  BEGIN                                       \n"
  "    SELECT identityDeleted (OLD.identity);    \n"
  "  END;                                        \n"
  "                                              \n"
  "CREATE TABLE IF NOT EXISTS                    \n"
  "  keys(                                       \n"
  "    id                    INTEGER PRIMARY KEY,\n"
  "    identity_id           INTEGER NOT NULL,   \n"
  "    key_name              BLOB NOT NULL,      \n"
  "    key_type              INTEGER NOT NULL,   \n"
  "    key_bits              BLOB NOT NULL,      \n"
  "    is_default            INTEGER DEFAULT 0,  \n"
  "    FOREIGN KEY(identity_id)                  \n"
  "      REFERENCES identities(id)               \n"
  "      ON DELETE CASCADE                       \n"
  "      ON UPDATE CASCADE                       \n"
  "  );                                          \n"
  "CREATE UNIQUE INDEX IF NOT EXISTS             \n"
  "  keyIndex ON keys(key_name);                 \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  key_default_before_insert_trigger           \n"
  "  BEFORE INSERT ON keys                       \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1                       \n"
  "  BEGIN                                       \n"
  "    UPDATE keys                               \n"
  "      SET is_default=0                        \n"
  "      WHERE identity_id=NEW.identity_id;      \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  key_default_after_insert_trigger            \n"
  "  AFTER INSERT ON keys                        \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NOT EXISTS                             \n"
  "    (SELECT id                                \n"
  "       FROM keys                              \n"
  "       WHERE is_default=1                     \n"
  "         AND identity_id=NEW.identity_id)     \n"
  "  BEGIN                                       \n"
  "    UPDATE keys                               \n"
  "      SET is_default=1                        \n"
  "      WHERE key_name=NEW.key_name;            \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  key_default_update_trigger                  \n"
  "  BEFORE UPDATE ON keys                       \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1 AND OLD.is_default=0  \n"
  "  BEGIN                                       \n"
  "    UPDATE keys                               \n"
  "      SET is_default=0                        \n"
  "      WHERE identity_id=NEW.identity_id;      \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  key_delete_trigger                          \n"
  "  AFTER DELETE ON keys                        \n"
  "  FOR EACH ROW                                \n"
  "  BEGIN                                       \n"
  "    SELECT keyDeleted (OLD.key_name);         \n"
  "  END;                                        \n"
  "                                              \n"
  "CREATE TABLE IF NOT EXISTS                    \n"
  "  certificates(                               \n"
  "    id                    INTEGER PRIMARY KEY,\n"
  "    key_id                INTEGER NOT NULL,   \n"
  "    certificate_name      BLOB NOT NULL,      \n"
  "    certificate_data      BLOB NOT NULL,      \n"
  "    is_default            INTEGER DEFAULT 0,  \n"
  "    FOREIGN KEY(key_id)                       \n"
  "      REFERENCES keys(id)                     \n"
  "      ON DELETE CASCADE                       \n"
  "      ON UPDATE CASCADE                       \n"
  "  );                                          \n"
  "CREATE UNIQUE INDEX IF NOT EXISTS             \n"
  "  certIndex ON certificates(certificate_name);\n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  cert_default_before_insert_trigger          \n"
  "  BEFORE INSERT ON certificates               \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1                       \n"
  "  BEGIN                                       \n"
  "    UPDATE certificates                       \n"
  "      SET is_default=0                        \n"
  "      WHERE key_id=NEW.key_id;                \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  cert_default_after_insert_trigger           \n"
  "  AFTER INSERT ON certificates                \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NOT EXISTS                             \n"
  "    (SELECT id                                \n"
  "       FROM certificates                      \n"
  "       WHERE is_default=1                     \n"
  "         AND key_id=NEW.key_id)               \n"
  "  BEGIN                                       \n"
  "    UPDATE certificates                       \n"
  "      SET is_default=1                        \n"
  "      WHERE certificate_name=NEW.certificate_name;\n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  cert_default_update_trigger                 \n"
  "  BEFORE UPDATE ON certificates               \n"
  "  FOR EACH ROW                                \n"
  "  WHEN NEW.is_default=1 AND OLD.is_default=0  \n"
  "  BEGIN                                       \n"
  "    UPDATE certificates                       \n"
  "      SET is_default=0                        \n"
  "      WHERE key_id=NEW.key_id;                \n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  cert_delete_trigger                         \n"
  "  AFTER DELETE ON certificates                \n"
  "  FOR EACH ROW                                \n"
  "  BEGIN                                       \n"
  "    SELECT certDeleted (OLD.certificate_name);\n"
  "  END;                                        \n"
  "CREATE TRIGGER IF NOT EXISTS                  \n"
  "  cert_insert_trigger                         \n"
  "  AFTER INSERT ON certificates                \n"
  "  FOR EACH ROW                                \n"
  "  BEGIN                                       \n"
  "    SELECT certInserted (NEW.certificate_name);\n"
  "  END;                                        \n";


/**
 * A utility function to call the normal sqlite3_bind_text where the value and length are
 * value.c_str() and value.size().
 */
static int
sqlite3_bind_string(sqlite3_stmt* statement,
                    int index,
                    const string& value,
                    void(*destructor)(void*))
{
  return sqlite3_bind_text(statement, index, value.c_str(), value.size(), destructor);
}

/**
 * A utility function to call the normal sqlite3_bind_blob where the value and length are
 * block.wire() and block.size().
 */
static int
sqlite3_bind_block(sqlite3_stmt* statement,
                   int index,
                   const Block& block,
                   void(*destructor)(void*))
{
  return sqlite3_bind_blob(statement, index, block.wire(), block.size(), destructor);
}

/**
 * A utility function to generate string by calling the normal sqlite3_column_text.
 */
static string
sqlite3_column_string(sqlite3_stmt* statement, int column)
{
  return string(reinterpret_cast<const char*>(sqlite3_column_text(statement, column)),
                sqlite3_column_bytes(statement, column));
}

/**
 * A utility function to generate block by calling the normal sqlite3_column_text.
 */
static Block
sqlite3_column_block(sqlite3_stmt* statement, int column)
{
  return Block(sqlite3_column_blob(statement, column), sqlite3_column_bytes(statement, column));
}

PibDb::PibDb(const string& dbDir)
{
  // Determine the path of PIB DB
  boost::filesystem::path dir;
  if (dbDir == "") {
    dir = boost::filesystem::path(getenv("HOME")) / ".ndn";
    boost::filesystem::create_directories(dir);
  }
  else {
    dir = boost::filesystem::path(dbDir);
    boost::filesystem::create_directories(dir);
  }
  // Open PIB
  int result = sqlite3_open_v2((dir / "pib.db").c_str(), &m_database,
                               SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
#ifdef NDN_CXX_DISABLE_SQLITE3_FS_LOCKING
                               "unix-dotfile"
#else
                               nullptr
#endif
                               );

  if (result != SQLITE_OK)
    throw Error("PIB DB cannot be opened/created: " + dbDir);


  // enable foreign key
  sqlite3_exec(m_database, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);

  // initialize PIB specific tables
  char* errorMessage = nullptr;
  result = sqlite3_exec(m_database, INITIALIZATION.c_str(), nullptr, nullptr, &errorMessage);
  if (result != SQLITE_OK && errorMessage != nullptr) {
    sqlite3_free(errorMessage);
    throw Error("PIB DB cannot be initialized");
  }

  // create delete trigger functions
  createDbDeleteTrigger();

  getOwnerName();
}

void
PibDb::createDbDeleteTrigger()
{
  int res = 0;

  res = sqlite3_create_function(m_database, "identityDeleted", -1, SQLITE_UTF8,
                                reinterpret_cast<void*>(this),
                                PibDb::identityDeletedFun, nullptr, nullptr);
  if (res != SQLITE_OK)
    throw Error("Cannot create function ``identityDeleted''");

  res = sqlite3_create_function(m_database, "keyDeleted", -1, SQLITE_UTF8,
                                reinterpret_cast<void*>(this),
                                PibDb::keyDeletedFun, nullptr, nullptr);
  if (res != SQLITE_OK)
    throw Error("Cannot create function ``keyDeleted''");

  res = sqlite3_create_function(m_database, "certDeleted", -1, SQLITE_UTF8,
                                reinterpret_cast<void*>(this),
                                PibDb::certDeletedFun, nullptr, nullptr);
  if (res != SQLITE_OK)
    throw Error("Cannot create function ``certDeleted''");

  res = sqlite3_create_function(m_database, "certInserted", -1, SQLITE_UTF8,
                                reinterpret_cast<void*>(this),
                                PibDb::certInsertedFun, nullptr, nullptr);
  if (res != SQLITE_OK)
    throw Error("Cannot create function ``certInserted''");
}

void
PibDb::identityDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  BOOST_ASSERT(argc == 1);

  PibDb* pibDb = reinterpret_cast<PibDb*>(sqlite3_user_data(context));
  Name identity(Block(sqlite3_value_blob(argv[0]), sqlite3_value_bytes(argv[0])));

  pibDb->identityDeleted(identity);
}

void
PibDb::keyDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  BOOST_ASSERT(argc == 1);

  PibDb* pibDb = reinterpret_cast<PibDb*>(sqlite3_user_data(context));
  Name keyName(Block(sqlite3_value_blob(argv[0]), sqlite3_value_bytes(argv[0])));

  pibDb->keyDeleted(keyName);
}

void
PibDb::certDeletedFun(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  BOOST_ASSERT(argc == 1);

  PibDb* pibDb = reinterpret_cast<PibDb*>(sqlite3_user_data(context));
  Name certName(Block(sqlite3_value_blob(argv[0]), sqlite3_value_bytes(argv[0])));

  pibDb->certificateDeleted(certName);
}

void
PibDb::certInsertedFun(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  BOOST_ASSERT(argc == 1);

  PibDb* pibDb = reinterpret_cast<PibDb*>(sqlite3_user_data(context));
  Name certName(Block(sqlite3_value_blob(argv[0]), sqlite3_value_bytes(argv[0])));

  pibDb->certificateInserted(certName);
}

void
PibDb::updateMgmtCertificate(const IdentityCertificate& certificate)
{
  const Name& keyName = certificate.getPublicKeyName();

  // Name of mgmt key should be "/localhost/pib/[UserName]/mgmt/[KeyID]"
  if (keyName.size() != 5 ||
      keyName.compare(0, 2, LOCALHOST_PIB) ||
      keyName.get(3) != MGMT_LABEL)
    throw Error("PibDb::updateMgmtCertificate: certificate does not follow the naming convention");

  string owner = keyName.get(2).toUri();
  sqlite3_stmt* statement;
  if (!m_owner.empty()) {
    if (m_owner != owner)
      throw Error("PibDb::updateMgmtCertificate: owner name does not match");
    else {
      sqlite3_prepare_v2(m_database,
                         "UPDATE mgmt SET local_management_cert=? WHERE owner=?",
                         -1, &statement, nullptr);
    }
  }
  else {
    sqlite3_prepare_v2(m_database,
                       "INSERT INTO mgmt (local_management_cert, owner) VALUES (?, ?)",
                       -1, &statement, nullptr);
  }

  sqlite3_bind_block(statement, 1, certificate.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_bind_string(statement, 2, owner, SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);

  m_owner = owner;

  mgmtCertificateChanged();
}

string
PibDb::getOwnerName() const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database, "SELECT owner FROM mgmt", -1, &statement, nullptr);

  if (sqlite3_step(statement) == SQLITE_ROW) {
    m_owner = sqlite3_column_string(statement, 0);
  }

  sqlite3_finalize(statement);
  return m_owner;
}

shared_ptr<IdentityCertificate>
PibDb::getMgmtCertificate() const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database, "SELECT local_management_cert FROM mgmt", -1, &statement, nullptr);

  shared_ptr<IdentityCertificate> certificate;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    certificate = make_shared<IdentityCertificate>();
    certificate->wireDecode(sqlite3_column_block(statement, 0));
  }

  sqlite3_finalize(statement);
  return certificate;
}

void
PibDb::setTpmLocator(const std::string& tpmLocator)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "UPDATE mgmt SET tpm_locator=? WHERE owner=?",
                     -1, &statement, nullptr);
  sqlite3_bind_string(statement, 1, tpmLocator, SQLITE_TRANSIENT);
  sqlite3_bind_string(statement, 2, m_owner, SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

std::string
PibDb::getTpmLocator() const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database, "SELECT tpm_locator FROM mgmt", -1, &statement, nullptr);

  string tpmLocator;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    tpmLocator = sqlite3_column_string(statement, 0);
  }

  sqlite3_finalize(statement);
  return tpmLocator;
}

int64_t
PibDb::addIdentity(const Name& identity)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "INSERT INTO identities (identity) values (?)",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);

  return sqlite3_last_insert_rowid(m_database);
}

void
PibDb::deleteIdentity(const Name& identity)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "DELETE FROM identities WHERE identity=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

bool
PibDb::hasIdentity(const Name& identity) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT id FROM identities WHERE identity=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);
  int result = sqlite3_step(statement);
  sqlite3_finalize(statement);

  if (result == SQLITE_ROW)
    return true;
  else
    return false;
}

void
PibDb::setDefaultIdentity(const Name& identity)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "UPDATE identities SET is_default=1 WHERE identity=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

Name
PibDb::getDefaultIdentity() const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT identity FROM identities WHERE is_default=1",
                     -1, &statement, nullptr);

  Name identity = NON_EXISTING_IDENTITY;
  if (sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_bytes(statement, 0) != 0) {
    identity = Name(sqlite3_column_block(statement, 0));
  }
  sqlite3_finalize(statement);
  return identity;
}

vector<Name>
PibDb::listIdentities() const
{
  vector<Name> identities;

  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database, "SELECT identity FROM identities", -1, &statement, nullptr);

  identities.clear();
  while (sqlite3_step(statement) == SQLITE_ROW) {
    Name name(sqlite3_column_block(statement, 0));
    identities.push_back(name);
  }
  sqlite3_finalize(statement);

  return identities;
}

int64_t
PibDb::addKey(const Name& keyName, const PublicKey& key)
{
  if (keyName.empty())
    return 0;

  Name&& identity = keyName.getPrefix(-1);
  if (!hasIdentity(identity))
    addIdentity(identity);

  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "INSERT INTO keys (identity_id, key_name, key_type, key_bits) \
                      values ((SELECT id FROM identities WHERE identity=?), ?, ?, ?)",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_bind_block(statement, 2, keyName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_bind_int(statement, 3, static_cast<int>(key.getKeyType()));
  sqlite3_bind_blob(statement, 4, key.get().buf(), key.get().size(), SQLITE_STATIC);
  sqlite3_step(statement);
  sqlite3_finalize(statement);

  return sqlite3_last_insert_rowid(m_database);
}

shared_ptr<PublicKey>
PibDb::getKey(const Name& keyName) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT key_bits FROM keys WHERE key_name=?"
                     , -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);

  shared_ptr<PublicKey> key;
  if (sqlite3_step(statement) == SQLITE_ROW) {
      key = make_shared<PublicKey>(static_cast<const uint8_t*>(sqlite3_column_blob(statement, 0)),
                                   sqlite3_column_bytes(statement, 0));
  }
  sqlite3_finalize(statement);
  return key;
}

void
PibDb::deleteKey(const Name& keyName)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "DELETE FROM keys WHERE key_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

bool
PibDb::hasKey(const Name& keyName) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT id FROM keys WHERE key_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);

  int result = sqlite3_step(statement);
  sqlite3_finalize(statement);

  if (result == SQLITE_ROW)
    return true;
  else
    return false;
}

void
PibDb::setDefaultKeyNameOfIdentity(const Name& keyName)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "UPDATE keys SET is_default=1 WHERE key_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

Name
PibDb::getDefaultKeyNameOfIdentity(const Name& identity) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT key_name FROM keys JOIN identities ON keys.identity_id=identities.id\
                      WHERE identities.identity=? AND keys.is_default=1",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);

  Name keyName = NON_EXISTING_KEY;
  if (sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_bytes(statement, 0) != 0) {
    keyName = Name(sqlite3_column_block(statement, 0));
  }

  sqlite3_finalize(statement);
  return keyName;
}

vector<Name>
PibDb::listKeyNamesOfIdentity(const Name& identity) const
{
  vector<Name> keyNames;

  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT key_name FROM keys JOIN identities ON keys.identity_id=identities.id\
                      WHERE identities.identity=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, identity.wireEncode(), SQLITE_TRANSIENT);

  keyNames.clear();
  while (sqlite3_step(statement) == SQLITE_ROW) {
    Name keyName(sqlite3_column_block(statement, 0));
    keyNames.push_back(keyName);
  }

  sqlite3_finalize(statement);
  return keyNames;
}


int64_t
PibDb::addCertificate(const IdentityCertificate& certificate)
{
  const Name& certName = certificate.getName();
  const Name& keyName = certificate.getPublicKeyName();

  if (!hasKey(keyName))
    addKey(keyName, certificate.getPublicKeyInfo());

  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "INSERT INTO certificates \
                      (key_id, certificate_name, certificate_data) \
                      values ((SELECT id FROM keys WHERE key_name=?), ?, ?)",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_bind_block(statement, 2, certName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_bind_block(statement, 3, certificate.wireEncode(), SQLITE_STATIC);
  sqlite3_step(statement);
  sqlite3_finalize(statement);

  return sqlite3_last_insert_rowid(m_database);
}

shared_ptr<IdentityCertificate>
PibDb::getCertificate(const Name& certificateName) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT certificate_data FROM certificates WHERE certificate_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, certificateName.wireEncode(), SQLITE_TRANSIENT);

  shared_ptr<IdentityCertificate> certificate;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    certificate = make_shared<IdentityCertificate>();
    certificate->wireDecode(sqlite3_column_block(statement, 0));
  }

  sqlite3_finalize(statement);
  return certificate;
}

void
PibDb::deleteCertificate(const Name& certificateName)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "DELETE FROM certificates WHERE certificate_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, certificateName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

bool
PibDb::hasCertificate(const Name& certificateName) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT id FROM certificates WHERE certificate_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, certificateName.wireEncode(), SQLITE_TRANSIENT);
  int result = sqlite3_step(statement);
  sqlite3_finalize(statement);

  if (result == SQLITE_ROW)
    return true;
  else
    return false;
}

void
PibDb::setDefaultCertNameOfKey(const Name& certificateName)
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "UPDATE certificates SET is_default=1 WHERE certificate_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, certificateName.wireEncode(), SQLITE_TRANSIENT);
  sqlite3_step(statement);
  sqlite3_finalize(statement);
}

Name
PibDb::getDefaultCertNameOfKey(const Name& keyName) const
{
  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT certificate_name\
                      FROM certificates JOIN keys ON certificates.key_id=keys.id\
                      WHERE keys.key_name=? AND certificates.is_default=1",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);

  Name certName = NON_EXISTING_CERTIFICATE;
  if (sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_bytes(statement, 0) != 0) {
    certName = Name(sqlite3_column_block(statement, 0));
  }
  sqlite3_finalize(statement);
  return certName;
}

vector<Name>
PibDb::listCertNamesOfKey(const Name& keyName) const
{
  vector<Name> certNames;

  sqlite3_stmt* statement;
  sqlite3_prepare_v2(m_database,
                     "SELECT certificate_name\
                      FROM certificates JOIN keys ON certificates.key_id=keys.id\
                      WHERE keys.key_name=?",
                     -1, &statement, nullptr);
  sqlite3_bind_block(statement, 1, keyName.wireEncode(), SQLITE_TRANSIENT);

  certNames.clear();
  while (sqlite3_step(statement) == SQLITE_ROW) {
    Name name(sqlite3_column_block(statement, 0));
    certNames.push_back(name);
  }
  sqlite3_finalize(statement);

  return certNames;
}

} // namespace pib
} // namespace ndn
