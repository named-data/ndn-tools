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
#include "tools/ping/client/ping.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/test-common.hpp"
#include "../identity-management-time-fixture.hpp"

namespace ndn {
namespace ping {
namespace tests {

using namespace ndn::tests;

class PingIntegratedFixture : public IdentityManagementTimeFixture
{
public:
  PingIntegratedFixture()
    : serverFace(io, m_keyChain, {false, true})
    , clientFace(io, m_keyChain, {false, true})
    , wantLoss(false)
  {
    serverFace.onSendInterest.connect([this] (const Interest& interest) {
      io.post([=] { if (!wantLoss) { clientFace.receive(interest); } });
    });
    clientFace.onSendInterest.connect([this] (const Interest& interest) {
      io.post([=] { if (!wantLoss) { serverFace.receive(interest); } });
    });
    serverFace.onSendData.connect([this] (const Data& data) {
      io.post([=] { if (!wantLoss) { clientFace.receive(data); } });
    });
    clientFace.onSendData.connect([this] (const Data& data) {
      io.post([=] { if (!wantLoss) { serverFace.receive(data); } });
    });
  }

  void onFinish()
  {
    serverFace.shutdown();
    clientFace.shutdown();
    io.stop();
  }

public:
  boost::asio::io_service io;
  util::DummyClientFace serverFace;
  util::DummyClientFace clientFace;
  std::unique_ptr<server::PingServer> server;
  std::unique_ptr<client::Ping> client;
  bool wantLoss;
};

BOOST_AUTO_TEST_SUITE(PingIntegrated)

BOOST_FIXTURE_TEST_CASE(Normal, PingIntegratedFixture)
{
  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = time::milliseconds(5000);
  serverOpts.nMaxPings = 4;
  serverOpts.shouldPrintTimestamp = false;
  serverOpts.payloadSize = 0;
  server.reset(new server::PingServer(serverFace, m_keyChain, serverOpts));
  BOOST_REQUIRE_EQUAL(0, server->getNPings());
  server->start();

  client::Options clientOpts;
  clientOpts.prefix = "ndn:/test-prefix";
  clientOpts.shouldAllowStaleData = false;
  clientOpts.shouldGenerateRandomSeq = false;
  clientOpts.shouldPrintTimestamp = false;
  clientOpts.nPings = 4;
  clientOpts.interval = time::milliseconds(100);
  clientOpts.timeout = time::milliseconds(2000);
  clientOpts.startSeq = 1000;
  client.reset(new client::Ping(clientFace, clientOpts));
  client->afterFinish.connect(bind(&PingIntegratedFixture::onFinish, this));
  client->start();

  this->advanceClocks(io, time::milliseconds(1), 400);
  io.run();

  BOOST_REQUIRE_EQUAL(4, server->getNPings());
}

BOOST_FIXTURE_TEST_CASE(Timeout, PingIntegratedFixture)
{
  wantLoss = true;

  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = time::milliseconds(5000);
  serverOpts.nMaxPings = 4;
  serverOpts.shouldPrintTimestamp = false;
  serverOpts.payloadSize = 0;
  server.reset(new server::PingServer(serverFace, m_keyChain, serverOpts));
  BOOST_REQUIRE_EQUAL(0, server->getNPings());
  server->start();

  client::Options clientOpts;
  clientOpts.prefix = "ndn:/test-prefix";
  clientOpts.shouldAllowStaleData = false;
  clientOpts.shouldGenerateRandomSeq = false;
  clientOpts.shouldPrintTimestamp = false;
  clientOpts.nPings = 4;
  clientOpts.interval = time::milliseconds(100);
  clientOpts.timeout = time::milliseconds(500);
  clientOpts.startSeq = 1000;
  client.reset(new client::Ping(clientFace, clientOpts));
  client->afterFinish.connect(bind(&PingIntegratedFixture::onFinish, this));
  client->start();

  this->advanceClocks(io, time::milliseconds(1), 1000);
  io.run();

  BOOST_REQUIRE_EQUAL(0, server->getNPings());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ping
} // namespace ndn
