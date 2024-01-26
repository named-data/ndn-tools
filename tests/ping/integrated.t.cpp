/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2024,  Arizona Board of Regents.
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
#include <boost/asio/post.hpp>

namespace ndn::ping::tests {

using namespace ndn::tests;

class PingIntegratedFixture : public IoFixture, public KeyChainFixture
{
protected:
  PingIntegratedFixture()
  {
    serverFace.onSendInterest.connect([this] (const auto& interest) {
      receive(clientFace, interest);
    });
    clientFace.onSendInterest.connect([this] (const auto& interest) {
      receive(serverFace, interest);
    });
    serverFace.onSendData.connect([this] (const auto& data) {
      receive(clientFace, data);
    });
    clientFace.onSendData.connect([this] (const auto& data) {
      receive(serverFace, data);
    });
  }

  template<typename Packet>
  void
  receive(DummyClientFace& face, const Packet& pkt)
  {
    boost::asio::post(m_io, [&face, pkt, wantLoss = wantLoss] {
      if (!wantLoss) {
        face.receive(pkt);
      }
    });
  }

  void
  onFinish()
  {
    serverFace.shutdown();
    clientFace.shutdown();
    m_io.stop();
  }

protected:
  DummyClientFace serverFace{m_io, m_keyChain, {false, true}};
  DummyClientFace clientFace{m_io, m_keyChain, {false, true}};
  std::unique_ptr<server::PingServer> server;
  std::unique_ptr<client::Ping> client;
  bool wantLoss = false;
};

BOOST_AUTO_TEST_SUITE(Ping)
BOOST_FIXTURE_TEST_SUITE(TestIntegrated, PingIntegratedFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = 5_s;
  serverOpts.nMaxPings = 4;
  serverOpts.wantTimestamp = false;
  serverOpts.payloadSize = 0;
  server = std::make_unique<server::PingServer>(serverFace, m_keyChain, serverOpts);
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
  client = std::make_unique<client::Ping>(clientFace, clientOpts);
  client->afterFinish.connect([this] { onFinish(); });
  client->start();

  advanceClocks(1_ms, 400);
  m_io.run();

  BOOST_CHECK_EQUAL(4, server->getNPings());
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  wantLoss = true;

  server::Options serverOpts;
  serverOpts.prefix = "ndn:/test-prefix";
  serverOpts.freshnessPeriod = 5_s;
  serverOpts.nMaxPings = 4;
  serverOpts.wantTimestamp = false;
  serverOpts.payloadSize = 0;
  server = std::make_unique<server::PingServer>(serverFace, m_keyChain, serverOpts);
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
  client = std::make_unique<client::Ping>(clientFace, clientOpts);
  client->afterFinish.connect([this] { onFinish(); });
  client->start();

  advanceClocks(1_ms, 1000);
  m_io.run();

  BOOST_CHECK_EQUAL(0, server->getNPings());
}

BOOST_AUTO_TEST_SUITE_END() // TestIntegrated
BOOST_AUTO_TEST_SUITE_END() // Ping

} // namespace ndn::ping::tests
