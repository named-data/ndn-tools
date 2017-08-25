/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Arizona Board of Regents.
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

namespace ndn {
namespace ping {
namespace client {

class Runner : noncopyable
{
public:
  explicit
  Runner(const Options& options)
    : m_ping(m_face, options)
    , m_statisticsCollector(m_ping, options)
    , m_tracer(m_ping, options)
    , m_signalSetInt(m_face.getIoService(), SIGINT)
    , m_signalSetQuit(m_face.getIoService(), SIGQUIT)
  {
    m_signalSetInt.async_wait(bind(&Runner::afterIntSignal, this, _1));
    m_signalSetQuit.async_wait(bind(&Runner::afterQuitSignal, this, _1));

    m_ping.afterFinish.connect([this] {
        this->cancel();
      });
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

    Statistics statistics = m_statisticsCollector.computeStatistics();

    std::cout << statistics << std::endl;

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
  afterIntSignal(const boost::system::error_code& errorCode)
  {
    if (errorCode == boost::asio::error::operation_aborted) {
      return;
    }

    cancel();
  }

  void
  afterQuitSignal(const boost::system::error_code& errorCode)
  {
    if (errorCode == boost::asio::error::operation_aborted) {
      return;
    }

    m_statisticsCollector.computeStatistics().printSummary(std::cout);
    m_signalSetQuit.async_wait(bind(&Runner::afterQuitSignal, this, _1));
  }

private:
  Face m_face;
  Ping m_ping;
  StatisticsCollector m_statisticsCollector;
  Tracer m_tracer;

  boost::asio::signal_set m_signalSetInt;
  boost::asio::signal_set m_signalSetQuit;
};

static time::milliseconds
getMinimumPingInterval()
{
  return time::milliseconds(1);
}

static time::milliseconds
getDefaultPingInterval()
{
  return time::milliseconds(1000);
}

static time::milliseconds
getDefaultPingTimeoutThreshold()
{
  return time::milliseconds(4000);
}

static void
usage(const boost::program_options::options_description& options)
{
  std::cout << "Usage: ndnping [options] ndn:/name/prefix\n"
      "\n"
      "Ping a NDN name prefix using Interests with name ndn:/name/prefix/ping/number.\n"
      "The numbers in the Interests are randomly generated unless specified.\n"
      "\n";
  std::cout << options;
  exit(2);
}

int
main(int argc, char* argv[])
{
  Options options;
  options.shouldAllowStaleData = false;
  options.nPings = -1;
  options.interval = time::milliseconds(getDefaultPingInterval());
  options.timeout = time::milliseconds(getDefaultPingTimeoutThreshold());
  options.startSeq = 0;
  options.shouldGenerateRandomSeq = true;
  options.shouldPrintTimestamp = false;

  std::string identifier;

  namespace po = boost::program_options;

  po::options_description visibleOptDesc("Allowed options");
  visibleOptDesc.add_options()
    ("help,h", "print this message and exit")
    ("version,V", "display version and exit")
    ("interval,i", po::value<int>(),
                   ("set ping interval in milliseconds (default " +
                   std::to_string(getDefaultPingInterval().count()) + " ms - minimum " +
                   std::to_string(getMinimumPingInterval().count()) + " ms)").c_str())
    ("timeout,o", po::value<int>(),
                  ("set ping timeout in milliseconds (default " +
                  std::to_string(getDefaultPingTimeoutThreshold().count()) + " ms)").c_str())
    ("count,c", po::value<int>(&options.nPings), "set total number of pings")
    ("start,n", po::value<uint64_t>(&options.startSeq),
                "set the starting seq number, the number is incremented by 1 after each Interest")
    ("identifier,p", po::value<std::string>(&identifier),
                     "add identifier to the Interest names before the numbers to avoid conflict")
    ("cache,a", "allows routers to return stale Data from cache")
    ("timestamp,t", "print timestamp with messages")
  ;
  po::options_description hiddenOptDesc("Hidden options");
  hiddenOptDesc.add_options()
    ("prefix", po::value<std::string>(), "prefix to send pings to")
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
      std::cout << "ndnping " << tools::VERSION << std::endl;
      exit(0);
    }

    if (optVm.count("prefix") > 0) {
      options.prefix = Name(optVm["prefix"].as<std::string>());
    }
    else {
      std::cerr << "ERROR: No prefix specified" << std::endl;
      usage(visibleOptDesc);
    }

    if (optVm.count("interval") > 0) {
      options.interval = time::milliseconds(optVm["interval"].as<int>());

      if (options.interval.count() < getMinimumPingInterval().count()) {
        std::cerr << "ERROR: Specified ping interval is less than the minimum " <<
                     getMinimumPingInterval() << std::endl;
        usage(visibleOptDesc);
      }
    }

    if (optVm.count("timeout") > 0) {
      options.timeout = time::milliseconds(optVm["timeout"].as<int>());
    }

    if (optVm.count("count") > 0) {
      if (options.nPings <= 0) {
        std::cerr << "ERROR: Number of ping must be positive" << std::endl;
        usage(visibleOptDesc);
      }
    }

    if (optVm.count("start") > 0) {
      options.shouldGenerateRandomSeq = false;
    }

    if (optVm.count("identifier") > 0) {
      bool isIdentifierAcceptable = std::all_of(identifier.begin(), identifier.end(), &isalnum);
      if (identifier.empty() || !isIdentifierAcceptable) {
        std::cerr << "ERROR: Unacceptable client identifier" << std::endl;
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
    std::cerr << "ERROR: " << e.what() << std::endl;
    usage(visibleOptDesc);
  }

  std::cout << "PING " << options.prefix << std::endl;
  return Runner(options).run();
}

} // namespace client
} // namespace ping
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::ping::client::main(argc, argv);
}
