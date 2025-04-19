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

#include "data-fetcher.hpp"

#include <boost/lexical_cast.hpp>

#include <cmath>
#include <iostream>

namespace ndn::get {

std::shared_ptr<DataFetcher>
DataFetcher::fetch(Face& face, const Interest& interest, int maxNackRetries, int maxTimeoutRetries,
                   DataCallback onData, FailureCallback onNack, FailureCallback onTimeout,
                   bool isVerbose)
{
  auto dataFetcher = std::shared_ptr<DataFetcher>(new DataFetcher(face,
                                                                  maxNackRetries,
                                                                  maxTimeoutRetries,
                                                                  std::move(onData),
                                                                  std::move(onNack),
                                                                  std::move(onTimeout),
                                                                  isVerbose));
  dataFetcher->expressInterest(interest, dataFetcher);
  return dataFetcher;
}

DataFetcher::DataFetcher(Face& face, int maxNackRetries, int maxTimeoutRetries,
                         DataCallback onData, FailureCallback onNack, FailureCallback onTimeout,
                         bool isVerbose)
  : m_face(face)
  , m_scheduler(m_face.getIoContext())
  , m_onData(std::move(onData))
  , m_onNack(std::move(onNack))
  , m_onTimeout(std::move(onTimeout))
  , m_maxNackRetries(maxNackRetries)
  , m_maxTimeoutRetries(maxTimeoutRetries)
  , m_isVerbose(isVerbose)
{
  BOOST_ASSERT(m_onData != nullptr);
}

void
DataFetcher::cancel()
{
  if (isRunning()) {
    m_isStopped = true;
    m_pendingInterest.cancel();
    m_scheduler.cancelAllEvents();
  }
}

void
DataFetcher::expressInterest(const Interest& interest, const std::shared_ptr<DataFetcher>& self)
{
  m_nCongestionRetries = 0;
  m_pendingInterest = m_face.expressInterest(interest,
    [this, self] (auto&&... args) { handleData(std::forward<decltype(args)>(args)..., self); },
    [this, self] (auto&&... args) { handleNack(std::forward<decltype(args)>(args)..., self); },
    [this, self] (auto&&... args) { handleTimeout(std::forward<decltype(args)>(args)..., self); });
}

void
DataFetcher::handleData(const Interest& interest, const Data& data,
                        const std::shared_ptr<DataFetcher>& self)
{
  if (!isRunning())
    return;

  m_isStopped = true;
  m_onData(interest, data);
}

void
DataFetcher::handleNack(const Interest& interest, const lp::Nack& nack,
                        const std::shared_ptr<DataFetcher>& self)
{
  if (!isRunning())
    return;

  if (m_maxNackRetries != MAX_RETRIES_INFINITE)
    ++m_nNacks;

  if (m_isVerbose)
    std::cerr << "Received Nack with reason " << nack.getReason()
              << " for Interest " << interest << "\n";

  if (m_nNacks <= m_maxNackRetries || m_maxNackRetries == MAX_RETRIES_INFINITE) {
    Interest newInterest(interest);
    newInterest.refreshNonce();

    switch (nack.getReason()) {
      case lp::NackReason::DUPLICATE: {
        expressInterest(newInterest, self);
        break;
      }
      case lp::NackReason::CONGESTION: {
        time::milliseconds backoffTime(static_cast<uint64_t>(std::pow(2, m_nCongestionRetries)));
        if (backoffTime > MAX_CONGESTION_BACKOFF_TIME) {
          backoffTime = MAX_CONGESTION_BACKOFF_TIME;
        }
        else {
          m_nCongestionRetries++;
        }
        m_scheduler.schedule(backoffTime, [this, newInterest, self] {
          expressInterest(newInterest, self);
        });
        break;
      }
      default: {
        m_hasError = true;
        if (m_onNack)
          m_onNack(interest, "Could not retrieve data for " + interest.getName().toUri() +
                             ", reason: " + boost::lexical_cast<std::string>(nack.getReason()));
        break;
      }
    }
  }
  else {
    m_hasError = true;
    if (m_onNack)
      m_onNack(interest, "Reached the maximum number of nack retries (" + std::to_string(m_maxNackRetries) +
                         ") while retrieving data for " + interest.getName().toUri());
  }
}

void
DataFetcher::handleTimeout(const Interest& interest, const std::shared_ptr<DataFetcher>& self)
{
  if (!isRunning())
    return;

  if (m_maxTimeoutRetries != MAX_RETRIES_INFINITE)
    ++m_nTimeouts;

  if (m_isVerbose)
    std::cerr << "Timeout for Interest " << interest << "\n";

  if (m_nTimeouts <= m_maxTimeoutRetries || m_maxTimeoutRetries == MAX_RETRIES_INFINITE) {
    Interest newInterest(interest);
    newInterest.refreshNonce();
    expressInterest(newInterest, self);
  }
  else {
    m_hasError = true;
    if (m_onTimeout)
      m_onTimeout(interest, "Reached the maximum number of timeout retries (" + std::to_string(m_maxTimeoutRetries) +
                            ") while retrieving data for " + interest.getName().toUri());
  }
}

} // namespace ndn::get
