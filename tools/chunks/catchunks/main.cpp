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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 * @author Davide Pesavento
 * @author Weiwei Liu
 * @author Klaus Schneider
 * @author Chavoosh Ghasemi
 */

#include "consumer.hpp"
#include "discover-version.hpp"
#include "pipeline-interests-aimd.hpp"
#include "pipeline-interests-cubic.hpp"
#include "pipeline-interests-fixed.hpp"
#include "statistics-collector.hpp"
#include "core/version.hpp"

#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/rtt-estimator.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <fstream>
#include <iostream>

namespace ndn::chunks {

namespace po = boost::program_options;

static int
main(int argc, char* argv[])
{
  const std::string programName(argv[0]);

  Options options;
  std::string prefix, nameConv, pipelineType("cubic");
  std::string cwndPath, rttPath;
  auto rttEstOptions = std::make_shared<util::RttEstimator::Options>();
  rttEstOptions->k = 8; // increased from the ndn-cxx default of 4

  po::options_description basicDesc("Basic Options");
  basicDesc.add_options()
    ("help,h",      "print this help message and exit")
    ("fresh,f",     po::bool_switch(&options.mustBeFresh),
                    "only return fresh content (set MustBeFresh on all outgoing Interests)")
    ("lifetime,l",  po::value<time::milliseconds::rep>()->default_value(options.interestLifetime.count()),
                    "lifetime of expressed Interests, in milliseconds")
    ("retries,r",   po::value<int>(&options.maxRetriesOnTimeoutOrNack)->default_value(options.maxRetriesOnTimeoutOrNack),
                    "maximum number of retries in case of Nack or timeout (-1 = no limit)")
    ("pipeline-type,p", po::value<std::string>(&pipelineType)->default_value(pipelineType),
                        "type of Interest pipeline to use; valid values are: 'fixed', 'aimd', 'cubic'")
    ("no-version-discovery,D", po::bool_switch(&options.disableVersionDiscovery),
                               "skip version discovery even if the name does not end with a version component")
    ("naming-convention,N", po::value<std::string>(&nameConv),
                            "encoding convention to use for name components, either 'marker' or 'typed'")
    ("quiet,q",     po::bool_switch(&options.isQuiet), "suppress all diagnostic output, except fatal errors")
    ("verbose,v",   po::bool_switch(&options.isVerbose), "turn on verbose output (per segment information")
    ("version,V",   "print program version and exit")
    ;

  po::options_description fixedPipeDesc("Fixed pipeline options");
  fixedPipeDesc.add_options()
    ("pipeline-size,s", po::value<size_t>(&options.maxPipelineSize)->default_value(options.maxPipelineSize),
                        "size of the Interest pipeline")
    ;

  po::options_description adaptivePipeDesc("Adaptive pipeline options (AIMD & CUBIC)");
  adaptivePipeDesc.add_options()
    ("ignore-marks",  po::bool_switch(&options.ignoreCongMarks),
                      "do not reduce the window after receiving a congestion mark")
    ("disable-cwa",   po::bool_switch(&options.disableCwa),
                      "disable Conservative Window Adaptation (reduce the window "
                      "on each congestion event instead of at most once per RTT)")
    ("init-cwnd",     po::value<double>(&options.initCwnd)->default_value(options.initCwnd),
                      "initial congestion window in segments")
    ("init-ssthresh", po::value<double>(&options.initSsthresh),
                      "initial slow start threshold in segments (defaults to infinity)")
    ("rto-alpha", po::value<double>(&rttEstOptions->alpha)->default_value(rttEstOptions->alpha),
                  "alpha value for RTO calculation")
    ("rto-beta",  po::value<double>(&rttEstOptions->beta)->default_value(rttEstOptions->beta),
                  "beta value for RTO calculation")
    ("rto-k",     po::value<int>(&rttEstOptions->k)->default_value(rttEstOptions->k),
                  "k value for RTO calculation")
    ("min-rto",   po::value<time::milliseconds::rep>()->default_value(
                    time::duration_cast<time::milliseconds>(rttEstOptions->minRto).count()),
                  "minimum RTO value, in milliseconds")
    ("max-rto",   po::value<time::milliseconds::rep>()->default_value(
                    time::duration_cast<time::milliseconds>(rttEstOptions->maxRto).count()),
                  "maximum RTO value, in milliseconds")
    ("log-cwnd",  po::value<std::string>(&cwndPath), "log file for congestion window stats")
    ("log-rtt",   po::value<std::string>(&rttPath), "log file for round-trip time stats")
    ;

  po::options_description aimdPipeDesc("AIMD pipeline options");
  aimdPipeDesc.add_options()
    ("aimd-step", po::value<double>(&options.aiStep)->default_value(options.aiStep),
                  "additive increase step")
    ("aimd-beta", po::value<double>(&options.mdCoef)->default_value(options.mdCoef),
                  "multiplicative decrease factor")
    ("reset-cwnd-to-init", po::bool_switch(&options.resetCwndToInit),
                           "after a congestion event, reset the window to the "
                           "initial value instead of resetting to ssthresh")
    ;

  po::options_description cubicPipeDesc("CUBIC pipeline options");
  cubicPipeDesc.add_options()
    ("cubic-beta", po::value<double>(&options.cubicBeta), "window decrease factor (defaults to 0.7)")
    ("fast-conv",  po::bool_switch(&options.enableFastConv), "enable fast convergence")
    ;

  po::options_description visibleDesc;
  visibleDesc.add(basicDesc)
             .add(fixedPipeDesc)
             .add(adaptivePipeDesc)
             .add(aimdPipeDesc)
             .add(cubicPipeDesc);

  po::options_description hiddenDesc;
  hiddenDesc.add_options()
    ("name", po::value<std::string>(&prefix), "NDN name of the requested content");

  po::options_description optDesc;
  optDesc.add(visibleDesc).add(hiddenDesc);

  po::positional_options_description p;
  p.add("name", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(p).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 2;
  }
  catch (const boost::bad_any_cast& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 2;
  }

  if (vm.count("help") > 0) {
    std::cout << "Usage: " << programName << " [options] ndn:/name\n";
    std::cout << visibleDesc;
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndncatchunks " << tools::VERSION << "\n";
    return 0;
  }

  if (prefix.empty()) {
    std::cerr << "Usage: " << programName << " [options] ndn:/name\n";
    std::cerr << visibleDesc;
    return 2;
  }

  if (nameConv == "marker" || nameConv == "m" || nameConv == "1") {
    name::setConventionEncoding(name::Convention::MARKER);
  }
  else if (nameConv == "typed" || nameConv == "t" || nameConv == "2") {
    name::setConventionEncoding(name::Convention::TYPED);
  }
  else if (!nameConv.empty()) {
    std::cerr << "ERROR: '" << nameConv << "' is not a valid naming convention\n";
    return 2;
  }

  options.interestLifetime = time::milliseconds(vm["lifetime"].as<time::milliseconds::rep>());
  if (options.interestLifetime < 0_ms) {
    std::cerr << "ERROR: --lifetime cannot be negative\n";
    return 2;
  }

  if (options.maxRetriesOnTimeoutOrNack < -1 || options.maxRetriesOnTimeoutOrNack > 1024) {
    std::cerr << "ERROR: --retries must be between -1 and 1024\n";
    return 2;
  }

  if (options.isQuiet && options.isVerbose) {
    std::cerr << "ERROR: cannot be quiet and verbose at the same time\n";
    return 2;
  }

  if (options.maxPipelineSize < 1 || options.maxPipelineSize > 1024) {
    std::cerr << "ERROR: --pipeline-size must be between 1 and 1024\n";
    return 2;
  }

  if (rttEstOptions->k < 0) {
    std::cerr << "ERROR: --rto-k cannot be negative\n";
    return 2;
  }

  rttEstOptions->minRto = time::milliseconds(vm["min-rto"].as<time::milliseconds::rep>());
  if (rttEstOptions->minRto < 0_ms) {
    std::cerr << "ERROR: --min-rto cannot be negative\n";
    return 2;
  }

  rttEstOptions->maxRto = time::milliseconds(vm["max-rto"].as<time::milliseconds::rep>());
  if (rttEstOptions->maxRto < rttEstOptions->minRto) {
    std::cerr << "ERROR: --max-rto cannot be smaller than --min-rto\n";
    return 2;
  }

  try {
    Face face;
    auto discover = std::make_unique<DiscoverVersion>(face, Name(prefix), options);
    std::unique_ptr<PipelineInterests> pipeline;
    std::unique_ptr<StatisticsCollector> statsCollector;
    std::unique_ptr<RttEstimatorWithStats> rttEstimator;
    std::ofstream statsFileCwnd;
    std::ofstream statsFileRtt;

    if (pipelineType == "fixed") {
      pipeline = std::make_unique<PipelineInterestsFixed>(face, options);
    }
    else if (pipelineType == "aimd" || pipelineType == "cubic") {
      if (options.isVerbose) {
        using namespace ndn::time;
        std::cerr << "RTT estimator parameters:\n"
                  << "\tAlpha = " << rttEstOptions->alpha << "\n"
                  << "\tBeta = " << rttEstOptions->beta << "\n"
                  << "\tK = " << rttEstOptions->k << "\n"
                  << "\tInitial RTO = " << duration_cast<milliseconds>(rttEstOptions->initialRto) << "\n"
                  << "\tMin RTO = " << duration_cast<milliseconds>(rttEstOptions->minRto) << "\n"
                  << "\tMax RTO = " << duration_cast<milliseconds>(rttEstOptions->maxRto) << "\n"
                  << "\tBackoff multiplier = " << rttEstOptions->rtoBackoffMultiplier << "\n";
      }
      rttEstimator = std::make_unique<RttEstimatorWithStats>(std::move(rttEstOptions));

      std::unique_ptr<PipelineInterestsAdaptive> adaptivePipeline;
      if (pipelineType == "aimd") {
        adaptivePipeline = std::make_unique<PipelineInterestsAimd>(face, *rttEstimator, options);
      }
      else {
        adaptivePipeline = std::make_unique<PipelineInterestsCubic>(face, *rttEstimator, options);
      }

      if (!cwndPath.empty() || !rttPath.empty()) {
        if (!cwndPath.empty()) {
          statsFileCwnd.open(cwndPath);
          if (statsFileCwnd.fail()) {
            std::cerr << "ERROR: failed to open '" << cwndPath << "'\n";
            return 4;
          }
        }
        if (!rttPath.empty()) {
          statsFileRtt.open(rttPath);
          if (statsFileRtt.fail()) {
            std::cerr << "ERROR: failed to open '" << rttPath << "'\n";
            return 4;
          }
        }
        statsCollector = std::make_unique<StatisticsCollector>(*adaptivePipeline, statsFileCwnd, statsFileRtt);
      }

      pipeline = std::move(adaptivePipeline);
    }
    else {
      std::cerr << "ERROR: '" << pipelineType << "' is not a valid pipeline type\n";
      return 2;
    }

    Consumer consumer(security::getAcceptAllValidator());
    BOOST_ASSERT(discover != nullptr);
    BOOST_ASSERT(pipeline != nullptr);
    consumer.run(std::move(discover), std::move(pipeline));
    face.processEvents();
  }
  catch (const Consumer::ApplicationNackError& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 3;
  }
  catch (const Consumer::DataValidationError& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 5;
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

} // namespace ndn::chunks

int
main(int argc, char* argv[])
{
  return ndn::chunks::main(argc, argv);
}
