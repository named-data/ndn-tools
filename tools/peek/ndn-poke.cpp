/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
 */

#include "core/version.hpp"

#include <sstream>

namespace ndn {
namespace peek {

class NdnPoke : boost::noncopyable
{
public:
  explicit
  NdnPoke(const char* programName)
    : m_programName(programName)
    , m_wantForceData(false)
    , m_wantDigestSha256(false)
    , m_wantFinalBlockId(false)
    , m_freshnessPeriod(-1)
    , m_timeout(-1)
    , m_isDataSent(false)
  {
  }

  void
  usage()
  {
    std::cout << "\n Usage:\n " << m_programName << " "
      "[-f] [-D] [-i identity] [-F] [-x freshness] [-w timeout] ndn:/name\n"
      "   Reads payload from stdin and sends it to local NDN forwarder as a "
      "single Data packet\n"
      "   [-f]          - force, send Data without waiting for Interest\n"
      "   [-D]          - use DigestSha256 signing method instead of "
      "SignatureSha256WithRsa\n"
      "   [-i identity] - set identity to be used for signing\n"
      "   [-F]          - set FinalBlockId to the last component of Name\n"
      "   [-x]          - set FreshnessPeriod in time::milliseconds\n"
      "   [-w timeout]  - set Timeout in time::milliseconds\n"
      "   [-h]          - print help and exit\n"
      "   [-V]          - print version and exit\n"
      "\n";
    exit(1);
  }

  void
  setForceData()
  {
    m_wantForceData = true;
  }

  void
  setUseDigestSha256()
  {
    m_wantDigestSha256 = true;
  }

  void
  setIdentityName(const char* identityName)
  {
    m_identityName = make_shared<Name>(identityName);
  }

  void
  setLastAsFinalBlockId()
  {
    m_wantFinalBlockId = true;
  }

  void
  setFreshnessPeriod(int freshnessPeriod)
  {
    if (freshnessPeriod < 0)
      usage();

    m_freshnessPeriod = time::milliseconds(freshnessPeriod);
  }

  void
  setTimeout(int timeout)
  {
    if (timeout < 0)
      usage();

    m_timeout = time::milliseconds(timeout);
  }

  void
  setPrefixName(const char* prefixName)
  {
    m_prefixName = Name(prefixName);
  }

  time::milliseconds
  getDefaultTimeout()
  {
    return time::seconds(10);
  }

  shared_ptr<Data>
  createDataPacket()
  {
    auto dataPacket = make_shared<Data>(m_prefixName);

    std::stringstream payloadStream;
    payloadStream << std::cin.rdbuf();
    std::string payload = payloadStream.str();
    dataPacket->setContent(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());

    if (m_freshnessPeriod >= time::milliseconds::zero())
      dataPacket->setFreshnessPeriod(m_freshnessPeriod);

    if (m_wantFinalBlockId) {
      if (!m_prefixName.empty())
        dataPacket->setFinalBlock(m_prefixName.get(-1));
      else {
        std::cerr << "Name Provided Has 0 Components" << std::endl;
        exit(1);
      }
    }

    if (m_wantDigestSha256) {
      m_keyChain.sign(*dataPacket, signingWithSha256());
    }
    else {
      if (m_identityName == nullptr) {
        m_keyChain.sign(*dataPacket);
      }
      else {
        m_keyChain.sign(*dataPacket, signingByIdentity(*m_identityName));
      }
    }

    return dataPacket;
  }

  void
  onInterest(const Name& name, const Interest& interest, const shared_ptr<Data>& data)
  {
    m_face.put(*data);
    m_isDataSent = true;
    m_face.shutdown();
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "Prefix Registration Failure." << std::endl;
    std::cerr << "Reason = " << reason << std::endl;
  }

  void
  run()
  {
    try {
      shared_ptr<Data> dataPacket = createDataPacket();
      if (m_wantForceData) {
        m_face.put(*dataPacket);
        m_isDataSent = true;
      }
      else {
        m_face.setInterestFilter(m_prefixName,
                                 bind(&NdnPoke::onInterest, this, _1, _2, dataPacket),
                                 RegisterPrefixSuccessCallback(),
                                 bind(&NdnPoke::onRegisterFailed, this, _1, _2));
      }

      if (m_timeout < time::milliseconds::zero())
        m_face.processEvents(getDefaultTimeout());
      else
        m_face.processEvents(m_timeout);
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR: " << e.what() << "\n" << std::endl;
      exit(1);
    }
  }

  bool
  isDataSent() const
  {
    return m_isDataSent;
  }

private:
  KeyChain m_keyChain;
  std::string m_programName;
  bool m_wantForceData;
  bool m_wantDigestSha256;
  shared_ptr<Name> m_identityName;
  bool m_wantFinalBlockId;
  time::milliseconds m_freshnessPeriod;
  time::milliseconds m_timeout;
  Name m_prefixName;
  bool m_isDataSent;
  Face m_face;
};

int
main(int argc, char* argv[])
{
  NdnPoke program(argv[0]);

  int option;
  while ((option = getopt(argc, argv, "hfDi:Fx:w:V")) != -1) {
    switch (option) {
    case 'h':
      program.usage();
      break;
    case 'f':
      program.setForceData();
      break;
    case 'D':
      program.setUseDigestSha256();
      break;
    case 'i':
      program.setIdentityName(optarg);
      break;
    case 'F':
      program.setLastAsFinalBlockId();
      break;
    case 'x':
      program.setFreshnessPeriod(atoi(optarg));
      break;
    case 'w':
      program.setTimeout(atoi(optarg));
      break;
    case 'V':
      std::cout << "ndnpoke " << tools::VERSION << std::endl;
      return 0;
    default:
      program.usage();
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (!argv[0])
    program.usage();

  program.setPrefixName(argv[0]);
  program.run();

  if (program.isDataSent())
    return 0;
  else
    return 1;
}

} // namespace peek
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::peek::main(argc, argv);
}
