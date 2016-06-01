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
 * @author Davide Pesavento
 * @author Weiwei Liu
 */

#include "pipeline-interests.hpp"

namespace ndn {
namespace chunks {

PipelineInterests::PipelineInterests(Face& face)
  : m_face(face)
  , m_lastSegmentNo(0)
  , m_excludedSegmentNo(0)
  , m_hasFinalBlockId(false)
  , m_isStopping(false)
{
}

PipelineInterests::~PipelineInterests() = default;

void
PipelineInterests::run(const Data& data, DataCallback onData, FailureCallback onFailure)
{
  BOOST_ASSERT(onData != nullptr);
  m_onData = std::move(onData);
  m_onFailure = std::move(onFailure);
  m_prefix = data.getName().getPrefix(-1);
  m_excludedSegmentNo = data.getName()[-1].toSegment();

  if (!data.getFinalBlockId().empty()) {
    m_lastSegmentNo = data.getFinalBlockId().toSegment();
    m_hasFinalBlockId = true;
  }

  doRun();
}

void
PipelineInterests::cancel()
{
  if (m_isStopping)
    return;

  m_isStopping = true;
  doCancel();
}

void
PipelineInterests::onFailure(const std::string& reason)
{
  if (m_isStopping)
    return;

  cancel();

  if (m_onFailure)
    m_face.getIoService().post([this, reason] { m_onFailure(reason); });
}

} // namespace chunks
} // namespace ndn
