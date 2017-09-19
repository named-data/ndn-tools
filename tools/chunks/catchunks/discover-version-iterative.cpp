/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2017, Regents of the University of California,
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
 * @author Klaus Schneider
 */

#include "discover-version-iterative.hpp"

namespace ndn {
namespace chunks {

DiscoverVersionIterative::DiscoverVersionIterative(const Name& prefix, Face& face,
                                                   const Options& options)
  : chunks::Options(options)
  , DiscoverVersion(prefix, face)
  , Options(options)
  , m_latestVersion(0)
  , m_foundVersion(false)
{
}

void
DiscoverVersionIterative::run()
{
  m_latestVersion = 0;
  m_foundVersion = false;

  Interest interest(m_prefix);
  interest.setInterestLifetime(interestLifetime);
  interest.setMustBeFresh(mustBeFresh);
  interest.setMinSuffixComponents(3);
  interest.setMaxSuffixComponents(3);
  interest.setChildSelector(1);

  expressInterest(interest, maxRetriesOnTimeoutOrNack, maxRetriesOnTimeoutOrNack);
}

void
DiscoverVersionIterative::handleData(const Interest& interest, const Data& data)
{
  size_t versionindex = m_prefix.size();

  const Name& name = data.getName();
  Exclude exclude;

  if (isVerbose)
    std::cerr << "Data: " << data << std::endl;

  BOOST_ASSERT(name.size() > m_prefix.size());
  if (name[versionindex].isVersion()) {
    m_latestVersion = name[versionindex].toVersion();
    m_latestVersionData = make_shared<Data>(data);
    m_foundVersion = true;

    exclude.excludeBefore(name[versionindex]);

    if (isVerbose)
      std::cerr << "Discovered version = " << m_latestVersion << std::endl;
  }
  else {
    // didn't find a version number at expected index.
    m_strayExcludes.excludeOne(name[versionindex]);
  }

  for (const Exclude::Range& range : m_strayExcludes) {
    BOOST_ASSERT(range.isSingular());
    exclude.excludeOne(range.from);
  }

  Interest newInterest(interest);
  newInterest.refreshNonce();
  newInterest.setExclude(exclude);
  newInterest.setInterestLifetime(discoveryTimeout);

  if (m_foundVersion)
    expressInterest(newInterest, maxRetriesOnTimeoutOrNack, maxRetriesAfterVersionFound);
  else
    expressInterest(interest, maxRetriesOnTimeoutOrNack, maxRetriesOnTimeoutOrNack);
}

void
DiscoverVersionIterative::handleTimeout(const Interest& interest, const std::string& reason)
{
  if (m_foundVersion) {
    // a version has been found and after a timeout error this version can be used as the latest.
    if (isVerbose)
      std::cerr << "Found data with the latest version: " << m_latestVersion << std::endl;

    // we discovered at least one version. assume what we have is the latest.
    this->emitSignal(onDiscoverySuccess, *m_latestVersionData);
  }
  else {
    DiscoverVersion::handleTimeout(interest, reason);
  }
}

} // namespace chunks
} // namespace ndn
