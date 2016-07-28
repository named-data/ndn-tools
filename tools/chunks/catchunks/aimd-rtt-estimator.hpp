/**
 * Copyright (c) 2016,  Arizona Board of Regents.
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
 */

#ifndef NDN_TOOLS_CHUNKS_CATCHUNKS_AIMD_RTT_ESTIMATOR_HPP
#define NDN_TOOLS_CHUNKS_CATCHUNKS_AIMD_RTT_ESTIMATOR_HPP

#include "core/common.hpp"

namespace ndn {
namespace chunks {
namespace aimd {

typedef time::duration<double, time::milliseconds::period> Milliseconds;

struct RttRtoSample
{
  uint64_t segNo;
  Milliseconds rtt; ///< measured RTT
  Milliseconds sRtt; ///< smoothed RTT
  Milliseconds rttVar; ///< RTT variation
  Milliseconds rto; ///< retransmission timeout
};

/**
 * @brief RTT Estimator.
 *
 * This class implements the "Mean--Deviation" RTT estimator, as discussed in RFC6298,
 * with the modifications to RTO calculation described in RFC 7323 Appendix G.
 */
class RttEstimator
{
public:
  class Options
  {
  public:
    Options()
      : isVerbose(false)
      , alpha(0.125)
      , beta(0.25)
      , k(4)
      , initialRto(1000.0)
      , minRto(200.0)
      , maxRto(20000.0)
      , rtoBackoffMultiplier(2)
    {
    }

  public:
    bool isVerbose;
    double alpha; ///< parameter for RTT estimation
    double beta; ///< parameter for RTT variation calculation
    int k; ///< factor of RTT variation when calculating RTO
    Milliseconds initialRto; ///< initial RTO value
    Milliseconds minRto; ///< lower bound of RTO
    Milliseconds maxRto; ///< upper bound of RTO
    int rtoBackoffMultiplier;
  };

  /**
   * @brief create a RTT Estimator
   *
   * Configures the RTT Estimator with the default parameters if an instance of Options
   * is not passed to the constructor.
   */
  explicit
  RttEstimator(const Options& options = Options());

  /**
   * @brief Add a new RTT measurement to the estimator for the given received segment.
   *
   * @note Don't take RTT measurement for retransmitted segments
   * @param segNo the segment number of the received segmented Data
   * @param rtt the sampled rtt
   * @param nExpectedSamples number of expected samples, must be greater than 0.
   *        It should be set to current number of in-flight Interests. Please
   *        refer to Appendix G of RFC 7323 for details.
   */
  void
  addMeasurement(uint64_t segNo, Milliseconds rtt, size_t nExpectedSamples);

  /**
   * @return estimated RTO
   */
  Milliseconds
  getEstimatedRto() const;

  /**
   * @brief backoff RTO by the factor of RttEstimatorOptions::rtoBackoffMultiplier
   */
  void
  backoffRto();

  /**
   * @brief Signals after rtt is measured
   */
  signal::Signal<RttEstimator, RttRtoSample> afterRttMeasurement;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  const Options m_options;
  Milliseconds m_sRtt; ///< smoothed round-trip time
  Milliseconds m_rttVar; ///< round-trip time variation
  Milliseconds m_rto; ///< retransmission timeout
};

/**
 * @brief returns the estimated RTO value
 */
inline Milliseconds
RttEstimator::getEstimatedRto() const
{
  return m_rto;
}

std::ostream&
operator<<(std::ostream& os, const RttEstimator::Options& options);

} // namespace aimd
} // namespace chunks
} // namespace ndn

#endif // NDN_TOOLS_CHUNKS_CATCHUNKS_RTT_ESTIMATOR_HPP
