/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California.
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
 */

#include "ndn-dissect.hpp"
#include "core/version.hpp"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <fstream>

namespace po = boost::program_options;

namespace ndn {
namespace dissect {

static void
usage(std::ostream& os, const std::string& appName, const po::options_description& options)
{
  os << "Usage:\n"
     << "  " << appName << " [input-file] \n"
     << "\n"
     << options;
}

static int
main(int argc, char* argv[])
{
  po::options_description visibleOptions;
  visibleOptions.add_options()
    ("help,h", "Print help and exit.")
    ("version,V", "Print version and exit.")
    ;

  std::string inputFileName;
  po::options_description hiddenOptions;
  hiddenOptions.add_options()
    ("input-file", po::value<std::string>(&inputFileName));
  ;
  po::positional_options_description positionalArguments;
  positionalArguments
    .add("input-file", -1);

  po::options_description allOptions;
  allOptions
    .add(visibleOptions)
    .add(hiddenOptions)
    ;

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv)
                .options(allOptions)
                .positional(positionalArguments)
                .run(),
              vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    usage(std::cerr, argv[0], visibleOptions);
    return 2;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, argv[0], visibleOptions);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndn-dissect " << tools::VERSION << std::endl;
    return 0;
  }

  std::ifstream inputFile;
  std::istream* inputStream;

  if (vm.count("input-file") > 0 && inputFileName != "-") {
    inputFile.open(inputFileName);
    if (!inputFile.is_open()) {
      std::cerr << argv[0] << ": " << inputFileName << ": File does not exist or is unreadable" << std::endl;
      return 3;
    }
    inputStream = &inputFile;
  }
  else {
    inputStream = &std::cin;
  }

  NdnDissect program;
  program.dissect(std::cout, *inputStream);

  return 0;
}

} // namespace dissect
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::dissect::main(argc, argv);
}
