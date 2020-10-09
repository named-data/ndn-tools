/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Arizona Board of Regents.
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

#include "tools/ping/server/ping-server.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace ping {
namespace server {
namespace tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(Ping)
BOOST_AUTO_TEST_SUITE(TestPingServer)

class CreatePingServerFixture : public IoFixture, public KeyChainFixture
{
protected:
  CreatePingServerFixture()
    : face(m_io, m_keyChain, {false, true})
    , pingOptions(makeOptions())
    , pingServer(face, m_keyChain, pingOptions)
  {
  }

  Interest
  makePingInterest(int seq) const
  {
    Name name(pingOptions.prefix);
    name.append("ping")
        .append(to_string(seq));

    return Interest(name)
           .setCanBePrefix(false)
           .setMustBeFresh(true)
           .setInterestLifetime(2_s);
  }

private:
  static Options
  makeOptions()
  {
    Options opt;
    opt.prefix = "/test-prefix";
    opt.freshnessPeriod = 5_s;
    opt.nMaxPings = 2;
    opt.payloadSize = 0;
    opt.wantTimestamp = false;
    opt.wantQuiet = true;
    return opt;
  }

protected:
  util::DummyClientFace face;
  Options pingOptions;
  PingServer pingServer;
};

BOOST_FIXTURE_TEST_CASE(CreatePingServer, CreatePingServerFixture)
{
  BOOST_REQUIRE_EQUAL(0, pingServer.getNPings());
  pingServer.start();

  advanceClocks(1_ms, 200);

  face.receive(makePingInterest(1000));
  face.receive(makePingInterest(1001));

  m_io.run();

  BOOST_CHECK_EQUAL(2, pingServer.getNPings());
}

BOOST_AUTO_TEST_SUITE_END() // TestPingServer
BOOST_AUTO_TEST_SUITE_END() // Ping

} // namespace tests
} // namespace server
} // namespace ping
} // namespace ndn
