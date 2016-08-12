/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author Zhuo Li <zhuoli@email.arizona.edu>
 */

#include "ndnpeek.hpp"
#include "core/version.hpp"

#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace peek {

namespace po = boost::program_options;

static void
usage(std::ostream& os, const po::options_description& options)
{
  os << "Usage: ndnpeek [options] ndn:/name\n"
        "\n"
        "Fetch one data item matching the name prefix and write it to standard output.\n"
        "\n"
     << options;
}

static int
main(int argc, char* argv[])
{
  PeekOptions options;
  options.isVerbose = false;
  options.mustBeFresh = false;
  options.wantRightmostChild = false;
  options.wantPayloadOnly = false;
  options.minSuffixComponents = -1;
  options.maxSuffixComponents = -1;
  options.interestLifetime = time::milliseconds(-1);
  options.timeout = time::milliseconds(-1);

  po::options_description genericOptDesc("Generic options");
  genericOptDesc.add_options()
    ("help,h", "print help and exit")
    ("payload,p", po::bool_switch(&options.wantPayloadOnly),
        "print payload only, instead of full packet")
    ("timeout,w", po::value<int>(),
        "set timeout (in milliseconds)")
    ("verbose,v", po::bool_switch(&options.isVerbose),
        "turn on verbose output")
    ("version,V", "print version and exit")
  ;

  po::options_description interestOptDesc("Interest construction");
  interestOptDesc.add_options()
    ("fresh,f", po::bool_switch(&options.mustBeFresh),
        "set MustBeFresh")
    ("rightmost,r", po::bool_switch(&options.wantRightmostChild),
        "set ChildSelector to rightmost")
    ("minsuffix,m", po::value<int>(&options.minSuffixComponents),
        "set MinSuffixComponents")
    ("maxsuffix,M", po::value<int>(&options.maxSuffixComponents),
        "set MaxSuffixComponents")
    ("lifetime,l", po::value<int>(),
        "set InterestLifetime (in milliseconds)")
    ("link-file", po::value<std::string>(),
        "set Link from a file")
  ;

  po::options_description visibleOptDesc;
  visibleOptDesc.add(genericOptDesc).add(interestOptDesc);

  po::options_description hiddenOptDesc;
  hiddenOptDesc.add_options()
    ("prefix", po::value<std::string>(), "Interest name");

  po::options_description optDesc;
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  po::positional_options_description optPos;
  optPos.add("prefix", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, visibleOptDesc);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndnpeek " << tools::VERSION << std::endl;
    return 0;
  }

  if (vm.count("prefix") > 0) {
    options.prefix = vm["prefix"].as<std::string>();
  }
  else {
    std::cerr << "ERROR: Interest name is missing" << std::endl;
    usage(std::cerr, visibleOptDesc);
    return 2;
  }

  if (vm.count("minsuffix") > 0 && options.minSuffixComponents < 0) {
    std::cerr << "ERROR: MinSuffixComponents must be a non-negative integer" << std::endl;
    usage(std::cerr, visibleOptDesc);
    return 2;
  }

  if (vm.count("maxsuffix") > 0 && options.maxSuffixComponents < 0) {
    std::cerr << "ERROR: MaxSuffixComponents must be a non-negative integer" << std::endl;
    usage(std::cerr, visibleOptDesc);
    return 2;
  }

  if (vm.count("lifetime") > 0) {
    if (vm["lifetime"].as<int>() >= 0) {
      options.interestLifetime = time::milliseconds(vm["lifetime"].as<int>());
    }
    else {
      std::cerr << "ERROR: InterestLifetime must be a non-negative integer" << std::endl;
      usage(std::cerr, visibleOptDesc);
      return 2;
    }
  }

  if (vm.count("timeout") > 0) {
    if (vm["timeout"].as<int>() > 0) {
      options.timeout = time::milliseconds(vm["timeout"].as<int>());
    }
    else {
      std::cerr << "ERROR: Timeout must be a positive integer" << std::endl;
      usage(std::cerr, visibleOptDesc);
      return 2;
    }
  }

  if (vm.count("link-file") > 0) {
    options.link = io::load<Link>(vm["link-file"].as<std::string>());
    if (options.link == nullptr) {
      std::cerr << "ERROR: Cannot read Link object from the specified file" << std::endl;
      usage(std::cerr, visibleOptDesc);
      return 2;
    }
  }

  Face face;
  NdnPeek program(face, options);

  try {
    program.start();
    face.processEvents(program.getTimeout());
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }

  ResultCode result = program.getResultCode();
  if (result == ResultCode::TIMEOUT && options.isVerbose) {
    std::cerr << "TIMEOUT" << std::endl;
  }
  return static_cast<int>(result);
}

} // namespace peek
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::peek::main(argc, argv);
}
