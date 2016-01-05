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

#include "discover-version.hpp"
#include "data-fetcher.hpp"

namespace ndn {
namespace chunks {

DiscoverVersion::DiscoverVersion(const Name& prefix, Face& face, const Options& options)
  : Options(options)
  , m_prefix(prefix)
  , m_face(face)
{
}

void
DiscoverVersion::expressInterest(const Interest& interest, int maxRetriesNack,
                                 int maxRetriesTimeout)
{
  fetcher = DataFetcher::fetch(m_face, interest, maxRetriesNack, maxRetriesTimeout,
                               bind(&DiscoverVersion::handleData, this, _1, _2),
                               bind(&DiscoverVersion::handleNack, this, _1, _2),
                               bind(&DiscoverVersion::handleTimeout, this, _1, _2),
                               isVerbose);
}

void
DiscoverVersion::handleData(const Interest& interest, const Data& data)
{
  onDiscoverySuccess(data);
}

void
DiscoverVersion::handleNack(const Interest& interest, const std::string& reason)
{
  onDiscoveryFailure(reason);
}

void
DiscoverVersion::handleTimeout(const Interest& interest, const std::string& reason)
{
  onDiscoveryFailure(reason);
}

} // namespace chunks
} // namespace ndn
