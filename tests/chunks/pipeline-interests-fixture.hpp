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
 * @author Andrea Tosatto
 * @author Weiwei Liu
 */

#ifndef NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP
#define NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP

#include "tools/chunks/catchunks/pipeline-interests.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn {
namespace chunks {
namespace tests {

using namespace ndn::tests;

class PipelineInterestsFixture : public UnitTestTimeFixture
{
public:
  PipelineInterestsFixture()
    : face(io)
    , name("/ndn/chunks/test")
    , nDataSegments(0)
    , nReceivedSegments(0)
    , hasFailed(false)
  {
  }

protected:
  void
  setPipeline(unique_ptr<PipelineInterests> pline)
  {
    pipeline = std::move(pline);
  }

  shared_ptr<Data>
  makeDataWithSegment(uint64_t segmentNo, bool setFinalBlockId = true) const
  {
    auto data = make_shared<Data>(Name(name).appendVersion(0).appendSegment(segmentNo));
    if (setFinalBlockId)
      data->setFinalBlockId(name::Component::fromSegment(nDataSegments - 1));
    return signData(data);
  }

  void
  runWithData(const Data& data)
  {
    pipeline->run(data,
                  bind(&PipelineInterestsFixture::onData, this, _1, _2),
                  bind(&PipelineInterestsFixture::onFailure, this, _1));
  }

private:
  void
  onData(const Interest& interest, const Data& data)
  {
    nReceivedSegments++;
  }

  void
  onFailure(const std::string& reason)
  {
    hasFailed = true;
  }

protected:
  boost::asio::io_service io;
  util::DummyClientFace face;
  Name name;
  unique_ptr<PipelineInterests> pipeline;
  uint64_t nDataSegments;
  uint64_t nReceivedSegments;
  bool hasFailed;
};

} // namespace tests
} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP
