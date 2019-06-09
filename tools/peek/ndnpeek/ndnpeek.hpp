/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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
 * @author Zhuo Li <zhuoli@email.arizona.edu>
 */

#ifndef NDN_TOOLS_NDNPEEK_NDNPEEK_HPP
#define NDN_TOOLS_NDNPEEK_NDNPEEK_HPP

#include "core/common.hpp"

#include <ndn-cxx/link.hpp>
#include <ndn-cxx/util/scheduler.hpp>

namespace ndn {
namespace peek {

/**
 * @brief options for NdnPeek
 */
struct PeekOptions
{
  // Interest construction options
  Name name;
  bool canBePrefix = false;
  bool mustBeFresh = false;
  shared_ptr<Link> link;
  optional<time::milliseconds> interestLifetime;

  // output behavior options
  bool isVerbose = false;
  bool wantPayloadOnly = false;
  optional<time::milliseconds> timeout;
};

enum class ResultCode {
  UNKNOWN = 1,
  DATA = 0,
  NACK = 4,
  TIMEOUT = 3,
};

class NdnPeek : boost::noncopyable
{
public:
  NdnPeek(Face& face, const PeekOptions& options);

  /**
   * @return the result of NdnPeek execution
   */
  ResultCode
  getResultCode() const
  {
    return m_resultCode;
  }

  /**
   * @brief express the Interest
   * @note The caller must invoke face.processEvents() afterwards
   */
  void
  start();

private:
  Interest
  createInterest() const;

  /**
   * @brief called when a Data packet is received
   */
  void
  onData(const Data& data);

  /**
   * @brief called when a Nack packet is received
   */
  void
  onNack(const lp::Nack& nack);

  /**
   * @brief called when the Interest times out
   */
  void
  onTimeout();

private:
  const PeekOptions& m_options;
  Face& m_face;
  Scheduler m_scheduler;
  time::steady_clock::TimePoint m_sendTime;
  ScopedPendingInterestHandle m_pendingInterest;
  scheduler::ScopedEventId m_timeoutEvent;
  ResultCode m_resultCode = ResultCode::UNKNOWN;
};

} // namespace peek
} // namespace ndn

#endif // NDN_TOOLS_NDNPEEK_NDNPEEK_HPP
