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

#ifndef NDN_TOOLS_PIB_PIB_HPP
#define NDN_TOOLS_PIB_PIB_HPP

#include "pib-db.hpp"
#include "pib-validator.hpp"
#include "cert-publisher.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/in-memory-storage-persistent.hpp>

#include "get-query-processor.hpp"
#include "default-query-processor.hpp"
#include "list-query-processor.hpp"
#include "update-query-processor.hpp"
#include "delete-query-processor.hpp"

#include <ndn-cxx/security/sec-tpm.hpp>

namespace ndn {
namespace pib {

/// @brief implements the PIB service
class Pib : noncopyable
{
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

  /**
   * @brief Constructor
   *
   * @param face The face pib used to receive queries and serve certificates.
   * @param dbDir Absolute path to the directory of the pib database.
   * @param tpmLocator URI to locate the TPM for pib service.
   * @param owner Owner of the pib database.
   */
  Pib(Face& face,
      const std::string& dbDir,
      const std::string& tpmLocator,
      const std::string& owner);

  ~Pib();

  void
  setMgmtCert(std::shared_ptr<IdentityCertificate> mgmtCert);

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
  PibDb&
  getDb()
  {
    return m_db;
  }

  SecTpm&
  getTpm()
  {
    return *m_tpm;
  }

  util::InMemoryStoragePersistent&
  getResponseCache()
  {
    return m_responseCache;
  }

  const std::string&
  getOwner() const
  {
    return m_owner;
  }

  const IdentityCertificate&
  getMgmtCert() const
  {
    BOOST_ASSERT(m_mgmtCert != nullptr);
    return *m_mgmtCert;
  }

private: // initialization
  /// @brief initialize the PIB's own TPM.
  void
  initializeTpm(const std::string& tpmLocator);

  /// @brief initialize management certificate
  void
  initializeMgmtCert();

  std::shared_ptr<IdentityCertificate>
  prepareCertificate(const Name& keyName, const KeyParams& keyParams,
                     const time::system_clock::TimePoint& notBefore,
                     const time::system_clock::TimePoint& notAfter,
                     const Name& signerName = EMPTY_SIGNER_NAME);

  /// @brief register prefix for PIB query and management certificate
  void
  registerPrefix();

  template<class Processor>
  const InterestFilterId*
  registerProcessor(const Name& prefix, Processor& process);

  template<class Processor>
  const InterestFilterId*
  registerSignedCommandProcessor(const Name& prefix, Processor& process);

  template<class Processor>
  void
  processCommand(Processor& process, const Interest& interest);

  void
  returnResult(const Name& dataName, const Block& content);

private:

  static const Name EMPTY_SIGNER_NAME;
  static const Name PIB_PREFIX;
  static const name::Component MGMT_LABEL;

  PibDb m_db;
  std::unique_ptr<SecTpm> m_tpm;
  std::string m_owner;
  std::shared_ptr<IdentityCertificate> m_mgmtCert;

  PibValidator m_validator;

  Face& m_face;
  CertPublisher m_certPublisher;
  util::InMemoryStoragePersistent m_responseCache;

  const RegisteredPrefixId* m_pibPrefixId;
  const InterestFilterId* m_pibMgmtFilterId;
  const InterestFilterId* m_pibGetFilterId;
  const InterestFilterId* m_pibDefaultFilterId;
  const InterestFilterId* m_pibListFilterId;
  const InterestFilterId* m_pibUpdateFilterId;
  const InterestFilterId* m_pibDeleteFilterId;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:

  GetQueryProcessor m_getProcessor;
  DefaultQueryProcessor m_defaultProcessor;
  ListQueryProcessor m_listProcessor;
  UpdateQueryProcessor m_updateProcessor;
  DeleteQueryProcessor m_deleteProcessor;
};

} // namespace pib
} // namespace ndn

#endif // NDN_TOOLS_PIB_PIB_HPP
