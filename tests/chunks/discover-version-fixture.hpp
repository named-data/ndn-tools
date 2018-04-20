/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2018, Regents of the University of California,
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
 * @author Andrea Tosatto
 */

#ifndef NDN_TOOLS_TESTS_CHUNKS_DISCOVER_VERSION_FIXTURE_HPP
#define NDN_TOOLS_TESTS_CHUNKS_DISCOVER_VERSION_FIXTURE_HPP

#include "tools/chunks/catchunks/discover-version.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace chunks {
namespace tests {

using namespace ndn::tests;

class DiscoverVersionFixture : public UnitTestTimeFixture, virtual protected Options
{
public:
  DiscoverVersionFixture(const Options& options)
    : Options(options)
    , face(io)
    , name("/ndn/chunks/test")
    , discoveredVersion(0)
    , isDiscoveryFinished(false)
  {
  }

  virtual
  ~DiscoverVersionFixture() = default;

protected:
  void
  setDiscover(unique_ptr<DiscoverVersion> disc)
  {
    discover = std::move(disc);
    discover->onDiscoverySuccess.connect(bind(&DiscoverVersionFixture::onSuccess, this, _1));
    discover->onDiscoveryFailure.connect(bind(&DiscoverVersionFixture::onFailure, this, _1));
  }

  shared_ptr<Data>
  makeDataWithVersion(uint64_t version) const
  {
    auto data = make_shared<Data>(Name(name).appendVersion(version).appendSegment(0));
    data->setFinalBlock(name::Component::fromSegment(0));
    return signData(data);
  }

  static Options
  makeOptions()
  {
    Options options;
    options.isVerbose = false;
    options.interestLifetime = time::seconds(1);
    options.maxRetriesOnTimeoutOrNack = 3;
    return options;
  }

  virtual void
  onSuccess(const Data& data)
  {
    isDiscoveryFinished = true;

    if (data.getName()[name.size()].isVersion())
      discoveredVersion = data.getName()[name.size()].toVersion();
  }

  virtual void
  onFailure(const std::string& reason)
  {
    isDiscoveryFinished = true;
  }

protected:
  boost::asio::io_service io;
  util::DummyClientFace face;
  Name name;
  unique_ptr<DiscoverVersion> discover;
  uint64_t discoveredVersion;
  bool isDiscoveryFinished;
};

} // namespace tests
} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_TESTS_CHUNKS_DISCOVER_VERSION_FIXTURE_HPP
