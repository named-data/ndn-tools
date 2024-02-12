/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Arizona Board of Regents.
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
 * @author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 * @author: Eric Newberry <enewberry@email.arizona.edu>
 */

#include "core/common.hpp"
#include "core/version.hpp"

#include "ping.hpp"
#include "statistics-collector.hpp"
#include "tracer.hpp"

#include <boost/asio/signal_set.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>

namespace ndn::ping::client {

class Runner : noncopyable
{
public:
  explicit
  Runner(const Options& options)
    : m_ping(m_face, options)
    , m_statisticsCollector(m_ping, options)
    , m_tracer(m_ping, options)
    , m_signalSetInt(m_face.getIoContext(), SIGINT)
    , m_signalSetQuit(m_face.getIoContext(), SIGQUIT)
  {
    m_signalSetInt.async_wait([this] (const auto& err, int) { onInterruptSignal(err); });
    m_signalSetQuit.async_wait([this] (const auto& err, int) { onQuitSignal(err); });
    m_ping.afterFinish.connect([this] { cancel(); });
  }

  int
  run()
  {
    try {
      m_ping.start();
      m_face.processEvents();
    }
    catch (const std::exception& e) {
      m_tracer.onError(e.what());
      return 2;
    }

    auto statistics = m_statisticsCollector.computeStatistics();
    statistics.printFull(std::cout);

    if (statistics.nReceived == statistics.nSent) {
      return 0;
    }
    else {
      return 1;
    }
  }

private:
  void
  cancel()
  {
    m_signalSetInt.cancel();
    m_signalSetQuit.cancel();
    m_ping.stop();
  }

  void
  onInterruptSignal(const boost::system::error_code& errorCode)
  {
    if (errorCode == boost::asio::error::operation_aborted) {
      return;
    }

    cancel();
  }

  void
  onQuitSignal(const boost::system::error_code& errorCode)
  {
    if (errorCode == boost::asio::error::operation_aborted) {
      return;
    }

    m_statisticsCollector.computeStatistics().printSummary(std::cout);

    m_signalSetQuit.async_wait([this] (const auto& err, int) { onQuitSignal(err); });
  }

private:
  Face m_face;
  Ping m_ping;
  StatisticsCollector m_statisticsCollector;
  Tracer m_tracer;

  boost::asio::signal_set m_signalSetInt;
  boost::asio::signal_set m_signalSetQuit;
};

[[noreturn]] static void
usage(const boost::program_options::options_description& options)
{
  std::cout << "Usage: ndnping [options] ndn:/name/prefix\n"
               "\n"
               "Ping a NDN name prefix using Interests with name ndn:/name/prefix/ping/number.\n"
               "The numbers in the Interests are randomly generated unless specified.\n"
               "\n"
            << options;
  exit(2);
}

static int
main(int argc, char* argv[])
{
  Options options;
  options.shouldAllowStaleData = false;
  options.nPings = -1;
  options.interval = 1_s;
  options.timeout = 4_s;
  options.startSeq = 0;
  options.shouldGenerateRandomSeq = true;
  options.shouldPrintTimestamp = false;

  std::string identifier;

  namespace po = boost::program_options;

  po::options_description visibleOptDesc("Options");
  visibleOptDesc.add_options()
    ("help,h",      "print this message and exit")
    ("version,V",   "display version and exit")
    ("interval,i",  po::value<time::milliseconds::rep>()->default_value(options.interval.count()),
                    "ping interval, in milliseconds")
    ("timeout,o",   po::value<time::milliseconds::rep>()->default_value(options.timeout.count()),
                    "ping timeout, in milliseconds")
    ("count,c",     po::value<int>(&options.nPings), "number of pings to send (default = no limit)")
    ("start,n",     po::value<uint64_t>(&options.startSeq),
                    "set the starting sequence number, the number is incremented by 1 after each Interest")
    ("identifier,p", po::value<std::string>(&identifier),
                     "add identifier to the Interest names before the sequence numbers to avoid conflicts")
    ("cache,a",     "allow routers to return stale Data from cache")
    ("timestamp,t", "print timestamp with messages")
  ;

  po::options_description hiddenOptDesc;
  hiddenOptDesc.add_options()
    ("prefix", po::value<std::string>(), "prefix to send pings to")
  ;

  po::options_description optDesc;
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  po::positional_options_description optPos;
  optPos.add("prefix", -1);

  try {
    po::variables_map optVm;
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), optVm);
    po::notify(optVm);

    if (optVm.count("help") > 0) {
      usage(visibleOptDesc);
    }

    if (optVm.count("version") > 0) {
      std::cout << "ndnping " << tools::VERSION << "\n";
      exit(0);
    }

    if (optVm.count("prefix") > 0) {
      options.prefix = Name(optVm["prefix"].as<std::string>());
    }
    else {
      std::cerr << "ERROR: No prefix specified\n";
      usage(visibleOptDesc);
    }

    options.interval = time::milliseconds(optVm["interval"].as<time::milliseconds::rep>());
    if (options.interval < 1_ms) {
      std::cerr << "ERROR: Specified ping interval is less than the minimum (1 ms)\n";
      usage(visibleOptDesc);
    }

    options.timeout = time::milliseconds(optVm["timeout"].as<time::milliseconds::rep>());

    if (optVm.count("count") > 0) {
      if (options.nPings <= 0) {
        std::cerr << "ERROR: Number of ping must be positive\n";
        usage(visibleOptDesc);
      }
    }

    if (optVm.count("start") > 0) {
      options.shouldGenerateRandomSeq = false;
    }

    if (optVm.count("identifier") > 0) {
      bool isIdentifierAcceptable = std::all_of(identifier.begin(), identifier.end(), &isalnum);
      if (identifier.empty() || !isIdentifierAcceptable) {
        std::cerr << "ERROR: Unacceptable client identifier\n";
        usage(visibleOptDesc);
      }

      options.clientIdentifier = name::Component(identifier);
    }

    if (optVm.count("cache") > 0) {
      options.shouldAllowStaleData = true;
    }

    if (optVm.count("timestamp") > 0) {
      options.shouldPrintTimestamp = true;
    }
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    usage(visibleOptDesc);
  }

  std::cout << "PING " << options.prefix << "\n";
  return Runner(options).run();
}

} // namespace ndn::ping::client

int
main(int argc, char* argv[])
{
  return ndn::ping::client::main(argc, argv);
}
