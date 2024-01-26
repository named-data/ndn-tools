/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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
 * @author Davide Pesavento <davidepesa@gmail.com>
 */

#include "ndnpeek.hpp"
#include "core/program-options-ext.hpp"
#include "core/version.hpp"

#include <ndn-cxx/util/io.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

namespace ndn::peek {

namespace po = boost::program_options;

static void
usage(std::ostream& os, std::string_view program, const po::options_description& options)
{
  os << "Usage: " << program << " [options] /name\n"
     << "\n"
     << "Fetch one data item matching the specified name and write it to the standard output.\n"
     << options;
}

static std::ifstream
openBinaryFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "ERROR: cannot open '" << filename << "' for reading: "
              << std::strerror(errno) << std::endl;
  }
  return file;
}

static int
main(int argc, char* argv[])
{
  std::string progName(argv[0]);
  PeekOptions options;

  po::options_description genericOptDesc("Generic options");
  genericOptDesc.add_options()
    ("help,h",    "print help and exit")
    ("payload,p", po::bool_switch(&options.wantPayloadOnly),
                  "print payload only, instead of full packet")
    ("timeout,w", po::value<time::milliseconds::rep>(), "execution timeout, in milliseconds")
    ("verbose,v", po::bool_switch(&options.isVerbose), "turn on verbose output")
    ("version,V", "print version and exit")
  ;

  po::options_description interestOptDesc("Interest construction");
  interestOptDesc.add_options()
    ("prefix,P",   po::bool_switch(&options.canBePrefix), "set CanBePrefix")
    ("fresh,f",    po::bool_switch(&options.mustBeFresh), "set MustBeFresh")
    ("fwhint,F",   po::value<std::vector<Name>>(&options.forwardingHint)->composing(),
                   "add ForwardingHint delegation name")
    ("lifetime,l", po::value<time::milliseconds::rep>()->default_value(options.interestLifetime.count()),
                   "set InterestLifetime, in milliseconds")
    ("hop-limit,H",     po::value<int>(), "set HopLimit")
    ("app-params,A",    po::value<std::string>(), "set ApplicationParameters from a base64-encoded string")
    ("app-params-file", po::value<std::string>(), "set ApplicationParameters from a file")
  ;

  po::options_description visibleOptDesc;
  visibleOptDesc.add(genericOptDesc).add(interestOptDesc);

  po::options_description hiddenOptDesc;
  hiddenOptDesc.add_options()
    ("name", po::value<Name>(&options.name), "Interest name");

  po::options_description optDesc;
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  po::positional_options_description optPos;
  optPos.add("name", -1);

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
    usage(std::cout, progName, visibleOptDesc);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndnpeek " << tools::VERSION << std::endl;
    return 0;
  }

  if (vm.count("name") == 0) {
    std::cerr << "ERROR: missing name\n\n";
    usage(std::cerr, progName, visibleOptDesc);
    return 2;
  }

  if (vm.count("timeout") > 0) {
    options.timeout = time::milliseconds(vm["timeout"].as<time::milliseconds::rep>());
    if (*options.timeout < 0_ms) {
      std::cerr << "ERROR: timeout cannot be negative" << std::endl;
      return 2;
    }
  }

  options.interestLifetime = time::milliseconds(vm["lifetime"].as<time::milliseconds::rep>());
  if (options.interestLifetime < 0_ms) {
    std::cerr << "ERROR: InterestLifetime cannot be negative" << std::endl;
    return 2;
  }

  if (vm.count("hop-limit") > 0) {
    auto hopLimit = vm["hop-limit"].as<int>();
    if (hopLimit < 0 || hopLimit > 255) {
      std::cerr << "ERROR: HopLimit must be between 0 and 255" << std::endl;
      return 2;
    }
    options.hopLimit = static_cast<uint8_t>(hopLimit);
  }

  if (vm.count("app-params") > 0) {
    if (vm.count("app-params-file") > 0) {
      std::cerr << "ERROR: cannot specify both '--app-params' and '--app-params-file'" << std::endl;
      return 2;
    }
    std::istringstream is(vm["app-params"].as<std::string>());
    try {
      options.applicationParameters = io::loadBuffer(is, io::BASE64);
    }
    catch (const io::Error& e) {
      std::cerr << "ERROR: invalid ApplicationParameters string: " << e.what() << std::endl;
      return 2;
    }
  }

  if (vm.count("app-params-file") > 0) {
    auto filename = vm["app-params-file"].as<std::string>();
    std::ifstream paramsFile = openBinaryFile(filename);
    if (!paramsFile) {
      return 2;
    }
    try {
      options.applicationParameters = io::loadBuffer(paramsFile, io::NO_ENCODING);
    }
    catch (const io::Error& e) {
      std::cerr << "ERROR: cannot read ApplicationParameters from file '" << filename
                << "': " << e.what() << std::endl;
      return 2;
    }
  }

  try {
    Face face;
    NdnPeek program(face, options);

    program.start();
    face.processEvents();

    return static_cast<int>(program.getResult());
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}

} // namespace ndn::peek

int
main(int argc, char* argv[])
{
  return ndn::peek::main(argc, argv);
}
