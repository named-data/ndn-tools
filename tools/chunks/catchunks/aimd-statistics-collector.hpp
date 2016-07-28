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
 * @author Weiwei Liu
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_AIMD_STATISTICS_COLLECTOR_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_AIMD_STATISTICS_COLLECTOR_HPP

#include "pipeline-interests-aimd.hpp"
#include "aimd-rtt-estimator.hpp"

namespace ndn {
namespace chunks {
namespace aimd {

/**
 * @brief Statistics collector for AIMD pipeline
 */
class StatisticsCollector : noncopyable
{
public:
  StatisticsCollector(PipelineInterestsAimd& pipeline, RttEstimator& rttEstimator,
                      std::ostream& osCwnd, std::ostream& osRtt);

private:
  std::ostream& m_osCwnd;
  std::ostream& m_osRtt;
};

} // namespace aimd
} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_AIMD_STATISTICS_COLLECTOR_HPP
