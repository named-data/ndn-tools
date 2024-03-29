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

#ifndef NDN_TOOLS_TESTS_TEST_COMMON_HPP
#define NDN_TOOLS_TESTS_TEST_COMMON_HPP

#include "core/common.hpp"
#include "tests/boost-test.hpp"

#include <ndn-cxx/lp/nack.hpp>
#include <optional>

namespace ndn::tests {

/**
 * \brief Create an Interest.
 */
std::shared_ptr<Interest>
makeInterest(const Name& name, bool canBePrefix = false,
             std::optional<time::milliseconds> lifetime = std::nullopt,
             std::optional<Interest::Nonce> nonce = std::nullopt);

/**
 * \brief Create a Data with a null (i.e., empty) signature.
 *
 * If a "real" signature is desired, use KeyChainFixture and sign again with `m_keyChain`.
 */
std::shared_ptr<Data>
makeData(const Name& name);

/**
 * \brief Add a null signature to \p data.
 */
Data&
signData(Data& data);

/**
 * \brief Add a null signature to \p data.
 */
inline std::shared_ptr<Data>
signData(std::shared_ptr<Data> data)
{
  signData(*data);
  return data;
}

/**
 * \brief Create a Nack.
 */
lp::Nack
makeNack(Interest interest, lp::NackReason reason);

/**
 * \brief Replace a name component in a packet.
 * \param[in,out] pkt the packet
 * \param index the index of the name component to replace
 * \param args arguments to name::Component constructor
 */
template<typename Packet, typename ...Args>
void
setNameComponent(Packet& pkt, ssize_t index, Args&& ...args)
{
  Name name = pkt.getName();
  name.set(index, name::Component(std::forward<Args>(args)...));
  pkt.setName(name);
}

} // namespace ndn::tests

#endif // NDN_TOOLS_TESTS_TEST_COMMON_HPP
