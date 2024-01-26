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
 */

#ifndef NDN_TOOLS_TESTS_CLOCK_FIXTURE_HPP
#define NDN_TOOLS_TESTS_CLOCK_FIXTURE_HPP

#include <ndn-cxx/util/time-unit-test-clock.hpp>

namespace ndn::tests {

/** \brief A test fixture that overrides steady clock and system clock.
 */
class ClockFixture
{
public:
  virtual
  ~ClockFixture();

  /** \brief Advance steady and system clocks.
   *
   *  Clocks are advanced in increments of \p tick for \p nTicks ticks.
   *  afterTick() is called after each tick.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancement will stop in the event of an exception.
   */
  void
  advanceClocks(time::nanoseconds tick, size_t nTicks = 1)
  {
    advanceClocks(tick, tick * nTicks);
  }

  /** \brief Advance steady and system clocks.
   *
   *  Clocks are advanced in increments of \p tick for \p total time.
   *  The last increment might be shorter than \p tick.
   *  afterTick() is called after each tick.
   *
   *  Exceptions thrown during I/O events are propagated to the caller.
   *  Clock advancement will stop in the event of an exception.
   */
  void
  advanceClocks(time::nanoseconds tick, time::nanoseconds total);

protected:
  ClockFixture();

private:
  /** \brief Called by advanceClocks() after each clock advancement (tick).
   *
   *  The base class implementation is a no-op.
   */
  virtual void
  afterTick()
  {
  }

protected:
  std::shared_ptr<time::UnitTestSteadyClock> m_steadyClock;
  std::shared_ptr<time::UnitTestSystemClock> m_systemClock;
};

} // namespace ndn::tests

#endif // NDN_TOOLS_TESTS_CLOCK_FIXTURE_HPP
