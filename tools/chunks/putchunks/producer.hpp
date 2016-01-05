/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2016,  Regents of the University of California,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University.
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
 */

#ifndef NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP
#define NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP

#include "core/common.hpp"

namespace ndn {
namespace chunks {

/**
 * @brief Segmented version Producer
 *
 * Packetizes and publishes data from an input stream under /prefix/<version>/<segment number>.
 * The current time is used as the version number. The store has always at least one element (also
 * with empty input stream).
 */
class Producer : noncopyable
{
public:
  /**
   * @brief Create the Producer
   *
   * @prefix prefix used to publish data, if the last component of prefix is not a version number
   *         the current time is used as version number.
   */
  Producer(const Name& prefix, Face& face, KeyChain& keyChain,
           const security::SigningInfo& signingInfo, time::milliseconds freshnessPeriod,
           size_t maxSegmentSize, bool isVerbose = false, bool needToPrintVersion = false,
           std::istream& is = std::cin);

  /**
   * @brief Run the Producer
   */
  void
  run();

private:
  void
  onInterest(const Interest& interest);

  /**
   * @brief Split the input stream in data packets and save them to the store
   *
   * Create data packets reading all the characters from the input stream until EOF, or an
   * error occurs. Each data packet has a maximum payload size of m_maxSegmentSize value and is
   * stored inside the vector m_store. An empty data packet is created and stored if the input
   * stream is empty.
   *
   * @return Number of data packets contained in the store after the operation
   */
  void
  populateStore(std::istream& is);

  void
  onRegisterFailed(const Name& prefix, const std::string& reason);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::vector<shared_ptr<Data>> m_store;

private:
  Name m_prefix;
  Name m_versionedPrefix;
  Face& m_face;
  KeyChain& m_keyChain;
  security::SigningInfo m_signingInfo;
  time::milliseconds m_freshnessPeriod;
  size_t m_maxSegmentSize;
  bool m_isVerbose;
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_PUTCHUNKS_PRODUCER_HPP
