/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2024, Arizona Board of Regents.
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

#include <boost/asio/signal_set.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>

namespace ndn::ping::server {

namespace po = boost::program_options;

class Runner : noncopyable
{
public:
  explicit
  Runner(const Options& options)
    : m_options(options)
    , m_pingServer(m_face, m_keyChain, options)
    , m_tracer(m_pingServer, options)
    , m_signalSet(m_face.getIoContext(), SIGINT)
  {
    m_pingServer.afterFinish.connect([this] {
      m_pingServer.stop();
      m_signalSet.cancel();
    });

    m_signalSet.async_wait([this] (const auto& ec, auto) {
      if (ec != boost::asio::error::operation_aborted) {
        m_pingServer.stop();
      }
    });
  }

  int
  run()
  {
    std::cout << "PING SERVER " << m_options.prefix << std::endl;

    try {
      m_pingServer.start();
      m_face.processEvents();
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
    }

    std::cout << "\n--- " << m_options.prefix << " ping server statistics ---\n"
              << m_pingServer.getNPings() << " packets processed" << std::endl;
    return 0;
  }

private:
  const Options& m_options;

  Face m_face;
  KeyChain m_keyChain;
  PingServer m_pingServer;
  Tracer m_tracer;

  boost::asio::signal_set m_signalSet;
};

static void
usage(std::ostream& os, std::string_view programName, const po::options_description& options)
{
  os << "Usage: " << programName << " [options] <prefix>\n"
     << "\n"
     << "Starts a NDN ping server that responds to Interests under name ndn:<prefix>/ping\n"
     << "\n"
     << options;
}

static int
main(int argc, char* argv[])
{
  Options options;
  std::string prefix;
  auto nMaxPings = static_cast<std::make_signed_t<size_t>>(options.nMaxPings);
  auto payloadSize = static_cast<std::make_signed_t<size_t>>(options.payloadSize);

  po::options_description visibleDesc("Options");
  visibleDesc.add_options()
    ("help,h",      "print this help message and exit")
    ("freshness,f", po::value<time::milliseconds::rep>()->default_value(options.freshnessPeriod.count()),
                    "FreshnessPeriod of the ping response, in milliseconds")
    ("satisfy,p",   po::value(&nMaxPings)->default_value(nMaxPings),
                    "maximum number of pings to satisfy (0 = no limit)")
    ("size,s",      po::value(&payloadSize)->default_value(payloadSize),
                    "size of response payload")
    ("timestamp,t", po::bool_switch(&options.wantTimestamp),
                    "prepend a timestamp to each log message")
    ("quiet,q",     po::bool_switch(&options.wantQuiet),
                    "do not print a log message each time a ping packet is received")
    ("version,V",   "print program version and exit")
    ;

  po::options_description hiddenDesc;
  hiddenDesc.add_options()
    ("prefix", po::value<std::string>(&prefix));

  po::options_description optDesc;
  optDesc.add(visibleDesc).add(hiddenDesc);

  po::positional_options_description posDesc;
  posDesc.add("prefix", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(posDesc).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, argv[0], visibleDesc);
    return 2;
  }
  catch (const boost::bad_any_cast& e) {
    std::cerr << "ERROR: " << e.what() << "\n\n";
    usage(std::cerr, argv[0], visibleDesc);
    return 2;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, argv[0], visibleDesc);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndnpingserver " << tools::VERSION << std::endl;
    return 0;
  }

  if (prefix.empty()) {
    std::cerr << "ERROR: no name prefix specified\n\n";
    usage(std::cerr, argv[0], visibleDesc);
    return 2;
  }
  options.prefix = prefix;

  options.freshnessPeriod = time::milliseconds(vm["freshness"].as<time::milliseconds::rep>());
  if (options.freshnessPeriod < 0_ms) {
    std::cerr << "ERROR: FreshnessPeriod cannot be negative" << std::endl;
    return 2;
  }

  if (nMaxPings < 0) {
    std::cerr << "ERROR: maximum number of pings to satisfy cannot be negative" << std::endl;
    return 2;
  }
  options.nMaxPings = static_cast<size_t>(nMaxPings);

  if (payloadSize < 0) {
    std::cerr << "ERROR: payload size cannot be negative" << std::endl;
    return 2;
  }
  options.payloadSize = static_cast<size_t>(payloadSize);

  return Runner(options).run();
}

} // namespace ndn::ping::server

int
main(int argc, char* argv[])
{
  return ndn::ping::server::main(argc, argv);
}
