/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015,  Arizona Board of Regents.
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
 * @author Eric Newberry <enewberry@email.arizona.edu>
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include "core/common.hpp"
#include "core/version.hpp"

#include "ping-server.hpp"
#include "tracer.hpp"

namespace ndn {
namespace ping {
namespace server {

static time::milliseconds
getMinimumFreshnessPeriod()
{
  return time::milliseconds(1000);
}

static void
usage(const boost::program_options::options_description& options)
{
  std::cout << "Usage: ndnpingserver [options] ndn:/name/prefix\n"
      "\n"
      "Starts a NDN ping server that responds to Interests with name"
      "ndn:/name/prefix/ping/number.\n"
      "\n";
  std::cout << options;
  exit(2);
}

static void
printStatistics(PingServer& pingServer, Options& options)
{
  std::cout << "\n--- ping server " << options.prefix << " ---" << std::endl;
  std::cout << pingServer.getNPings() << " packets processed" << std::endl;
}

/**
 * @brief SIGINT handler: exits
 * @param pingServer pingServer instance
 * @param options options
 */
static void
onSigInt(PingServer& pingServer, Options& options, Face& face)
{
  printStatistics(pingServer, options);
  face.shutdown();
  face.getIoService().stop();
  exit(1);
}

/**
 * @brief outputs errors to cerr
 */
static void
onError(std::string msg)
{
  std::cerr << "ERROR: " << msg << std::endl;
}

int
main(int argc, char* argv[])
{
  Options options;
  options.freshnessPeriod = getMinimumFreshnessPeriod();
  options.shouldLimitSatisfied = false;
  options.nMaxPings = 0;
  options.shouldPrintTimestamp = false;

  namespace po = boost::program_options;

  po::options_description visibleOptDesc("Allowed options");
  visibleOptDesc.add_options()
    ("help,h", "print this message and exit")
    ("version,V", "display version and exit")
    ("freshness,x", po::value<int>(),
                   ("set freshness period in milliseconds (minimum " +
                   std::to_string(getMinimumFreshnessPeriod().count()) + " ms)").c_str())
    ("satisfy,p", po::value<int>(&options.nMaxPings), "set maximum number of pings to be satisfied")
    ("timestamp,t", "log timestamp with responses")
  ;
  po::options_description hiddenOptDesc("Hidden options");
  hiddenOptDesc.add_options()
    ("prefix", po::value<std::string>(), "prefix to register")
  ;

  po::options_description optDesc("Allowed options");
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  try {
    po::positional_options_description optPos;
    optPos.add("prefix", -1);

    po::variables_map optVm;
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), optVm);
    po::notify(optVm);

    if (optVm.count("help") > 0) {
      usage(visibleOptDesc);
    }

    if (optVm.count("version") > 0) {
      std::cout << "ndnpingserver " << tools::VERSION << std::endl;
      exit(0);
    }

    if (optVm.count("prefix") > 0) {
      options.prefix = Name(optVm["prefix"].as<std::string>());
    }
    else {
      std::cerr << "ERROR: No prefix specified" << std::endl;
      usage(visibleOptDesc);
    }

    if (optVm.count("freshness") > 0) {
      options.freshnessPeriod = time::milliseconds(optVm["freshness"].as<int>());

      if (options.freshnessPeriod.count() < getMinimumFreshnessPeriod().count()) {
        std::cerr << "ERROR: Specified FreshnessPeriod is less than the minimum "
                  << getMinimumFreshnessPeriod() << std::endl;
        usage(visibleOptDesc);
      }
    }

    if (optVm.count("satisfy") > 0) {
      options.shouldLimitSatisfied = true;

      if (options.nMaxPings < 1) {
        std::cerr << "ERROR: Maximum number of pings to satisfy must be greater than 0" << std::endl;
        usage(visibleOptDesc);
      }
    }

    if (optVm.count("timestamp") > 0) {
      options.shouldPrintTimestamp = true;
    }
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    usage(visibleOptDesc);
  }

  boost::asio::io_service ioService;
  Face face(ioService);
  PingServer pingServer(face, options);
  Tracer tracer(pingServer, options);

  boost::asio::signal_set signalSetInt(face.getIoService(), SIGINT);
  signalSetInt.async_wait(bind(&onSigInt, ref(pingServer), ref(options), ref(face)));

  std::cout << "PING SERVER " << options.prefix << std::endl;

  try {
    pingServer.run();
  }
  catch (std::exception& e) {
    onError(e.what());
    face.getIoService().stop();
    return 1;
  }

  printStatistics(pingServer, options);
  face.shutdown();
  face.getIoService().stop();

  return 0;
}

} // namespace server
} // namespace ping
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::ping::server::main(argc, argv);
}