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

#ifndef NDN_TOOLS_PIB_UPDATE_QUERY_PROCESSOR_HPP
#define NDN_TOOLS_PIB_UPDATE_QUERY_PROCESSOR_HPP

#include "pib-db.hpp"
#include "encoding/update-param.hpp"
#include <ndn-cxx/interest.hpp>
#include <utility>

namespace ndn {
namespace pib {

class Pib;

/// @brief Processing unit for PIB update query
class UpdateQueryProcessor : noncopyable
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
   * @param db The pib database.
   */
  UpdateQueryProcessor(PibDb& db, Pib& pib);

  std::pair<bool, Block>
  operator()(const Interest& interest);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   * @brief Determine if an update command is allowed.
   *
   * @param targetType The type of the target that will be updated.
   * @param targetName The name of the target that will be updated.
   * @param signer     The name of the command signer.
   * @param defaultOpt The default settings that is requested to update.
   */
  bool
  isUpdateAllowed(const pib::Type targetType,
                  const Name& targetName,
                  const Name& signer,
                  const pib::DefaultOpt defaultOpt) const;

private:
  std::pair<bool, Block>
  processUpdateUserQuery(const UpdateParam& param, const Name& signerName);

  std::pair<bool, Block>
  processUpdateIdQuery(const UpdateParam& param, const Name& signerName);

  std::pair<bool, Block>
  processUpdateKeyQuery(const UpdateParam& param, const Name& signerName);

  std::pair<bool, Block>
  processUpdateCertQuery(const UpdateParam& param, const Name& signerName);

private:
  static const size_t UPDATE_QUERY_LENGTH;
  static const Name PIB_PREFIX;

  PibDb&  m_db;
  Pib& m_pib;
};

} // namespace pib
} // namespace ndn

#endif // NDN_TOOLS_PIB_UPDATE_QUERY_PROCESSOR_HPP
