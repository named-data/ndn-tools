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

#ifndef NDN_TOOLS_PIB_CERT_PUBLISHER_HPP
#define NDN_TOOLS_PIB_CERT_PUBLISHER_HPP

#include "pib-db.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/in-memory-storage-persistent.hpp>

namespace ndn {
namespace pib {

/// @brief implements the certificate publisher
class CertPublisher : noncopyable
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
   * @param pibDb Database which holds the certificates.
   */
  CertPublisher(Face& face, PibDb& pibDb);

  ~CertPublisher();

private:
  void
  startPublishAll();

  /**
   * @brief add an interest filter for the certificate
   */
  void
  registerCertPrefix(const Name& certName);

  void
  processInterest(const InterestFilter& interestFilter,
                  const Interest& interest);

  void
  startPublish(const Name& certName);

  /**
   * @brief callback when a certificate is deleted
   *
   * The method will remove the cert from in-memory storage
   * and also unset interest filter if the removed cert
   * is the only one with the registered prefix.
   *
   * @param certName removed certificate name
   */
  void
  stopPublish(const Name& certName);

private:
  Face& m_face;
  PibDb& m_db;
  util::InMemoryStoragePersistent m_responseCache;
  std::map<Name, const RegisteredPrefixId*> m_registeredPrefixes;

  util::signal::ScopedConnection m_certDeletedConnection;
  util::signal::ScopedConnection m_certInsertedConnection;
};

} // namespace pib
} // namespace ndn

#endif // NDN_TOOLS_PIB_CERT_PUBLISHER_HPP
