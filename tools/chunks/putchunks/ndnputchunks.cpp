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
#include "producer.hpp"

namespace ndn {
namespace chunks {

static int
main(int argc, char** argv)
{
  std::string programName = argv[0];
  uint64_t freshnessPeriod = 10000;
  bool printVersion = false;
  size_t maxChunkSize = MAX_NDN_PACKET_SIZE >> 1;
  std::string signingStr;
  bool isVerbose = false;
  std::string prefix;

  namespace po = boost::program_options;
  po::options_description visibleDesc("Options");
  visibleDesc.add_options()
    ("help,h",          "print this help message and exit")
    ("freshness,f",     po::value<uint64_t>(&freshnessPeriod)->default_value(freshnessPeriod),
                        "specify FreshnessPeriod, in milliseconds")
    ("print-data-version,p",  po::bool_switch(&printVersion), "print Data version to the standard output")
    ("size,s",          po::value<size_t>(&maxChunkSize)->default_value(maxChunkSize),
                        "maximum chunk size, in bytes")
    ("signing-info,S",  po::value<std::string>(&signingStr)->default_value(signingStr),
                        "set signing information")
    ("verbose,v",       po::bool_switch(&isVerbose), "turn on verbose output")
    ("version,V",       "print program version and exit")
    ;

  po::options_description hiddenDesc("Hidden options");
  hiddenDesc.add_options()
    ("ndn-name,n", po::value<std::string>(&prefix), "NDN name for the served content");

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
    std::cout << "ndnputchunks " << tools::VERSION << std::endl;
    return 0;
  }

  if (prefix.empty()) {
    std::cerr << "Usage: " << programName << " [options] ndn:/name" << std::endl;
    std::cerr << visibleDesc;
    return 2;
  }

  if (maxChunkSize < 1 || maxChunkSize > MAX_NDN_PACKET_SIZE) {
    std::cerr << "ERROR: Maximum chunk size must be between 1 and " << MAX_NDN_PACKET_SIZE << std::endl;
    return 2;
  }

  security::SigningInfo signingInfo;
  try {
    signingInfo = security::SigningInfo(signingStr);
  }
  catch (const std::invalid_argument& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  try {
    Face face;
    KeyChain keyChain;
    Producer producer(prefix, face, keyChain, signingInfo, time::milliseconds(freshnessPeriod),
                      maxChunkSize, isVerbose, printVersion, std::cin);
    producer.run();
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
