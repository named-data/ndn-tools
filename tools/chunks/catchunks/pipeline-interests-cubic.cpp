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
 * @author Klaus Schneider
 */

#include "pipeline-interests-cubic.hpp"

#include <cmath>
#include <iostream>

namespace ndn::chunks {

constexpr double CUBIC_C = 0.4;

PipelineInterestsCubic::PipelineInterestsCubic(Face& face, RttEstimatorWithStats& rttEstimator,
                                               const Options& opts)
  : PipelineInterestsAdaptive(face, rttEstimator, opts)
  , m_lastDecrease(time::steady_clock::now())
{
  if (m_options.isVerbose) {
    printOptions();
    std::cerr << "\tCubic beta = " << m_options.cubicBeta << "\n"
              << "\tFast convergence = " << (m_options.enableFastConv ? "yes" : "no") << "\n";
  }
}

void
PipelineInterestsCubic::increaseWindow()
{
  // Slow start phase
  if (m_cwnd < m_ssthresh) {
    m_cwnd += 1.0;
  }
  // Congestion avoidance phase
  else {
    // If wmax is still 0, set it to the current cwnd. Usually unnecessary,
    // if m_ssthresh is large enough.
    if (m_wmax < m_options.initCwnd) {
      m_wmax = m_cwnd;
    }

    // 1. Time since last congestion event in seconds
    const double t = (time::steady_clock::now() - m_lastDecrease).count() / 1e9;

    // 2. Time it takes to increase the window to m_wmax = the cwnd right before the last
    // window decrease.
    // K = cubic_root(wmax*(1-beta_cubic)/C) (Eq. 2)
    const double k = std::cbrt(m_wmax * (1 - m_options.cubicBeta) / CUBIC_C);

    // 3. Target: W_cubic(t) = C*(t-K)^3 + wmax (Eq. 1)
    const double wCubic = CUBIC_C * std::pow(t - k, 3) + m_wmax;

    // 4. Estimate of Reno Increase (Eq. 4)
    const double rtt = m_rttEstimator.getSmoothedRtt().count() / 1e9;
    const double wEst = m_wmax * m_options.cubicBeta +
                        (3 * (1 - m_options.cubicBeta) / (1 + m_options.cubicBeta)) * (t / rtt);

    // Actual adaptation
    double cubicIncrement = std::max(wCubic, wEst) - m_cwnd;
    // Cubic increment must be positive
    // Note: This change is not part of the RFC, but I added it to improve performance.
    cubicIncrement = std::max(0.0, cubicIncrement);

    m_cwnd += cubicIncrement / m_cwnd;
  }

  emitSignal(afterCwndChange, time::steady_clock::now() - getStartTime(), m_cwnd);
}

void
PipelineInterestsCubic::decreaseWindow()
{
  // A flow remembers the last value of wmax,
  // before it updates wmax for the current congestion event.

  // Current wmax < last_wmax
  if (m_options.enableFastConv && m_cwnd < m_lastWmax) {
    m_lastWmax = m_cwnd;
    m_wmax = m_cwnd * (1.0 + m_options.cubicBeta) / 2.0;
  }
  else {
    // Save old cwnd as wmax
    m_lastWmax = m_cwnd;
    m_wmax = m_cwnd;
  }

  m_ssthresh = std::max(m_options.initCwnd, m_cwnd * m_options.cubicBeta);
  m_cwnd = m_ssthresh;
  m_lastDecrease = time::steady_clock::now();

  emitSignal(afterCwndChange, time::steady_clock::now() - getStartTime(), m_cwnd);
}

} // namespace ndn::chunks
