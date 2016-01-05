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

#include "pipeline-interests.hpp"
#include "data-fetcher.hpp"

namespace ndn {
namespace chunks {

PipelineInterests::PipelineInterests(Face& face, const Options& options)
  : m_face(face)
  , m_nextSegmentNo(0)
  , m_lastSegmentNo(0)
  , m_excludeSegmentNo(0)
  , m_options(options)
  , m_hasFinalBlockId(false)
  , m_hasError(false)
  , m_hasFailure(false)
{
  m_segmentFetchers.resize(m_options.maxPipelineSize);
}

PipelineInterests::~PipelineInterests()
{
  cancel();
}

void
PipelineInterests::runWithExcludedSegment(const Data& data, DataCallback onData,
                                          FailureCallback onFailure)
{
  BOOST_ASSERT(onData != nullptr);
  m_onData = std::move(onData);
  m_onFailure = std::move(onFailure);

  Name dataName = data.getName();
  m_prefix = dataName.getPrefix(-1);
  m_excludeSegmentNo = dataName[-1].toSegment();

  if (!data.getFinalBlockId().empty()) {
    m_hasFinalBlockId = true;
    m_lastSegmentNo = data.getFinalBlockId().toSegment();
  }

  // if the FinalBlockId is unknown, this could potentially request non-existent segments
  for (size_t nRequestedSegments = 0; nRequestedSegments < m_options.maxPipelineSize;
       nRequestedSegments++) {
    if (!fetchNextSegment(nRequestedSegments)) // all segments have been requested
      break;
  }
}

bool
PipelineInterests::fetchNextSegment(std::size_t pipeNo)
{
  if (m_hasFailure) {
    fail("Fetching terminated but no final segment number has been found");
    return false;
  }

  if (m_nextSegmentNo == m_excludeSegmentNo)
    m_nextSegmentNo++;

  if (m_hasFinalBlockId && m_nextSegmentNo > m_lastSegmentNo)
   return false;

  // Send interest for next segment
  if (m_options.isVerbose)
    std::cerr << "Requesting segment #" << m_nextSegmentNo << std::endl;

  Interest interest(Name(m_prefix).appendSegment(m_nextSegmentNo));
  interest.setInterestLifetime(m_options.interestLifetime);
  interest.setMustBeFresh(m_options.mustBeFresh);
  interest.setMaxSuffixComponents(1);

  BOOST_ASSERT(!m_segmentFetchers[pipeNo].first || !m_segmentFetchers[pipeNo].first->isRunning());

  auto fetcher = DataFetcher::fetch(m_face, interest,
                                    m_options.maxRetriesOnTimeoutOrNack,
                                    m_options.maxRetriesOnTimeoutOrNack,
                                    bind(&PipelineInterests::handleData, this, _1, _2, pipeNo),
                                    bind(&PipelineInterests::handleFail, this, _2, pipeNo),
                                    bind(&PipelineInterests::handleFail, this, _2, pipeNo),
                                    m_options.isVerbose);

  m_segmentFetchers[pipeNo] = make_pair(fetcher, m_nextSegmentNo);

  m_nextSegmentNo++;
  return true;
}

void
PipelineInterests::cancel()
{
  for (auto& fetcher : m_segmentFetchers)
    if (fetcher.first)
      fetcher.first->cancel();

  m_segmentFetchers.clear();
}

void
PipelineInterests::fail(const std::string& reason)
{
  if (!m_hasError) {
    cancel();
    m_hasError = true;
    m_hasFailure = true;
    if (m_onFailure)
      m_face.getIoService().post([this, reason] { m_onFailure(reason); });
  }
}

void
PipelineInterests::handleData(const Interest& interest, const Data& data, size_t pipeNo)
{
  if (m_hasError)
    return;

  BOOST_ASSERT(data.getName().equals(interest.getName()));

  if (m_options.isVerbose)
    std::cerr << "Received segment #" << data.getName()[-1].toSegment() << std::endl;

  m_onData(interest, data);

  if (!m_hasFinalBlockId && !data.getFinalBlockId().empty()) {
    m_lastSegmentNo = data.getFinalBlockId().toSegment();
    m_hasFinalBlockId = true;

    for (auto& fetcher : m_segmentFetchers) {
      if (fetcher.first && fetcher.second > m_lastSegmentNo) {
        // Stop trying to fetch segments that are not part of the content
        fetcher.first->cancel();
      }
      else if (fetcher.first && fetcher.first->hasError()) { // fetcher.second <= m_lastSegmentNo
        // there was an error while fetching a segment that is part of the content
        fail("Failure retriving segment #" + to_string(fetcher.second));
        return;
      }
    }
  }

  fetchNextSegment(pipeNo);
}

void PipelineInterests::handleFail(const std::string& reason, std::size_t pipeNo)
{
  if (m_hasError)
    return;

  if (m_hasFinalBlockId && m_segmentFetchers[pipeNo].second <= m_lastSegmentNo) {
    fail(reason);
  }
  else if (!m_hasFinalBlockId) {
    // don't fetch the following segments
    bool areAllFetchersStopped = true;
    for (auto& fetcher : m_segmentFetchers) {
      if (fetcher.first && fetcher.second > m_segmentFetchers[pipeNo].second) {
        fetcher.first->cancel();
      }
      else if (fetcher.first && fetcher.first->isRunning()) {
        // fetcher.second <= m_segmentFetchers[pipeNo].second
        areAllFetchersStopped = false;
      }
    }
    if (areAllFetchersStopped) {
      if (m_onFailure)
        fail("Fetching terminated but no final segment number has been found");
    }
    else {
      m_hasFailure = true;
    }
  }
}

} // namespace chunks
} // namespace ndn
