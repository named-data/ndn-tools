/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020,  Arizona Board of Regents.
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
#include "tools/ping/server/ping-server.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"
#include "tests/key-chain-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace ping {
namespace tests {

using namespace ndn::tests;

class PingIntegratedFixture : public IoFixture, public KeyChainFixture
{
public:
  PingIntegratedFixture()
    : serverFace(m_io, m_keyChain, {false, true})
    , clientFace(m_io, m_keyChain, {false, true})
    , wantLoss(false)
  {
    serverFace.onSendInterest.connect([this] (const Interest& interest) {
      m_io.post([=] { if (!wantLoss) { clientFace.receive(interest); } });
    });
    clientFace.onSendInterest.connect([this] (const Interest& interest) {
      m_io.post([=] { if (!wantLoss) { serverFace.receive(interest); } });
    });
    serverFace.onSendData.connect([this] (const Data& data) {
      m_io.post([=] { if (!wantLoss) { clientFace.receive(data); } });
    });
    clientFace.onSendData.connect([this] (const Data& data) {
      m_io.post([=] { if (!wantLoss) { serverFace.receive(data); } });
    });
  }

  void onFinish()
  {
    serverFace.shutdown();
    clientFace.shutdown();
    m_io.stop();
  }

public:
  util::DummyClientFace serverFace;
  util::DummyClientFace clientFace;
  std::unique_ptr<server::PingServer> server;
  std::unique_ptr<client::Ping> client;
  bool wantLoss;
};

BOOST_AUTO_TEST_SUITE(Ping)
BOOST_AUTO_TEST_SUITE(TestIntegrated)

BOOST_FIXTURE_TEST_CASE(Normal, PingIntegratedFixture)
{
  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = 5_s;
  serverOpts.nMaxPings = 4;
  serverOpts.wantTimestamp = false;
  serverOpts.payloadSize = 0;
  server = make_unique<server::PingServer>(serverFace, m_keyChain, serverOpts);
  BOOST_REQUIRE_EQUAL(0, server->getNPings());
  server->start();

  client::Options clientOpts;
  clientOpts.prefix = "ndn:/test-prefix";
  clientOpts.shouldAllowStaleData = false;
  clientOpts.shouldGenerateRandomSeq = false;
  clientOpts.shouldPrintTimestamp = false;
  clientOpts.nPings = 4;
  clientOpts.interval = 100_ms;
  clientOpts.timeout = 2_s;
  clientOpts.startSeq = 1000;
  client = make_unique<client::Ping>(clientFace, clientOpts);
  client->afterFinish.connect([this] { onFinish(); });
  client->start();

  advanceClocks(1_ms, 400);
  m_io.run();

  BOOST_CHECK_EQUAL(4, server->getNPings());
}

BOOST_FIXTURE_TEST_CASE(Timeout, PingIntegratedFixture)
{
  wantLoss = true;

  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = 5_s;
  serverOpts.nMaxPings = 4;
  serverOpts.wantTimestamp = false;
  serverOpts.payloadSize = 0;
  server = make_unique<server::PingServer>(serverFace, m_keyChain, serverOpts);
  BOOST_REQUIRE_EQUAL(0, server->getNPings());
  server->start();

  client::Options clientOpts;
  clientOpts.prefix = "ndn:/test-prefix";
  clientOpts.shouldAllowStaleData = false;
  clientOpts.shouldGenerateRandomSeq = false;
  clientOpts.shouldPrintTimestamp = false;
  clientOpts.nPings = 4;
  clientOpts.interval = 100_ms;
  clientOpts.timeout = 500_ms;
  clientOpts.startSeq = 1000;
  client = make_unique<client::Ping>(clientFace, clientOpts);
  client->afterFinish.connect([this] { onFinish(); });
  client->start();

  advanceClocks(1_ms, 1000);
  m_io.run();

  BOOST_CHECK_EQUAL(0, server->getNPings());
}

BOOST_AUTO_TEST_SUITE_END() // TestIntegrated
BOOST_AUTO_TEST_SUITE_END() // Ping

} // namespace tests
} // namespace ping
} // namespace ndn
