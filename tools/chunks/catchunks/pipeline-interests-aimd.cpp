/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2022, Regents of the University of California,
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
 * @author Shuo Yang
 * @author Weiwei Liu
 * @author Chavoosh Ghasemi
 * @author Klaus Schneider
 */

#include "pipeline-interests-aimd.hpp"

#include <cmath>

namespace ndn::chunks {

PipelineInterestsAimd::PipelineInterestsAimd(Face& face, RttEstimatorWithStats& rttEstimator,
                                             const Options& opts)
  : PipelineInterestsAdaptive(face, rttEstimator, opts)
{
  if (m_options.isVerbose) {
    printOptions();
  }
}

void
PipelineInterestsAimd::increaseWindow()
{
  if (m_cwnd < m_ssthresh) {
    m_cwnd += m_options.aiStep; // additive increase
  }
  else {
    m_cwnd += m_options.aiStep / std::floor(m_cwnd); // congestion avoidance
  }

  emitSignal(afterCwndChange, time::steady_clock::now() - getStartTime(), m_cwnd);
}

void
PipelineInterestsAimd::decreaseWindow()
{
  // please refer to RFC 5681, Section 3.1 for the rationale behind it
  m_ssthresh = std::max(MIN_SSTHRESH, m_cwnd * m_options.mdCoef); // multiplicative decrease
  m_cwnd = m_options.resetCwndToInit ? m_options.initCwnd : m_ssthresh;

  emitSignal(afterCwndChange, time::steady_clock::now() - getStartTime(), m_cwnd);
}

} // namespace ndn::chunks
