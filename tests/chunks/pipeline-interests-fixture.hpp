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
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 */

#ifndef NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP
#define NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP

#include "tools/chunks/catchunks/pipeline-interests.hpp"

#include "tests/test-common.hpp"
#include "tests/io-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace ndn::chunks::tests {

using namespace ndn::tests;

class PipelineInterestsFixture : public IoFixture
{
protected:
  void
  setPipeline(std::unique_ptr<PipelineInterests> pline)
  {
    m_pipeline = std::move(pline);
  }

  std::shared_ptr<Data>
  makeDataWithSegment(uint64_t segmentNo, bool setFinalBlockId = true) const
  {
    auto data = std::make_shared<Data>(Name(name).appendVersion(0).appendSegment(segmentNo));
    if (setFinalBlockId)
      data->setFinalBlock(name::Component::fromSegment(nDataSegments - 1));
    return signData(data);
  }

  std::shared_ptr<Data>
  makeDataWithSegmentAndCongMark(uint64_t segmentNo,
                                 uint64_t congestionMark = 1,
                                 bool setFinalBlockId = true) const
  {
    auto data = makeDataWithSegment(segmentNo, setFinalBlockId);
    data->setCongestionMark(congestionMark);
    return data;
  }

  void
  run(const Name& name, uint64_t version = 0)
  {
    m_pipeline->run(Name(name).appendVersion(version),
                [] (const Data&) {},
                [this] (const std::string&) { hasFailed = true; });
  }

protected:
  DummyClientFace face{m_io};
  Name name{"/ndn/chunks/test"};
  uint64_t nDataSegments = 0;
  bool hasFailed = false;

private:
  std::unique_ptr<PipelineInterests> m_pipeline;
};

} // namespace ndn::chunks::tests

#endif // NDN_TOOLS_TESTS_CHUNKS_PIPELINE_INTERESTS_FIXTURE_HPP
