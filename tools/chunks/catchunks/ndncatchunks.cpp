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
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 */

#include "core/version.hpp"
#include "options.hpp"
#include "consumer.hpp"
#include "discover-version-fixed.hpp"
#include "discover-version-iterative.hpp"

#include <ndn-cxx/security/validator-null.hpp>

namespace ndn {
namespace chunks {

static int
main(int argc, char** argv)
{
  std::string programName(argv[0]);
  Options options;
  std::string discoverType("fixed");
  size_t maxPipelineSize(1);
  int maxRetriesAfterVersionFound(1);
  std::string uri;

  namespace po = boost::program_options;
  po::options_description visibleDesc("Options");
  visibleDesc.add_options()
    ("help,h",      "print this help message and exit")
    ("discover-version,d",  po::value<std::string>(&discoverType)->default_value(discoverType),
                            "version discovery algorithm to use; valid values are: 'fixed', 'iterative'")
    ("fresh,f",     po::bool_switch(&options.mustBeFresh), "only return fresh content")
    ("lifetime,l",  po::value<uint64_t>()->default_value(options.interestLifetime.count()),
                    "lifetime of expressed Interests, in milliseconds")
    ("pipeline,p",  po::value<size_t>(&maxPipelineSize)->default_value(maxPipelineSize),
                    "maximum size of the Interest pipeline")
    ("retries,r",   po::value<int>(&options.maxRetriesOnTimeoutOrNack)->default_value(options.maxRetriesOnTimeoutOrNack),
                    "maximum number of retries in case of Nack or timeout (-1 = no limit)")
    ("retries-iterative,i", po::value<int>(&maxRetriesAfterVersionFound)->default_value(maxRetriesAfterVersionFound),
                            "number of timeouts that have to occur in order to confirm a discovered Data "
                            "version as the latest one (only applicable to 'iterative' version discovery)")
    ("verbose,v",   po::bool_switch(&options.isVerbose), "turn on verbose output")
    ("version,V",   "print program version and exit")
    ;

  po::options_description hiddenDesc("Hidden options");
  hiddenDesc.add_options()
    ("ndn-name,n", po::value<std::string>(&uri), "NDN name of the requested content");

  po::positional_options_description p;
  p.add("ndn-name", -1);

  po::options_description optDesc("Allowed options");
  optDesc.add(visibleDesc).add(hiddenDesc);

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

  Name prefix(uri);
  if (discoverType == "fixed" && (prefix.empty() || !prefix[-1].isVersion())) {
    std::cerr << "ERROR: The specified name must contain a version component when using "
                 "fixed version discovery" << std::endl;
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

  if (maxRetriesAfterVersionFound < 0 || maxRetriesAfterVersionFound > 1024) {
    std::cerr << "ERROR: retries iterative value must be between 0 and 1024" << std::endl;
    return 2;
  }

  options.interestLifetime = time::milliseconds(vm["lifetime"].as<uint64_t>());

  try {
    Face face;

    unique_ptr<DiscoverVersion> discover;
    if (discoverType == "fixed") {
      discover = make_unique<DiscoverVersionFixed>(prefix, face, options);
    }
    else if (discoverType == "iterative") {
      DiscoverVersionIterative::Options optionsIterative(options);
      optionsIterative.maxRetriesAfterVersionFound = maxRetriesAfterVersionFound;
      discover = make_unique<DiscoverVersionIterative>(prefix, face, optionsIterative);
    }
    else {
      std::cerr << "ERROR: discover version type not valid" << std::endl;
      return 2;
    }

    ValidatorNull validator;
    Consumer consumer(face, validator, options.isVerbose);

    PipelineInterests::Options optionsPipeline(options);
    optionsPipeline.maxPipelineSize = maxPipelineSize;
    PipelineInterests pipeline(face, optionsPipeline);

    BOOST_ASSERT(discover != nullptr);
    consumer.run(*discover, pipeline);
  }
  catch (const Consumer::ApplicationNackError& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 3;
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
main(int argc, char** argv)
{
  return ndn::chunks::main(argc, argv);
}
