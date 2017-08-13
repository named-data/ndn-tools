/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
 */

#ifndef NDN_TOOLS_TESTS_TEST_COMMON_HPP
#define NDN_TOOLS_TESTS_TEST_COMMON_HPP

#include "boost-test.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/util/time-unit-test-clock.hpp>

#include <boost/asio/io_service.hpp>

namespace ndn {
namespace tests {

/** \brief a test fixture that overrides steady clock and system clock
 */
class UnitTestTimeFixture
{
protected:
  UnitTestTimeFixture();

  ~UnitTestTimeFixture();

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p nTicks ticks.
   *  After each tick, the supplied io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(boost::asio::io_service& io,
                time::nanoseconds tick, size_t nTicks = 1);

  /** \brief advance steady and system clocks
   *
   *  Clocks are advanced in increments of \p tick for \p total time.
   *  The last increment might be shorter than \p tick.
   *  After each tick, the supplied io_service is polled to process pending I/O events.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancing would stop in case of an exception.
   */
  void
  advanceClocks(boost::asio::io_service& io,
                time::nanoseconds tick, time::nanoseconds total);

protected:
  shared_ptr<time::UnitTestSteadyClock> steadyClock;
  shared_ptr<time::UnitTestSystemClock> systemClock;
};

/** \brief create an Interest
 *  \param name Interest name
 *  \param nonce if non-zero, set Nonce to this value
 *               (useful for creating Nack with same Nonce)
 */
shared_ptr<Interest>
makeInterest(const Name& name, uint32_t nonce = 0);

/** \brief create a Data with fake signature
 *  \note Data may be modified afterwards without losing the fake signature.
 *        If a real signature is desired, sign again with KeyChain.
 */
shared_ptr<Data>
makeData(const Name& name);

/** \brief add a fake signature to Data
 */
Data&
signData(Data& data);

/** \brief add a fake signature to Data
 */
inline shared_ptr<Data>
signData(shared_ptr<Data> data)
{
  signData(*data);
  return data;
}

/** \brief create a Nack
 *  \param interest Interest
 *  \param reason Nack reason
 */
lp::Nack
makeNack(const Interest& interest, lp::NackReason reason);

/** \brief create a Nack
 *  \param name Interest name
 *  \param nonce Interest nonce
 *  \param reason Nack reason
 */
lp::Nack
makeNack(const Name& name, uint32_t nonce, lp::NackReason reason);

} // namespace tests
} // namespace ndn

#endif // NDN_TOOLS_TESTS_TEST_COMMON_HPP
