/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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

#include "test-common.hpp"
#include <ndn-cxx/security/signature-sha256-with-rsa.hpp>

namespace ndn {
namespace tests {

UnitTestTimeFixture::UnitTestTimeFixture()
  : steadyClock(make_shared<time::UnitTestSteadyClock>())
  , systemClock(make_shared<time::UnitTestSystemClock>())
{
  time::setCustomClocks(steadyClock, systemClock);
}

UnitTestTimeFixture::~UnitTestTimeFixture()
{
  time::setCustomClocks(nullptr, nullptr);
}

void
UnitTestTimeFixture::advanceClocks(boost::asio::io_service& io,
                                   time::nanoseconds tick, size_t nTicks)
{
  this->advanceClocks(io, tick, tick * nTicks);
}

void
UnitTestTimeFixture::advanceClocks(boost::asio::io_service& io,
                                   time::nanoseconds tick, time::nanoseconds total)
{
  BOOST_ASSERT(tick > time::nanoseconds::zero());
  BOOST_ASSERT(total >= time::nanoseconds::zero());

  time::nanoseconds remaining = total;
  while (remaining > time::nanoseconds::zero()) {
    if (remaining >= tick) {
      steadyClock->advance(tick);
      systemClock->advance(tick);
      remaining -= tick;
    }
    else {
      steadyClock->advance(remaining);
      systemClock->advance(remaining);
      remaining = time::nanoseconds::zero();
    }

    if (io.stopped())
      io.reset();
    io.poll();
  }
}

shared_ptr<Interest>
makeInterest(const Name& name, uint32_t nonce)
{
  auto interest = make_shared<Interest>(name);
  if (nonce != 0) {
    interest->setNonce(nonce);
  }
  return interest;
}

shared_ptr<Data>
makeData(const Name& name)
{
  auto data = make_shared<Data>(name);
  return signData(data);
}

Data&
signData(Data& data)
{
  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::encoding::makeEmptyBlock(tlv::SignatureValue));
  data.setSignature(fakeSignature);
  data.wireEncode();

  return data;
}

shared_ptr<Link>
makeLink(const Name& name, std::initializer_list<std::pair<uint32_t, Name>> delegations)
{
  auto link = make_shared<Link>(name, delegations);
  signData(link);
  return link;
}

lp::Nack
makeNack(const Name& name, uint32_t nonce, lp::NackReason reason)
{
  Interest interest(name);
  interest.setNonce(nonce);
  lp::Nack nack(std::move(interest));
  nack.setReason(reason);
  return nack;
}

} // namespace tests
} // namespace ndn
