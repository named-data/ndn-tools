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

#include "tools/ping/client/ping.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/test-common.hpp"

namespace ndn {
namespace ping {
namespace client {
namespace tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(PingClientPing)

BOOST_FIXTURE_TEST_CASE(Basic, UnitTestTimeFixture)
{
  Options pingOptions;
  pingOptions.prefix = "ndn:/test-prefix";
  pingOptions.shouldAllowStaleData = false;
  pingOptions.shouldGenerateRandomSeq = false;
  pingOptions.shouldPrintTimestamp = false;
  pingOptions.nPings = 4;
  pingOptions.interval = time::milliseconds(100);
  pingOptions.timeout = time::milliseconds(2000);
  pingOptions.startSeq = 1000;

  boost::asio::io_service io;
  util::DummyClientFace face(io, {true, true});
  Ping ping(face, pingOptions);

  int nFinishSignals = 0;
  std::vector<uint64_t> dataSeqs;
  std::vector<uint64_t> nackSeqs;
  std::vector<uint64_t> timeoutSeqs;

  ping.afterData.connect(bind([&] (uint64_t seq) { dataSeqs.push_back(seq); }, _1));
  ping.afterNack.connect(bind([&] (uint64_t seq) { nackSeqs.push_back(seq); }, _1));
  ping.afterTimeout.connect(bind([&] (uint64_t seq) { timeoutSeqs.push_back(seq); }, _1));
  ping.afterFinish.connect(bind([&] {
    BOOST_REQUIRE_EQUAL(dataSeqs.size(), 2);
    BOOST_REQUIRE_EQUAL(nackSeqs.size(), 1);
    BOOST_REQUIRE_EQUAL(timeoutSeqs.size(), 1);

    BOOST_CHECK_EQUAL(dataSeqs[0], 1000);
    BOOST_CHECK_EQUAL(nackSeqs[0], 1001);
    BOOST_CHECK_EQUAL(dataSeqs[1], 1002);
    BOOST_CHECK_EQUAL(timeoutSeqs[0], 1003);

    nFinishSignals++;
  }));

  ping.start();

  this->advanceClocks(io, time::milliseconds(1), 500);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 4);

  face.receive(*makeData("ndn:/test-prefix/ping/1000"));

  lp::Nack nack(face.sentInterests[1]);
  nack.setReason(lp::NackReason::DUPLICATE);
  face.receive(nack);

  face.receive(*makeData("ndn:/test-prefix/ping/1002"));

  this->advanceClocks(io, time::milliseconds(100), 20);

  // ndn:/test-prefix/ping/1003 is unanswered and will timeout

  BOOST_CHECK_EQUAL(nFinishSignals, 1);

  face.shutdown();
  io.stop();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace client
} // namespace ping
} // namespace ndn
