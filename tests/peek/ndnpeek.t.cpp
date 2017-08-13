/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Arizona Board of Regents.
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

#include <ndn-cxx/util/dummy-client-face.hpp>

#include <boost/mpl/vector.hpp>

namespace ndn {
namespace peek {
namespace tests {

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

class NdnPeekFixture : public UnitTestTimeFixture
{
protected:
  NdnPeekFixture()
    : face(io)
  {
  }

  void
  initialize(const PeekOptions& opts)
  {
    peek = make_unique<NdnPeek>(face, opts);
  }

protected:
  boost::asio::io_service io;
  ndn::util::DummyClientFace face;
  output_test_stream output;
  unique_ptr<NdnPeek> peek;
};

static PeekOptions
makeDefaultOptions()
{
  PeekOptions opt;
  opt.prefix = "ndn:/peek/test";
  opt.minSuffixComponents = -1;
  opt.maxSuffixComponents = -1;
  opt.interestLifetime = DEFAULT_INTEREST_LIFETIME;
  opt.timeout = time::milliseconds(200);
  opt.link = nullptr;
  opt.isVerbose = false;
  opt.mustBeFresh = false;
  opt.wantRightmostChild = false;
  opt.wantPayloadOnly = false;
  return opt;
}

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
    std::string expected(reinterpret_cast<const char*>(block.wire()), block.size());
    BOOST_CHECK(output.is_equal(expected));
  }

  static void
  checkOutput(output_test_stream& output, const lp::Nack& nack)
  {
    const Block& block = nack.getHeader().wireEncode();
    std::string expected(reinterpret_cast<const char*>(block.wire()), block.size());
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

using OutputChecks = boost::mpl::vector<OutputFull, OutputPayloadOnly>;

BOOST_AUTO_TEST_CASE_TEMPLATE(Default, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);

  auto data = makeData(options.prefix);
  std::string payload = "NdnPeekTest";
  data->setContent(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(io, time::milliseconds(25), 4);
    face.receive(*data);
  }

  OutputCheck::checkOutput(output, *data);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMaxSuffixComponents(), -1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMinSuffixComponents(), -1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK(face.sentInterests.back().getForwardingHint().empty());
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMustBeFresh(), false);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getChildSelector(), DEFAULT_CHILD_SELECTOR);
  BOOST_CHECK(peek->getResultCode() == ResultCode::DATA);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Selectors, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  options.minSuffixComponents = 1;
  options.maxSuffixComponents = 1;
  options.interestLifetime = time::milliseconds(200);
  options.mustBeFresh = true;
  options.wantRightmostChild = true;
  initialize(options);

  auto data = makeData(options.prefix);
  std::string payload = "NdnPeekTest";
  data->setContent(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(io, time::milliseconds(25), 4);
    face.receive(*data);
  }

  OutputCheck::checkOutput(output, *data);
  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMaxSuffixComponents(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMinSuffixComponents(), 1);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getInterestLifetime(), time::milliseconds(200));
  BOOST_CHECK_EQUAL(face.sentInterests.back().getForwardingHint().size(), 0);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getMustBeFresh(), true);
  BOOST_CHECK_EQUAL(face.sentInterests.back().getChildSelector(), 1);
  BOOST_CHECK(peek->getResultCode() == ResultCode::DATA);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ReceiveNackWithReason, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);
  lp::Nack nack;

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(io, time::milliseconds(25), 4);
    nack = makeNack(face.sentInterests.at(0), lp::NackReason::NO_ROUTE);
    face.receive(nack);
  }

  OutputCheck::checkOutput(output, nack);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResultCode() == ResultCode::NACK);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ReceiveNackWithoutReason, OutputCheck, OutputChecks)
{
  auto options = OutputCheck::makeOptions();
  initialize(options);
  lp::Nack nack;

  {
    CoutRedirector redir(output);
    peek->start();
    this->advanceClocks(io, time::milliseconds(25), 4);
    nack = makeNack(face.sentInterests.at(0), lp::NackReason::NONE);
    face.receive(nack);
  }

  OutputCheck::checkOutput(output, nack);
  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face.sentData.size(), 0);
  BOOST_CHECK_EQUAL(face.sentNacks.size(), 0);
  BOOST_CHECK(peek->getResultCode() == ResultCode::NACK);
}

BOOST_AUTO_TEST_CASE(TimeoutDefault)
{
  auto options = makeDefaultOptions();
  initialize(options);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(io, time::milliseconds(25), 4);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResultCode() == ResultCode::TIMEOUT);
}

BOOST_AUTO_TEST_CASE(TimeoutLessThanLifetime)
{
  auto options = makeDefaultOptions();
  options.interestLifetime = time::milliseconds(200);
  options.timeout = time::milliseconds(100);
  initialize(options);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(io, time::milliseconds(25), 8);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResultCode() == ResultCode::TIMEOUT);
}

BOOST_AUTO_TEST_CASE(TimeoutGreaterThanLifetime)
{
  auto options = makeDefaultOptions();
  options.interestLifetime = time::milliseconds(50);
  options.timeout = time::milliseconds(200);
  initialize(options);

  BOOST_REQUIRE_EQUAL(face.sentInterests.size(), 0);

  peek->start();
  this->advanceClocks(io, time::milliseconds(25), 4);

  BOOST_CHECK_EQUAL(face.sentInterests.size(), 1);
  BOOST_CHECK(peek->getResultCode() == ResultCode::TIMEOUT);
}

BOOST_AUTO_TEST_SUITE_END() // TestNdnPeek
BOOST_AUTO_TEST_SUITE_END() // Peek

} // namespace tests
} // namespace peek
} // namespace ndn
