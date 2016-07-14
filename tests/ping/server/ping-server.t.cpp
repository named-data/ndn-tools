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

#include "tools/ping/server/ping-server.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/test-common.hpp"
#include "../../identity-management-time-fixture.hpp"

namespace ndn {
namespace ping {
namespace server {
namespace tests {

using namespace ndn::tests;

BOOST_AUTO_TEST_SUITE(PingServerPingServer)

class CreatePingServerFixture : public IdentityManagementTimeFixture
{
protected:
  CreatePingServerFixture()
    : face(io, m_keyChain, {false, true})
    , pingOptions(makeOptions())
    , pingServer(face, m_keyChain, pingOptions)
  {
  }

  Interest
  makePingInterest(int seq) const
  {
    Name name(pingOptions.prefix);
    name.append("ping");
    name.append(std::to_string(seq));
    Interest interest(name);
    interest.setMustBeFresh(true);
    interest.setInterestLifetime(time::milliseconds(2000));
    return interest;
  }

private:
  static Options
  makeOptions()
  {
    Options opt;
    opt.prefix = "ndn:/test-prefix";
    opt.freshnessPeriod = time::milliseconds(5000);
    opt.nMaxPings = 2;
    opt.shouldPrintTimestamp = false;
    opt.payloadSize = 0;
    return opt;
  }

protected:
  boost::asio::io_service io;
  util::DummyClientFace face;
  Options pingOptions;
  PingServer pingServer;
};

BOOST_FIXTURE_TEST_CASE(CreatePingServer, CreatePingServerFixture)
{
  BOOST_REQUIRE_EQUAL(0, pingServer.getNPings());
  pingServer.start();

  this->advanceClocks(io, time::milliseconds(1), 200);

  face.receive(makePingInterest(1000));
  face.receive(makePingInterest(1001));

  io.run();

  BOOST_REQUIRE_EQUAL(2, pingServer.getNPings());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
} // namespace ping
} // namespace ndn
