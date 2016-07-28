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

#include "aimd-rtt-estimator.hpp"
#include <cmath>

namespace ndn {
namespace chunks {
namespace aimd {

RttEstimator::RttEstimator(const Options& options)
  : m_options(options)
  , m_sRtt(std::numeric_limits<double>::quiet_NaN())
  , m_rttVar(std::numeric_limits<double>::quiet_NaN())
  , m_rto(m_options.initialRto.count())
{
  if (m_options.isVerbose) {
    std::cerr << m_options;
  }
}

void
RttEstimator::addMeasurement(uint64_t segNo, Milliseconds rtt, size_t nExpectedSamples)
{
  BOOST_ASSERT(nExpectedSamples > 0);

  if (std::isnan(m_sRtt.count())) { // first measurement
    m_sRtt = rtt;
    m_rttVar = m_sRtt / 2;
    m_rto = m_sRtt + m_options.k * m_rttVar;
  }
  else {
    double alpha = m_options.alpha / nExpectedSamples;
    double beta = m_options.beta / nExpectedSamples;
    m_rttVar = (1 - beta) * m_rttVar + beta * time::abs(m_sRtt - rtt);
    m_sRtt = (1 - alpha) * m_sRtt + alpha * rtt;
    m_rto = m_sRtt + m_options.k * m_rttVar;
  }

  m_rto = ndn::clamp(m_rto, m_options.minRto, m_options.maxRto);

  afterRttMeasurement({segNo, rtt, m_sRtt, m_rttVar, m_rto});
}

void
RttEstimator::backoffRto()
{
  m_rto = ndn::clamp(m_rto * m_options.rtoBackoffMultiplier,
                     m_options.minRto, m_options.maxRto);
}

std::ostream&
operator<<(std::ostream& os, const RttEstimator::Options& options)
{
  os << "RttEstimator initial parameters:\n"
     << "\tAlpha = " << options.alpha << "\n"
     << "\tBeta = " << options.beta << "\n"
     << "\tK = " << options.k << "\n"
     << "\tInitial RTO = " << options.initialRto << "\n"
     << "\tMin RTO = " << options.minRto << "\n"
     << "\tMax RTO = " << options.maxRto << "\n";
  return os;
}

} // namespace aimd
} // namespace chunks
} // namespace ndn
