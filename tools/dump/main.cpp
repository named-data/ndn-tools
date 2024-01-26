/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2024,  Regents of the University of California.
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

#include "ndndump.hpp"
#include "core/version.hpp"

#include <ndn-cxx/util/ostream-joiner.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <sstream>

namespace ndn::dump {

namespace po = boost::program_options;

static void
usage(std::ostream& os, std::string_view progName, const po::options_description& options)
{
  os << "Usage: " << progName << " [options] [pcap-filter]\n"
     << "\n"
     << "Default pcap-filter:\n"
     << "  '" << NdnDump::getDefaultPcapFilter() << "'\n"
     << "\n"
     << options;
}

static int
main(int argc, char* argv[])
{
  NdnDump instance;
  std::string nameFilter;
  std::vector<std::string> pcapFilter;

  po::options_description visibleOptions("Options");
  visibleOptions.add_options()
    ("help,h",      "print this help message and exit")
    ("interface,i", po::value<std::string>(&instance.interface),
                    "capture packets from the specified interface; if unspecified, the first "
                    "non-loopback interface will be used; on Linux, the special value \"any\" "
                    "can be used to capture from all interfaces")
    ("read,r",      po::value<std::string>(&instance.inputFile),
                    "read packets from the specified file; use \"-\" to read from standard input")
    ("filter,f",    po::value<std::string>(&nameFilter),
                    "print packet only if name matches this regular expression")
    ("no-promiscuous-mode,p", po::bool_switch(), "do not put the interface into promiscuous mode")
    ("no-timestamp,t",        po::bool_switch(), "do not print a timestamp for each packet")
    ("verbose,v",   po::bool_switch(&instance.wantVerbose),
                    "print more detailed information about each packet")
    ("version,V",   "print program version and exit")
    ;

  po::options_description hiddenOptions;
  hiddenOptions.add_options()
    ("pcap-filter", po::value<std::vector<std::string>>(&pcapFilter))
    ;

  po::positional_options_description posOptions;
  posOptions.add("pcap-filter", -1);

  po::options_description allOptions;
  allOptions.add(visibleOptions).add(hiddenOptions);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(allOptions).positional(posOptions).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, argv[0], visibleOptions);
    return 2;
  }
  catch (const boost::bad_any_cast& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, argv[0], visibleOptions);
    return 2;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, argv[0], visibleOptions);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndndump " << tools::VERSION << "\n"
              << pcap_lib_version() << std::endl;
    return 0;
  }

  if (vm.count("interface") > 0 && vm.count("read") > 0) {
    std::cerr << "ERROR: conflicting '--interface' and '--read' options specified\n\n";
    usage(std::cerr, argv[0], visibleOptions);
    return 2;
  }

  if (vm.count("filter") > 0) {
    try {
      instance.nameFilter = std::regex(nameFilter);
    }
    catch (const std::regex_error& e) {
      std::cerr << "ERROR: invalid filter regex: " << e.what() << std::endl;
      return 2;
    }
  }

  if (vm.count("pcap-filter") > 0) {
    std::ostringstream os;
    std::copy(pcapFilter.begin(), pcapFilter.end(), make_ostream_joiner(os, " "));
    instance.pcapFilter = os.str();
  }

  instance.wantPromisc = !vm["no-promiscuous-mode"].as<bool>();
  instance.wantTimestamp = !vm["no-timestamp"].as<bool>();

  try {
    instance.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

} // namespace ndn::dump

int
main(int argc, char* argv[])
{
  return ndn::dump::main(argc, argv);
}
