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
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_OPTIONS_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_OPTIONS_HPP

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/time.hpp>

#include <limits>

namespace ndn::chunks {

struct Options
{
  // Common options
  time::milliseconds interestLifetime = DEFAULT_INTEREST_LIFETIME;
  int maxRetriesOnTimeoutOrNack = 15;
  bool disableVersionDiscovery = false;
  bool mustBeFresh = false;
  bool isQuiet = false;
  bool isVerbose = false;

  // Fixed pipeline options
  size_t maxPipelineSize = 1;

  // Adaptive pipeline common options
  double initCwnd = 2.0;        ///< initial congestion window size
  double initSsthresh = std::numeric_limits<double>::max(); ///< initial slow start threshold
  time::milliseconds rtoCheckInterval{10}; ///< interval for checking retransmission timer
  bool ignoreCongMarks = false; ///< disable window decrease after receiving congestion mark
  bool disableCwa = false;      ///< disable conservative window adaptation

  // AIMD pipeline options
  double aiStep = 1.0;          ///< AIMD additive increase step (in segments)
  double mdCoef = 0.5;          ///< AIMD multiplicative decrease factor
  bool resetCwndToInit = false; ///< reduce cwnd to initCwnd when loss event occurs

  // Cubic pipeline options
  double cubicBeta = 0.7;       ///< cubic multiplicative decrease factor
  bool enableFastConv = false;  ///< use cubic fast convergence
};

} // namespace ndn::chunks

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_OPTIONS_HPP
