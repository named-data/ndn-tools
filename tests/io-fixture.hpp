/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#ifndef NDN_TOOLS_TESTS_IO_FIXTURE_HPP
#define NDN_TOOLS_TESTS_IO_FIXTURE_HPP

#include "tests/clock-fixture.hpp"

#include <boost/asio/io_context.hpp>

namespace ndn::tests {

class IoFixture : public ClockFixture
{
private:
  void
  afterTick() final
  {
    if (m_io.stopped()) {
      m_io.restart();
    }
    m_io.poll();
  }

protected:
  boost::asio::io_context m_io;
};

} // namespace ndn::tests

#endif // NDN_TOOLS_TESTS_IO_FIXTURE_HPP
