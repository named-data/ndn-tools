/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2019, Regents of the University of California,
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

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_CUBIC_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_CUBIC_HPP

#include "pipeline-interests-adaptive.hpp"

namespace ndn {
namespace chunks {

class PipelineInterestsCubicOptions : public PipelineInterestsAdaptiveOptions
{
public:
  explicit
  PipelineInterestsCubicOptions(const PipelineInterestsAdaptiveOptions& options =
                                PipelineInterestsAdaptiveOptions())
    : PipelineInterestsAdaptiveOptions(options)
  {
  }

public:
  bool enableFastConv = false; ///< use cubic fast convergence
  double cubicBeta = 0.7; ///< multiplicative decrease factor (from Linux kernel: 717/1024)
};

std::ostream&
operator<<(std::ostream& os, const PipelineInterestsCubicOptions& options);

/**
 * @brief Implements Cubic window increase and decrease.
 *
 * This implementation follows the RFC8312 https://tools.ietf.org/html/rfc8312
 * and the Linux kernel implementation https://github.com/torvalds/linux/blob/master/net/ipv4/tcp_cubic.c
 */
class PipelineInterestsCubic : public PipelineInterestsAdaptive
{
public:
  using Options = PipelineInterestsCubicOptions;

public:
  PipelineInterestsCubic(Face& face, RttEstimator& rttEstimator,
                         const Options& options = Options());

private:
  void
  increaseWindow() final;

  void
  decreaseWindow() final;

private:
  const Options m_cubicOptions;

  double m_wmax = 0.0; ///< window size before last window decrease
  double m_lastWmax = 0.0; ///< last wmax
  time::steady_clock::TimePoint m_lastDecrease; ///< time of last window decrease
};

} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_PIPELINE_INTERESTS_CUBIC_HPP
