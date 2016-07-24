/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California.
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
/**
 * Copyright (c) 2011-2014, Regents of the University of California,
 *
 * This file is part of ndndump, the packet capture and analysis tool for Named Data
 * Networking (NDN).
 *
 * ndndump is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndndump is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndndump, e.g., in COPYING file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndndump.hpp"
#include "core/version.hpp"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace po = boost::program_options;

namespace boost {

void
validate(boost::any& v,
         const std::vector<std::string>& values,
         boost::regex*, int)
{
  po::validators::check_first_occurrence(v);
  const std::string& str = po::validators::get_single_string(values);
  v = boost::any(boost::regex(str));
}

} // namespace boost

namespace ndn {
namespace dump {

void
usage(std::ostream& os, const std::string& appName, const po::options_description& options)
{
  os << "Usage:\n"
     << "  " << appName << " [-i interface] [-f name-filter] [tcpdump-expression] \n"
     << "\n"
     << "Default tcpdump-expression:\n"
     << "  '(ether proto 0x8624) || (tcp port 6363) || (udp port 6363)'\n"
     << "\n";
  os << options;
}

int
main(int argc, char* argv[])
{
  Ndndump instance;

  po::options_description visibleOptions;
  visibleOptions.add_options()
    ("help,h", "Produce this help message")
    ("version,V", "display version and exit")
    ("interface,i", po::value<std::string>(&instance.interface),
     "Interface from which to dump packets")
    ("read,r", po::value<std::string>(&instance.inputFile),
     "Read  packets  from file")
    ("verbose,v",
     "When  parsing  and  printing, produce verbose output")
    // ("write,w", po::value<std::string>(&instance.outputFile),
    //  "Write the raw packets to file rather than parsing and printing them out")
    ("filter,f", po::value<boost::regex>(&instance.nameFilter),
     "Regular expression to filter out Interest and Data packets")
    ;

  po::options_description hiddenOptions;
  hiddenOptions.add_options()
    ("pcap-program", po::value<std::vector<std::string>>());

  po::positional_options_description positionalArguments;
  positionalArguments.add("pcap-program", -1);

  po::options_description allOptions;
  allOptions.add(visibleOptions)
            .add(hiddenOptions);

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
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, argv[0], visibleOptions);
    return 1;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, argv[0], visibleOptions);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndndump " << tools::VERSION << '\n';
    return 0;
  }

  if (vm.count("verbose") > 0) {
    instance.isVerbose = true;
  }

  if (vm.count("pcap-program") > 0) {
    const auto& items = vm["pcap-program"].as<std::vector<std::string>>();

    std::ostringstream os;
    std::copy(items.begin(), items.end(), std::ostream_iterator<std::string>(os, " "));
    instance.pcapProgram = os.str();
  }

  if (vm.count("read") > 0 && vm.count("interface") > 0) {
    std::cerr << "ERROR: Conflicting -r and -i options\n";
    usage(std::cerr, argv[0], visibleOptions);
    return 2;
  }

  try {
    instance.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
  }

  return 0;
}

} // namespace dump
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::dump::main(argc, argv);
}
