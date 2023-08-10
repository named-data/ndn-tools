/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Arizona Board of Regents.
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

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn::ping::client::tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(Ping)
BOOST_AUTO_TEST_SUITE(TestClient)

using client::Ping;

BOOST_FIXTURE_TEST_CASE(Basic, IoFixture)
{
  DummyClientFace face(m_io, {true, true});
  Options pingOptions;
  pingOptions.prefix = "/test-prefix";
  pingOptions.shouldAllowStaleData = false;
  pingOptions.shouldGenerateRandomSeq = false;
  pingOptions.shouldPrintTimestamp = false;
  pingOptions.nPings = 4;
  pingOptions.interval = 100_ms;
  pingOptions.timeout = 2_s;
  pingOptions.startSeq = 1000;
  Ping ping(face, pingOptions);

  int nFinishSignals = 0;
  std::vector<uint64_t> dataSeqs;
  std::vector<uint64_t> nackSeqs;
  std::vector<uint64_t> timeoutSeqs;

  ping.afterData.connect([&] (uint64_t seq, auto&&...) { dataSeqs.push_back(seq); });
  ping.afterNack.connect([&] (uint64_t seq, auto&&...) { nackSeqs.push_back(seq); });
  ping.afterTimeout.connect([&] (uint64_t seq, auto&&...) { timeoutSeqs.push_back(seq); });
  ping.afterFinish.connect([&] {
    BOOST_REQUIRE_EQUAL(dataSeqs.size(), 2);
    BOOST_REQUIRE_EQUAL(nackSeqs.size(), 1);
    BOOST_REQUIRE_EQUAL(timeoutSeqs.size(), 1);

    BOOST_CHECK_EQUAL(dataSeqs[0], 1000);
    BOOST_CHECK_EQUAL(nackSeqs[0], 1001);
    BOOST_CHECK_EQUAL(dataSeqs[1], 1002);
    BOOST_CHECK_EQUAL(timeoutSeqs[0], 1003);

    nFinishSignals++;
  });

  ping.start();

  this->advanceClocks(1_ms, 500);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 4);

  auto data = makeData("/test-prefix/ping/1000");
  data->setFreshnessPeriod(1_s);
  face.receive(*data);

  face.receive(makeNack(face.sentInterests[1], lp::NackReason::DUPLICATE));

  data = makeData("/test-prefix/ping/1002");
  data->setFreshnessPeriod(1_s);
  face.receive(*data);

  this->advanceClocks(100_ms, 20);

  // /test-prefix/ping/1003 is unanswered and will timeout

  BOOST_CHECK_EQUAL(nFinishSignals, 1);
}

BOOST_AUTO_TEST_SUITE_END() // TestClient
BOOST_AUTO_TEST_SUITE_END() // Ping

} // namespace ndn::ping::client::tests
