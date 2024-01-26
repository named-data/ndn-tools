/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Arizona Board of Regents.
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

#include "tools/peek/ndnpeek/ndnpeek.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/test/tools/output_test_stream.hpp>

namespace ndn::peek::tests {

using namespace ndn::tests;
using boost::test_tools::output_test_stream;

class CoutRedirector : noncopyable
{
public:
  explicit
  CoutRedirector(std::ostream& destination)
  {
    m_originalBuf = std::cout.rdbuf(destination.rdbuf());
  }

  ~CoutRedirector()
  {
    std::cout.rdbuf(m_originalBuf);
  }

private:
  std::streambuf* m_originalBuf;
};

static PeekOptions
makeDefaultOptions()
{
  PeekOptions opt;
  opt.name = "/peek/test";
  return opt;
}

class NdnPeekFixture : public IoFixture
{
protected:
  void
  initialize(const PeekOptions& opts = makeDefaultOptions())
  {
    peek = std::make_unique<NdnPeek>(face, opts);
  }

protected:
  DummyClientFace face{m_io};
  output_test_stream output;
  std::unique_ptr<NdnPeek> peek;
};

class OutputFull
{
public:
  static PeekOptions
  makeOptions()
  {
    return makeDefaultOptions();
  }

  static void
  checkOutput(output_test_stream& output, const Data& data)
  {
    const Block& block = data.wireEncode();
    std::string expected(reinterpret_cast<const char*>(block.data()), block.size());
    BOOST_CHECK(output.is_equal(expected));
  }

  static void
  checkOutput(output_test_stream& output, const lp::Nack& nack)
  {
    const Block& block = nack.getHeader().wireEncode();
    std::string expected(reinterpret_cast<const char*>(block.data()), block.size());
    BOOST_CHECK(output.is_equal(expected));
  }
};

class OutputPayloadOnly
{
public:
  static PeekOptions
  makeOptions()
  {
    PeekOptions opt = makeDefaultOptions();
    opt.wantPayloadOnly = true;
    return opt;
  }

  static void
  checkOutput(output_test_stream& output, const Data& data)
  {
    const Block& block = data.getContent();
    std::string expected(reinterpret_cast<const char*>(block.value()), block.value_size());
    BOOST_CHECK(output.is_equal(expected));
  }

  static void
  checkOutput(output_test_stream& output, const lp::Nack& nack)
  {
    std::string expected = boost::lexical_cast<std::string>(nack.getReason()) + '\n';
    BOOST_CHECK(output.is_equal(expected));
  }
};

BOOST_AUTO_TEST_SUITE(Peek)
BOOST_FIXTURE_TEST_SUITE(TestNdnPeek, NdnPeekFixture)

using OutputChecks = std::tuple<OutputFull, OutputPayloadOnly>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Default, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);

  auto data = makeData(options.name);
  data->setContent({'n', 'd', 'n', 'p', 'e', 'e', 'k'});

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(25_ms, 4);
    face.receive(*data);
  }

  OutputCheck::checkOutput(output, *data);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const auto& interest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(interest.getCanBePrefix(), false);
  BOOST_CHECK_EQUAL(interest.getMustBeFresh(), false);
  BOOST_CHECK_EQUAL(interest.getForwardingHint().empty(), true);
  BOOST_CHECK_EQUAL(interest.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK(interest.getHopLimit() == std::nullopt);
  BOOST_CHECK(!interest.hasApplicationParameters());
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::DATA);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(NonDefault, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  options.canBePrefix = true;
  options.mustBeFresh = true;
  options.forwardingHint.emplace_back("/fh");
  options.interestLifetime = 200_ms;
  options.hopLimit = 64;
  initialize(options);

  auto data = makeData(Name(options.name).append("suffix"));
  data->setFreshnessPeriod(1_s);
  data->setContent({'n', 'd', 'n', 'p', 'e', 'e', 'k'});

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(25_ms, 4);
    face.receive(*data);
  }

  OutputCheck::checkOutput(output, *data);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  const auto& interest = face.sentInterests.back();
  BOOST_CHECK_EQUAL(interest.getCanBePrefix(), true);
  BOOST_CHECK_EQUAL(interest.getMustBeFresh(), true);
  BOOST_TEST(interest.getForwardingHint() == std::vector<Name>({"/fh"}),
             boost::test_tools::per_element());
  BOOST_CHECK_EQUAL(interest.getInterestLifetime(), 200_ms);
  BOOST_CHECK(interest.getHopLimit() == 64);
  BOOST_CHECK(!interest.hasApplicationParameters());
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::DATA);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ReceiveNackWithReason, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);
  lp::Nack nack;

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(25_ms, 4);
    nack = makeNack(face.sentInterests.at(0), lp::NackReason::NO_ROUTE);
    face.receive(nack);
  }

  OutputCheck::checkOutput(output, nack);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::NACK);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ReceiveNackWithoutReason, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);
  lp::Nack nack;

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(25_ms, 4);
    nack = makeNack(face.sentInterests.at(0), lp::NackReason::NONE);
    face.receive(nack);
  }

  OutputCheck::checkOutput(output, nack);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::NACK);
}

BOOST_AUTO_TEST_CASE(ApplicationParameters)
{
  auto options = makeDefaultOptions();
  options.applicationParameters = std::make_shared<Buffer>("hello", 5);
  initialize(options);

  peek->start();
  this->advanceClocks(25_ms, 4);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getCanBePrefix(), false);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMustBeFresh(), false);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getForwardingHint().empty(), true);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK_EQUAL(face.sentInterests.back().hasApplicationParameters(), true);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getApplicationParameters(), "2405 68656C6C6F"_block);
}

BOOST_AUTO_TEST_CASE(NoTimeout)
{
  auto options = makeDefaultOptions();
  options.interestLifetime = 1_s;
  options.timeout = std::nullopt;
  initialize(options);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(100_ms, 9);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 1);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::UNKNOWN);

  this->advanceClocks(100_ms, 2);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::TIMEOUT);
}

BOOST_AUTO_TEST_CASE(TimeoutLessThanLifetime)
{
  auto options = makeDefaultOptions();
  options.interestLifetime = 200_ms;
  options.timeout = 100_ms;
  initialize(options);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(25_ms, 6);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::TIMEOUT);
}

BOOST_AUTO_TEST_CASE(TimeoutGreaterThanLifetime)
{
  auto options = makeDefaultOptions();
  options.interestLifetime = 50_ms;
  options.timeout = 200_ms;
  initialize(options);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(25_ms, 4);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.getNPendingInterests(), 0);
  BOOST_CHECK(peek->getResult() == NdnPeek::Result::TIMEOUT);
}

BOOST_AUTO_TEST_CASE(OversizedPacket)
{
  auto options = makeDefaultOptions();
  options.applicationParameters = std::make_shared<Buffer>(MAX_NDN_PACKET_SIZE);
  initialize(options);

  peek->start();
  BOOST_CHECK_THROW(this->advanceClocks(1_ms, 10), Face::OversizedPacketError);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestNdnPeek
BOOST_AUTO_TEST_SUITE_END() // Peek

} // namespace ndn::peek::tests
