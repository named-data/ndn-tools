/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "tests/test-common.hpp"

namespace ndn::tests {

std::shared_ptr<Interest>
makeInterest(const Name& name, bool canBePrefix, std::optional<time::milliseconds> lifetime,
             std::optional<Interest::Nonce> nonce)
{
  auto interest = std::make_shared<Interest>(name);
  interest->setCanBePrefix(canBePrefix);
  if (lifetime) {
    interest->setInterestLifetime(*lifetime);
  }
  interest->setNonce(nonce);
  return interest;
}

std::shared_ptr<Data>
makeData(const Name& name)
{
  auto data = std::make_shared<Data>(name);
  return signData(data);
}

Data&
signData(Data& data)
{
  data.setSignatureInfo(SignatureInfo(tlv::NullSignature));
  data.setSignatureValue(std::make_shared<Buffer>());
  data.wireEncode();
  return data;
}

lp::Nack
makeNack(Interest interest, lp::NackReason reason)
{
  lp::Nack nack(std::move(interest));
  nack.setReason(reason);
  return nack;
}

} // namespace ndn::tests
