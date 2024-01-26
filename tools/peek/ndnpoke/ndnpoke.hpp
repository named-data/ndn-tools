/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author Davide Pesavento <davidepesa@gmail.com>
 */

#ifndef NDN_TOOLS_NDNPOKE_NDNPOKE_HPP
#define NDN_TOOLS_NDNPOKE_NDNPOKE_HPP

#include "core/common.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <optional>

namespace ndn::peek {

/**
 * \brief options for NdnPoke
 */
struct PokeOptions
{
  // Data construction options
  Name name;
  time::milliseconds freshnessPeriod = DEFAULT_FRESHNESS_PERIOD;
  bool wantFinalBlockId = false;
  security::SigningInfo signingInfo;

  // program behavior options
  bool isVerbose = false;
  bool wantUnsolicited = false;
  std::optional<time::milliseconds> timeout;
};

class NdnPoke : noncopyable
{
public:
  NdnPoke(Face& face, KeyChain& keyChain, std::istream& input, const PokeOptions& options);

  enum class Result {
    DATA_SENT = 0,
    UNKNOWN = 1,
    // 2 is reserved for "malformed command line"
    TIMEOUT = 3,
    // 4 is reserved for "nack"
    PREFIX_REG_FAIL = 5,
  };

  /**
   * @return the result of NdnPoke execution
   */
  Result
  getResult() const
  {
    return m_result;
  }

  void
  start();

private:
  std::shared_ptr<Data>
  createData() const;

  void
  sendData(const Data& data);

  void
  onInterest(const Interest& interest, const Data& data);

  void
  onRegSuccess();

  void
  onRegFailure(std::string_view reason);

private:
  const PokeOptions m_options;
  Face& m_face;
  KeyChain& m_keyChain;
  std::istream& m_input;
  Scheduler m_scheduler;
  ScopedRegisteredPrefixHandle m_registeredPrefix;
  scheduler::ScopedEventId m_timeoutEvent;
  Result m_result = Result::UNKNOWN;
};

} // namespace ndn::peek

#endif // NDN_TOOLS_NDNPOKE_NDNPOKE_HPP
