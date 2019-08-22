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
#include "options.hpp"
#include "pipeline-interests-aimd.hpp"
#include "pipeline-interests-cubic.hpp"
#include "pipeline-interests-fixed.hpp"
#include "statistics-collector.hpp"
#include "core/version.hpp"

#include <fstream>
#include <ndn-cxx/security/validator-null.hpp>

namespace ndn {
namespace chunks {

static int
main(int argc, char* argv[])
{
  std::string programName(argv[0]);
  Options options;
  std::string pipelineType("cubic");
  size_t maxPipelineSize(1);
  std::string uri;

  // congestion control parameters
  bool disableCwa(false), resetCwndToInit(false),
       ignoreCongMarks(false), enableFastConv(false);
  int initCwnd(1), initSsthresh(std::numeric_limits<int>::max()), k(8);
  double aiStep(1.0), rtoAlpha(0.125), rtoBeta(0.25), aimdBeta(0.5), cubicBeta(0.7);
  time::milliseconds::rep minRto(200), maxRto(60000);
  std::string cwndPath, rttPath;

  namespace po = boost::program_options;
  po::options_description basicDesc("Basic Options");
  basicDesc.add_options()
    ("help,h",      "print this help message and exit")
    ("pipeline-type,p", po::value<std::string>(&pipelineType)->default_value(pipelineType),
                        "type of Interest pipeline to use; valid values are: 'fixed', 'aimd', 'cubic'")
    ("fresh,f",     po::bool_switch(&options.mustBeFresh), "only return fresh content")
    ("lifetime,l",  po::value<time::milliseconds::rep>()->default_value(options.interestLifetime.count()),
                    "lifetime of expressed Interests, in milliseconds")
    ("retries,r",   po::value<int>(&options.maxRetriesOnTimeoutOrNack)->default_value(options.maxRetriesOnTimeoutOrNack),
                    "maximum number of retries in case of Nack or timeout (-1 = no limit)")
    ("quiet,q",     po::bool_switch(&options.isQuiet), "suppress all diagnostic output, except fatal errors")
    ("verbose,v",   po::bool_switch(&options.isVerbose), "turn on verbose output (per segment information")
    ("version,V",   "print program version and exit")
    ;

  po::options_description fixedPipeDesc("Fixed pipeline options");
  fixedPipeDesc.add_options()
    ("pipeline-size,s", po::value<size_t>(&maxPipelineSize)->default_value(maxPipelineSize),
                        "size of the Interest pipeline")
    ;

  po::options_description adaptivePipeDesc("Adaptive pipeline options (AIMD & CUBIC)");
  adaptivePipeDesc.add_options()
    ("ignore-marks", po::bool_switch(&ignoreCongMarks),
                     "do not decrease the window after receiving a congestion mark")
    ("disable-cwa",  po::bool_switch(&disableCwa),
                     "disable Conservative Window Adaptation, i.e., reduce the window on "
                     "each timeout or congestion mark instead of at most once per RTT")
    ("reset-cwnd-to-init", po::bool_switch(&resetCwndToInit),
                           "after a timeout or congestion mark, reset the window "
                           "to the initial value instead of resetting to ssthresh")
    ("init-cwnd",     po::value<int>(&initCwnd)->default_value(initCwnd),
                      "initial congestion window in segments")
    ("init-ssthresh", po::value<int>(&initSsthresh),
                      "initial slow start threshold in segments (defaults to infinity)")
    ("aimd-step", po::value<double>(&aiStep)->default_value(aiStep),
                  "additive-increase step")
    ("aimd-beta", po::value<double>(&aimdBeta)->default_value(aimdBeta),
                  "multiplicative decrease factor (AIMD)")
    ("rto-alpha", po::value<double>(&rtoAlpha)->default_value(rtoAlpha),
                  "alpha value for RTO calculation")
    ("rto-beta",  po::value<double>(&rtoBeta)->default_value(rtoBeta),
                  "beta value for RTO calculation")
    ("rto-k",     po::value<int>(&k)->default_value(k),
                  "k value for RTO calculation")
    ("min-rto",   po::value<time::milliseconds::rep>(&minRto)->default_value(minRto),
                  "minimum RTO value, in milliseconds")
    ("max-rto",   po::value<time::milliseconds::rep>(&maxRto)->default_value(maxRto),
                  "maximum RTO value, in milliseconds")
    ("log-cwnd",  po::value<std::string>(&cwndPath), "log file for congestion window stats")
    ("log-rtt",   po::value<std::string>(&rttPath), "log file for round-trip time stats")
    ;

  po::options_description cubicPipeDesc("CUBIC pipeline options");
  cubicPipeDesc.add_options()
    ("fast-conv",  po::bool_switch(&enableFastConv), "enable cubic fast convergence")
    ("cubic-beta", po::value<double>(&cubicBeta),
                   "window decrease factor for CUBIC (defaults to 0.7)")
    ;

  po::options_description visibleDesc;
  visibleDesc.add(basicDesc)
             .add(fixedPipeDesc)
             .add(adaptivePipeDesc)
             .add(cubicPipeDesc);

  po::options_description hiddenDesc;
  hiddenDesc.add_options()
    ("ndn-name,n", po::value<std::string>(&uri), "NDN name of the requested content");

  po::options_description optDesc;
  optDesc.add(visibleDesc).add(hiddenDesc);

  po::positional_options_description p;
  p.add("ndn-name", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(p).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
  catch (const boost::bad_any_cast& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  if (vm.count("help") > 0) {
    std::cout << "Usage: " << programName << " [options] ndn:/name" << std::endl;
    std::cout << visibleDesc;
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndncatchunks " << tools::VERSION << std::endl;
    return 0;
  }

  if (vm.count("ndn-name") == 0) {
    std::cerr << "Usage: " << programName << " [options] ndn:/name" << std::endl;
    std::cerr << visibleDesc;
    return 2;
  }

  if (maxPipelineSize < 1 || maxPipelineSize > 1024) {
    std::cerr << "ERROR: pipeline size must be between 1 and 1024" << std::endl;
    return 2;
  }

  if (options.maxRetriesOnTimeoutOrNack < -1 || options.maxRetriesOnTimeoutOrNack > 1024) {
    std::cerr << "ERROR: retries value must be between -1 and 1024" << std::endl;
    return 2;
  }

  options.interestLifetime = time::milliseconds(vm["lifetime"].as<time::milliseconds::rep>());
  if (options.interestLifetime < 0_ms) {
    std::cerr << "ERROR: lifetime cannot be negative" << std::endl;
    return 2;
  }

  if (options.isQuiet && options.isVerbose) {
    std::cerr << "ERROR: cannot be quiet and verbose at the same time" << std::endl;
    return 2;
  }

  try {
    Face face;
    auto discover = make_unique<DiscoverVersion>(Name(uri), face, options);
    unique_ptr<PipelineInterests> pipeline;
    unique_ptr<StatisticsCollector> statsCollector;
    unique_ptr<RttEstimatorWithStats> rttEstimator;
    std::ofstream statsFileCwnd;
    std::ofstream statsFileRtt;

    if (pipelineType == "fixed") {
      PipelineInterestsFixed::Options optionsPipeline(options);
      optionsPipeline.maxPipelineSize = maxPipelineSize;
      pipeline = make_unique<PipelineInterestsFixed>(face, optionsPipeline);
    }
    else if (pipelineType == "aimd" || pipelineType == "cubic") {
      auto optionsRttEst = make_shared<RttEstimatorWithStats::Options>();
      optionsRttEst->alpha = rtoAlpha;
      optionsRttEst->beta = rtoBeta;
      optionsRttEst->k = k;
      optionsRttEst->initialRto = 1_s;
      optionsRttEst->minRto = time::milliseconds(minRto);
      optionsRttEst->maxRto = time::milliseconds(maxRto);
      optionsRttEst->rtoBackoffMultiplier = 2;
      if (options.isVerbose) {
        using namespace ndn::time;
        std::cerr << "RTT estimator parameters:\n"
                  << "\tAlpha = " << optionsRttEst->alpha << "\n"
                  << "\tBeta = " << optionsRttEst->beta << "\n"
                  << "\tK = " << optionsRttEst->k << "\n"
                  << "\tInitial RTO = " << duration_cast<milliseconds>(optionsRttEst->initialRto) << "\n"
                  << "\tMin RTO = " << duration_cast<milliseconds>(optionsRttEst->minRto) << "\n"
                  << "\tMax RTO = " << duration_cast<milliseconds>(optionsRttEst->maxRto) << "\n"
                  << "\tBackoff multiplier = " << optionsRttEst->rtoBackoffMultiplier << "\n";
      }
      rttEstimator = make_unique<RttEstimatorWithStats>(std::move(optionsRttEst));

      PipelineInterestsAdaptive::Options optionsPipeline(options);
      optionsPipeline.disableCwa = disableCwa;
      optionsPipeline.resetCwndToInit = resetCwndToInit;
      optionsPipeline.initCwnd = initCwnd;
      optionsPipeline.initSsthresh = initSsthresh;
      optionsPipeline.aiStep = aiStep;
      optionsPipeline.mdCoef = aimdBeta;
      optionsPipeline.ignoreCongMarks = ignoreCongMarks;

      unique_ptr<PipelineInterestsAdaptive> adaptivePipeline;
      if (pipelineType == "aimd") {
        adaptivePipeline = make_unique<PipelineInterestsAimd>(face, *rttEstimator, optionsPipeline);
      }
      else {
        PipelineInterestsCubic::Options optionsCubic(optionsPipeline);
        optionsCubic.enableFastConv = enableFastConv;
        optionsCubic.cubicBeta = cubicBeta;
        adaptivePipeline = make_unique<PipelineInterestsCubic>(face, *rttEstimator, optionsCubic);
      }

      if (!cwndPath.empty() || !rttPath.empty()) {
        if (!cwndPath.empty()) {
          statsFileCwnd.open(cwndPath);
          if (statsFileCwnd.fail()) {
            std::cerr << "ERROR: failed to open " << cwndPath << std::endl;
            return 4;
          }
        }
        if (!rttPath.empty()) {
          statsFileRtt.open(rttPath);
          if (statsFileRtt.fail()) {
            std::cerr << "ERROR: failed to open " << rttPath << std::endl;
            return 4;
          }
        }
        statsCollector = make_unique<StatisticsCollector>(*adaptivePipeline, statsFileCwnd, statsFileRtt);
      }

      pipeline = std::move(adaptivePipeline);
    }
    else {
      std::cerr << "ERROR: Interest pipeline type not valid" << std::endl;
      return 2;
    }

    Consumer consumer(security::v2::getAcceptAllValidator());
    BOOST_ASSERT(discover != nullptr);
    BOOST_ASSERT(pipeline != nullptr);
    consumer.run(std::move(discover), std::move(pipeline));
    face.processEvents();
  }
  catch (const Consumer::ApplicationNackError& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 3;
  }
  catch (const Consumer::DataValidationError& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 5;
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

} // namespace chunks
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::chunks::main(argc, argv);
}
