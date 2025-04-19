/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2025, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Andrea Tosatto
 * @author Davide Pesavento
 */

#ifndef NDN_TOOLS_GET_DATA_FETCHER_HPP
#define NDN_TOOLS_GET_DATA_FETCHER_HPP

#include "core/common.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <functional>

namespace ndn::get {

/**
 * @brief Fetch data for a given interest and handle timeout or nack error with retries.
 *
 * To instantiate a DataFetcher you need to use the static method fetch, this will also express the
 * interest. After a timeout or nack is received, the same interest with a different nonce will be
 * requested for a maximum number of time specified by the class user. There are separate retry
 * counters for timeouts and nacks.
 *
 * A specified callback is called after the data matching the expressed interest is received. A
 * different callback is called in case one of the retries counter reach the maximum. This callback
 * can be different for timeout and nack. The data callback must be defined but the others callback
 * are optional.
 *
 */
class DataFetcher
{
public:
  /**
   * @brief means that there is no maximum number of retries,
   *        i.e. fetching must be retried indefinitely
   */
  static constexpr int MAX_RETRIES_INFINITE = -1;

  /**
   * @brief ceiling value for backoff time used in congestion handling
   */
  static constexpr time::milliseconds MAX_CONGESTION_BACKOFF_TIME = 10_s;

  using FailureCallback = std::function<void(const Interest& interest, const std::string& reason)>;

  /**
   * @brief instantiate a DataFetcher object and start fetching data
   *
   * @param onData callback for segment correctly received, must not be empty
   */
  static std::shared_ptr<DataFetcher>
  fetch(Face& face, const Interest& interest, int maxNackRetries, int maxTimeoutRetries,
        DataCallback onData, FailureCallback onTimeout, FailureCallback onNack,
        bool isVerbose);

  /**
   * @brief stop data fetching without error and calling any callback
   */
  void
  cancel();

  bool
  isRunning() const
  {
    return !m_isStopped && !m_hasError;
  }

  bool
  hasError() const
  {
    return m_hasError;
  }

private:
  DataFetcher(Face& face, int maxNackRetries, int maxTimeoutRetries,
              DataCallback onData, FailureCallback onNack, FailureCallback onTimeout,
              bool isVerbose);

  void
  expressInterest(const Interest& interest, const std::shared_ptr<DataFetcher>& self);

  void
  handleData(const Interest& interest, const Data& data, const std::shared_ptr<DataFetcher>& self);

  void
  handleNack(const Interest& interest, const lp::Nack& nack, const std::shared_ptr<DataFetcher>& self);

  void
  handleTimeout(const Interest& interest, const std::shared_ptr<DataFetcher>& self);

private:
  Face& m_face;
  Scheduler m_scheduler;
  PendingInterestHandle m_pendingInterest;
  DataCallback m_onData;
  FailureCallback m_onNack;
  FailureCallback m_onTimeout;

  int m_maxNackRetries;
  int m_maxTimeoutRetries;
  int m_nNacks = 0;
  int m_nTimeouts = 0;
  uint32_t m_nCongestionRetries = 0;

  bool m_isVerbose = false;
  bool m_isStopped = false;
  bool m_hasError = false;
};

} // namespace ndn::get

#endif // NDN_TOOLS_GET_DATA_FETCHER_HPP
