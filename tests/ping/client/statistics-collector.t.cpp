/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Arizona Board of Regents.
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

#include "tools/ping/client/statistics-collector.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/test-common.hpp"

namespace ndn {
namespace ping {
namespace client {
namespace tests {

using namespace ndn::tests;

class StatisticsCollectorFixture
{
protected:
  StatisticsCollectorFixture()
    : pingOptions(makeOptions())
    , pingProgram(face, pingOptions)
    , sc(pingProgram, pingOptions)
  {
  }

private:
  static Options
  makeOptions()
  {
    Options opt;
    opt.prefix = "ndn:/ping-prefix";
    opt.shouldAllowStaleData = false;
    opt.shouldGenerateRandomSeq = false;
    opt.shouldPrintTimestamp = false;
    opt.nPings = 5;
    opt.interval = time::milliseconds(100);
    opt.timeout = time::milliseconds(2000);
    opt.startSeq = 1;
    return opt;
  }

protected:
  util::DummyClientFace face;
  Options pingOptions;
  Ping pingProgram;
  StatisticsCollector sc;
};

BOOST_FIXTURE_TEST_SUITE(PingClientStatisticsCollector, StatisticsCollectorFixture)

BOOST_AUTO_TEST_CASE(Resp50msResp50ms)
{
  sc.recordData(time::milliseconds(50));

  Statistics stats1 = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats1.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats1.nSent, 1);
  BOOST_CHECK_EQUAL(stats1.nReceived, 1);
  BOOST_CHECK_CLOSE(stats1.minRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats1.maxRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats1.packetLossRate, 0.0, 0.001);
  BOOST_CHECK_CLOSE(stats1.sumRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats1.avgRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats1.stdDevRtt, 0.0, 0.001);

  sc.recordData(time::milliseconds(50));

  Statistics stats2 = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats2.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats2.nSent, 2);
  BOOST_CHECK_EQUAL(stats2.nReceived, 2);
  BOOST_CHECK_CLOSE(stats2.minRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats2.maxRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats2.packetLossRate, 0.0, 0.001);
  BOOST_CHECK_CLOSE(stats2.sumRtt, 100.0, 0.001);
  BOOST_CHECK_CLOSE(stats2.avgRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats2.stdDevRtt, 0.0, 0.001);
}

BOOST_AUTO_TEST_CASE(Resp50msResp100ms)
{
  sc.recordData(time::milliseconds(50));
  sc.recordData(time::milliseconds(100));

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 2);
  BOOST_CHECK_CLOSE(stats.minRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.maxRtt, 100.0, 0.001);
  BOOST_CHECK_CLOSE(stats.packetLossRate, 0.0, 0.001);
  BOOST_CHECK_CLOSE(stats.sumRtt, 150.0, 0.001);
  BOOST_CHECK_CLOSE(stats.avgRtt, 75.0, 0.001);
  BOOST_CHECK_CLOSE(stats.stdDevRtt, 25.0, 0.001);
}

BOOST_AUTO_TEST_CASE(LossLoss)
{
  sc.recordTimeout();
  sc.recordTimeout();

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 0);
  BOOST_CHECK_CLOSE(stats.packetLossRate, 1.0, 0.001);
}

BOOST_AUTO_TEST_CASE(Resp50msLoss)
{
  sc.recordData(time::milliseconds(50));
  sc.recordTimeout();

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 1);
  BOOST_CHECK_CLOSE(stats.minRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.maxRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.packetLossRate, 0.5, 0.001);
  BOOST_CHECK_CLOSE(stats.sumRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.avgRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.stdDevRtt, 0.0, 0.001);
}

BOOST_AUTO_TEST_CASE(NackNack)
{
  sc.recordNack();
  sc.recordNack();

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nNacked, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 0);
  BOOST_CHECK_CLOSE(stats.packetNackedRate, 1.0, 0.001);
}

BOOST_AUTO_TEST_CASE(Resp50msNack)
{
  sc.recordData(time::milliseconds(50));
  sc.recordNack();

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 1);
  BOOST_CHECK_CLOSE(stats.minRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.maxRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.sumRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.avgRtt, 50.0, 0.001);
  BOOST_CHECK_CLOSE(stats.stdDevRtt, 0.0, 0.001);
  BOOST_CHECK_EQUAL(stats.nNacked, 1);
  BOOST_CHECK_CLOSE(stats.packetNackedRate, 0.5, 0.001);
}

BOOST_AUTO_TEST_CASE(NackLoss)
{
  sc.recordNack();
  sc.recordTimeout();

  Statistics stats = sc.computeStatistics();
  BOOST_CHECK_EQUAL(stats.prefix, pingOptions.prefix);
  BOOST_CHECK_EQUAL(stats.nSent, 2);
  BOOST_CHECK_EQUAL(stats.nReceived, 0);
  BOOST_CHECK_EQUAL(stats.nNacked, 1);
  BOOST_CHECK_CLOSE(stats.packetNackedRate, 0.5, 0.001);
  BOOST_CHECK_CLOSE(stats.packetLossRate, 0.5, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace client
} // namespace ping
} // namespace ndn
