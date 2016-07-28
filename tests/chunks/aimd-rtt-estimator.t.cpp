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
 * @author Weiwei Liu
 */

#include "tools/chunks/catchunks/aimd-rtt-estimator.hpp"

#include "tests/test-common.hpp"

namespace ndn {
namespace chunks {
namespace aimd {
namespace tests {

class RttEstimatorFixture
{
protected:
  RttEstimatorFixture()
    : options(makeRttEstimatorOptions())
    , rttEstimator(options)
  {
  }

private:
  static RttEstimator::Options
  makeRttEstimatorOptions()
  {
    RttEstimator::Options rttOptions;
    rttOptions.alpha = 0.125;
    rttOptions.beta = 0.25;
    rttOptions.k = 4;
    rttOptions.minRto = Milliseconds(200.0);
    rttOptions.maxRto = Milliseconds(4000.0);
    return rttOptions;
  }

protected:
  RttEstimator::Options options;
  RttEstimator rttEstimator;
};

BOOST_AUTO_TEST_SUITE(Chunks)
BOOST_FIXTURE_TEST_SUITE(TestAimdRttEstimator, RttEstimatorFixture)

BOOST_AUTO_TEST_CASE(MeasureRtt)
{
  BOOST_REQUIRE(std::isnan(rttEstimator.m_sRtt.count()));
  BOOST_REQUIRE(std::isnan(rttEstimator.m_rttVar.count()));
  BOOST_REQUIRE_CLOSE(rttEstimator.m_rto.count(), options.initialRto.count(), 1);

  // first measurement
  rttEstimator.addMeasurement(1, Milliseconds(100), 1);

  BOOST_CHECK_CLOSE(rttEstimator.m_sRtt.count(), 100, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rttVar.count(), 50, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 300, 1);

  rttEstimator.m_sRtt = Milliseconds(500.0);
  rttEstimator.m_rttVar = Milliseconds(100.0);
  rttEstimator.m_rto = Milliseconds(900.0);

  size_t nExpectedSamples = 1;
  rttEstimator.addMeasurement(1, Milliseconds(100), nExpectedSamples);

  BOOST_CHECK_CLOSE(rttEstimator.m_sRtt.count(), 450, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rttVar.count(), 175, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 1150, 0.1);

  // expected Samples larger than 1
  nExpectedSamples = 5;
  rttEstimator.addMeasurement(1, Milliseconds(100), nExpectedSamples);

  BOOST_CHECK_CLOSE(rttEstimator.m_sRtt.count(), 441.25, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rttVar.count(), 183.75, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 1176.25, 0.1);

  rttEstimator.m_sRtt = Milliseconds(100.0);
  rttEstimator.m_rttVar = Milliseconds(30.0);
  rttEstimator.m_rto = Milliseconds(220.0);

  // check if minRto works
  nExpectedSamples = 1;
  rttEstimator.addMeasurement(1, Milliseconds(100), nExpectedSamples);

  BOOST_CHECK_CLOSE(rttEstimator.m_sRtt.count(), 100, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rttVar.count(), 22.5, 1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 200, 1);

  rttEstimator.m_sRtt = Milliseconds(2000.0);
  rttEstimator.m_rttVar = Milliseconds(400.0);
  rttEstimator.m_rto = Milliseconds(3600.0);

  // check if maxRto works
  nExpectedSamples = 1;
  rttEstimator.addMeasurement(1, Milliseconds(100), nExpectedSamples);

  BOOST_CHECK_CLOSE(rttEstimator.m_sRtt.count(), 1762.5, 0.1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rttVar.count(), 775, 0.1);
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 4000, 0.1);
}

BOOST_AUTO_TEST_CASE(RtoBackoff)
{
  rttEstimator.m_rto = Milliseconds(500.0);
  rttEstimator.backoffRto();
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 1000, 0.1);

  // check if minRto works
  rttEstimator.m_rto = Milliseconds(10.0);
  rttEstimator.backoffRto();
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 200, 0.1);

  // check if maxRto works
  rttEstimator.m_rto = Milliseconds(3000.0);
  rttEstimator.backoffRto();
  BOOST_CHECK_CLOSE(rttEstimator.m_rto.count(), 4000, 0.1);
}

BOOST_AUTO_TEST_SUITE_END() // TestAimdRttEstimator
BOOST_AUTO_TEST_SUITE_END() // Chunks

} // namespace tests
} // namespace aimd
} // namespace chunks
} // namespace ndn
