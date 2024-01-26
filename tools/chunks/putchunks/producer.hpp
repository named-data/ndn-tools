/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2024, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Klaus Schneider
 */

#ifndef NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP
#define NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP

#include "core/common.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <vector>

namespace ndn::chunks {

/**
 * @brief Segmented & versioned data publisher.
 *
 * Packetizes and publishes data from an input stream as `/prefix/<version>/<segment number>`.
 * Unless another value is provided, the current time is used as the version number.
 * The packet store always has at least one item, even when the input is empty.
 */
class Producer : noncopyable
{
public:
  struct Options
  {
    security::SigningInfo signingInfo;
    time::milliseconds freshnessPeriod = 10_s;
    size_t maxSegmentSize = 8000;
    bool isQuiet = false;
    bool isVerbose = false;
    bool wantShowVersion = false;
  };

  /**
   * @brief Create the producer.
   * @param prefix prefix used to publish data; if the last component is not a valid
   *               version number, the current system time is used as version number.
   */
  Producer(const Name& prefix, Face& face, KeyChain& keyChain, std::istream& is,
           const Options& opts);

  /**
   * @brief Run the producer.
   */
  void
  run();

private:
  /**
   * @brief Respond with a metadata packet containing the versioned content name.
   */
  void
  processDiscoveryInterest(const Interest& interest);

  /**
   * @brief Respond with the requested segment of content.
   */
  void
  processSegmentInterest(const Interest& interest);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::vector<std::shared_ptr<Data>> m_store;

private:
  Name m_prefix;
  Name m_versionedPrefix;
  Face& m_face;
  KeyChain& m_keyChain;
  const Options m_options;
};

} // namespace ndn::chunks

#endif // NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP
