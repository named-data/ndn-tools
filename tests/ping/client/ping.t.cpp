/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Arizona Board of Regents.
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

class SequenceNumberIncrementFixture : public UnitTestTimeFixture
{
protected:
  SequenceNumberIncrementFixture()
    : face(util::makeDummyClientFace(io, {false, true}))
    , pingOptions(makeOptions())
    , ping(*face, pingOptions)
    , numPings(0)
    , lastPingSeq(pingOptions.startSeq)
  {
    ping.afterResponse.connect(bind(&SequenceNumberIncrementFixture::onResponse, this, _1));
    ping.afterTimeout.connect(bind(&SequenceNumberIncrementFixture::onTimeout, this, _1));
  }

public:
  void
  onResponse(uint64_t seq)
  {
    numPings++;
    lastPingSeq = seq;
    if (numPings == maxPings) {
      face->shutdown();
      io.stop();
    }
  }

  void
  onTimeout(uint64_t seq)
  {
    numPings++;
    lastPingSeq = seq;
    if (numPings == maxPings) {
      face->shutdown();
      io.stop();
    }
  }

private:
  static Options
  makeOptions()
  {
    Options opt;
    opt.prefix = "ndn:/test-prefix";
    opt.shouldAllowStaleData = false;
    opt.shouldGenerateRandomSeq = false;
    opt.shouldPrintTimestamp = false;
    opt.nPings = 4;
    opt.interval = time::milliseconds(100);
    opt.timeout = time::milliseconds(1000);
    opt.startSeq = 1000;
    return opt;
  }

protected:
  boost::asio::io_service io;
  shared_ptr<util::DummyClientFace> face;
  Options pingOptions;
  Ping ping;
  uint32_t numPings;
  uint32_t maxPings;
  uint64_t lastPingSeq;
  KeyChain keyChain;
};

BOOST_FIXTURE_TEST_CASE(SequenceNumberIncrement, SequenceNumberIncrementFixture)
{
  maxPings = 4;
  ping.start();

  this->advanceClocks(io, time::milliseconds(1), 400);

  face->receive(*makeData("ndn:/test-prefix/ping/1000"));
  face->receive(*makeData("ndn:/test-prefix/ping/1001"));
  face->receive(*makeData("ndn:/test-prefix/ping/1002"));
  face->receive(*makeData("ndn:/test-prefix/ping/1003"));

  io.run();

  BOOST_REQUIRE_EQUAL(1003, lastPingSeq);
  BOOST_REQUIRE_EQUAL(4, numPings);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace client
} // namespace ping
} // namespace ndn
