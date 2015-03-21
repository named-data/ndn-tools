/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 Arizona Board of Regents
 *
 * This file is part of ndn-tlv-ping (Ping Application for Named Data Networking).
 *
 * ndn-tlv-ping is a free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-tlv-ping is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-tlv-ping, e.g., in LICENSE file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author: Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace ndn {

class NdnTlvPingServer : boost::noncopyable
{
public:

  explicit
  NdnTlvPingServer(char* programName)
    : m_programName(programName)
    , m_hasError(false)
    , m_isPrintTimestampSet(false)
    , m_freshnessPeriod(getMinimumFreshnessPeriod())
    , m_maximumPings(-1)
    , m_totalPings(0)
    , m_face(m_ioService)
  {
  }

  void
  usage()
  {
    std::cout << "\n Usage:\n " << m_programName << " ndn:/name/prefix [options]\n"
        " Starts a NDN ping server that responds to Interests with name"
        " ndn:/name/prefix/ping/number.\n"
        "   [-x freshness] - set FreshnessSeconds\n"
        "   [-p]           - specify number of pings to be satisfied (>=1)\n"
        "   [-t]           - print timestamp\n"
        "   [-h]           - print this message and exit\n\n";
    exit(1);
  }

  time::milliseconds
  getMinimumFreshnessPeriod()
  {
    return time::milliseconds(1000);
  }

  void
  setFreshnessPeriod(int freshnessPeriod)
  {
    if (freshnessPeriod <= 0)

    m_freshnessPeriod = time::milliseconds(freshnessPeriod);
  }

  void
  setMaximumPings(int maximumPings)
  {
    if (maximumPings <= 0)
      usage();
    m_maximumPings = maximumPings;
  }

  void
  setPrefix(char* prefix)
  {
    m_prefix = prefix;
  }

  void
  setPrintTimestamp()
  {
    m_isPrintTimestampSet = true;
  }

  bool
  hasError() const
  {
    return m_hasError;
  }

  void
  onInterest(const Name& name, const Interest& interest)
  {
    Name interestName = interest.getName();
    if (m_isPrintTimestampSet)
      std::cout << time::toIsoString(time::system_clock::now())  << " - ";
    std::cout << "Interest Received - Ping Reference = "
              << interestName.at(-1).toUri() << std::endl;
    char responseContent[] = "NDN TLV Ping Response";
    shared_ptr<Data> data = make_shared<Data>(interestName);
    data->setFreshnessPeriod(m_freshnessPeriod);
    data->setContent(reinterpret_cast<const uint8_t*>(responseContent),
                     sizeof(responseContent));
    m_keyChain.sign(*data);
    m_face.put(*data);
    ++m_totalPings;
    if (m_maximumPings > 0 && m_maximumPings == m_totalPings) {
      std::cout << "\n\nTotal Ping Interests Processed = " << m_totalPings << std::endl;
      std::cout << "Shutting Down Ping Server (" << m_name << ").\n" << std::endl;
      m_face.shutdown();
      m_ioService.stop();
    }
  }

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon" << std::endl;
    std::cerr << "REASON: " << reason << std::endl;
    m_hasError = true;
    m_face.shutdown();
    m_ioService.stop();
  }

  void
  signalHandler()
  {
    std::cout << "\n\nTotal Ping Interests Processed = " << m_totalPings << std::endl;
    std::cout << "Shutting Down Ping Server (" << m_name.toUri() << ").\n" << std::endl;
    m_face.shutdown();
    exit(1);
  }

  void
  run()
  {
    std::cout << "\n=== Ping Server " << m_prefix <<" ===\n" << std::endl;

    boost::asio::signal_set signalSet(m_ioService, SIGINT, SIGTERM);
    signalSet.async_wait(bind([this]() { signalHandler(); }));
    m_name = m_prefix;
    m_name.append("ping");
    m_face.setInterestFilter(m_name,
                             bind(&NdnTlvPingServer::onInterest,
                                  this, _1, _2),
                             bind(&NdnTlvPingServer::onRegisterFailed,
                                  this, _1,_2));
    try {
      m_face.processEvents();
    }
    catch (std::exception& e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
      m_hasError = true;
      m_ioService.stop();
    }
  }

private:
  char* m_programName;
  bool m_hasError;
  bool m_isPrintTimestampSet;
  time::milliseconds m_freshnessPeriod;
  int m_maximumPings;
  int m_totalPings;

  char* m_prefix;
  Name m_name;

  boost::asio::io_service m_ioService;
  KeyChain m_keyChain;
  Face m_face;
};

}

int
main(int argc, char* argv[])
{
  int res;

  ndn::NdnTlvPingServer ndnTlvPingServer(argv[0]);
  while ((res = getopt(argc, argv, "hdtp:x:")) != -1)
    {
      switch (res) {
      case 'h':
        ndnTlvPingServer.usage();
        break;
      case 'p':
        ndnTlvPingServer.setMaximumPings(atoi(optarg));
        break;
      case 'x':
        ndnTlvPingServer.setFreshnessPeriod(atoi(optarg));
        break;
      case 't':
        ndnTlvPingServer.setPrintTimestamp();
        break;
      default:
        ndnTlvPingServer.usage();
        break;
      }
    }

  argc -= optind;
  argv += optind;

  if (argv[0] == 0)
    ndnTlvPingServer.usage();

  ndnTlvPingServer.setPrefix(argv[0]);
  ndnTlvPingServer.run();

  std::cout << std::endl;

  if (ndnTlvPingServer.hasError())
    return 1;
  else
    return 0;
}
