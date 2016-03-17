/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
 */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include "core/version.hpp"
#include "core/common.hpp"

#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace peek {

class NdnPeek : boost::noncopyable
{
public:
  explicit
  NdnPeek(char* programName)
    : isVerbose(false)
    , mustBeFresh(false)
    , wantRightmostChild(false)
    , wantPayloadOnly(false)
    , m_programName(programName)
    , m_minSuffixComponents(-1)
    , m_maxSuffixComponents(-1)
    , m_interestLifetime(-1)
    , m_timeout(-1)
    , m_prefixName("")
    , m_didReceiveData(false)
    , m_didReceiveNack(false)
  {
  }

  void
  usage(std::ostream& os ,const boost::program_options::options_description& options) const
  {
    os << "Usage: " << m_programName << " [options] ndn:/name\n"
          "   Fetch one data item matching the name prefix and write it to standard output\n"
          "\n"
       << options;
  }

  void
  setMinSuffixComponents(int minSuffixComponents)
  {
    if (minSuffixComponents < 0)
      throw std::out_of_range("'minSuffixComponents' must be a non-negative integer");

    m_minSuffixComponents = minSuffixComponents;
  }

  void
  setMaxSuffixComponents(int maxSuffixComponents)
  {
    if (maxSuffixComponents < 0)
      throw std::out_of_range("'maxSuffixComponents' must be a non-negative integer");

    m_maxSuffixComponents = maxSuffixComponents;
  }

  void
  setInterestLifetime(int interestLifetime)
  {
    if (interestLifetime < 0)
      throw std::out_of_range("'lifetime' must be a non-negative integer");

    m_interestLifetime = time::milliseconds(interestLifetime);
  }

  void
  setTimeout(int timeout)
  {
    if (timeout < 0)
      throw std::out_of_range("'timeout' must be a non-negative integer");

    m_timeout = time::milliseconds(timeout);
  }

  void
  setLink(const std::string& file)
  {
    m_link = io::load<Link>(file);
    if (m_link == nullptr)
      throw std::runtime_error(file + " is either nonreadable or nonparseable");
  }

  void
  setPrefixName(const std::string& prefixName)
  {
    m_prefixName = prefixName;
  }

  time::milliseconds
  getDefaultInterestLifetime()
  {
    return time::seconds(4);
  }

  Interest
  createInterestPacket()
  {
    Name interestName(m_prefixName);
    Interest interestPacket(interestName);

    if (mustBeFresh)
      interestPacket.setMustBeFresh(true);

    if (wantRightmostChild)
      interestPacket.setChildSelector(1);

    if (m_minSuffixComponents >= 0)
      interestPacket.setMinSuffixComponents(m_minSuffixComponents);

    if (m_maxSuffixComponents >= 0)
      interestPacket.setMaxSuffixComponents(m_maxSuffixComponents);

    if (m_interestLifetime < time::milliseconds::zero())
      interestPacket.setInterestLifetime(getDefaultInterestLifetime());
    else
      interestPacket.setInterestLifetime(m_interestLifetime);

    if (m_link != nullptr)
      interestPacket.setLink(m_link->wireEncode());

    if (isVerbose) {
      std::cerr << "INTEREST: " << interestPacket << std::endl;
    }

    return interestPacket;
  }

  void
  onData(const Interest& interest, const Data& data)
  {
    m_didReceiveData = true;

    if (isVerbose) {
      std::cerr << "DATA, RTT: "
                << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_expressInterestTime).count()
                << "ms" << std::endl;
    }

    if (wantPayloadOnly) {
      const Block& block = data.getContent();
      std::cout.write(reinterpret_cast<const char*>(block.value()), block.value_size());
    }
    else {
      const Block& block = data.wireEncode();
      std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
    }
  }

  void
  onNack(const Interest& interest, const lp::Nack& nack)
  {
    m_didReceiveNack = true;
    lp::NackHeader header = nack.getHeader();

    if (isVerbose) {
      std::cerr << "NACK, RTT: "
                << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_expressInterestTime).count()
                << "ms" << std::endl;
    }

    if (wantPayloadOnly) {
      std::cout << header.getReason() << std::endl;
    }
    else {
      const Block& block = header.wireEncode();
      std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
    }
  }

  void
  onTimeout(const Interest& interest)
  {
  }

  int
  run()
  {
    try {
      m_face.expressInterest(createInterestPacket(),
                             bind(&NdnPeek::onData, this, _1, _2),
                             bind(&NdnPeek::onNack, this, _1, _2),
                             bind(&NdnPeek::onTimeout, this, _1));
      m_expressInterestTime = time::steady_clock::now();
      if (m_timeout < time::milliseconds::zero()) {
        m_timeout = m_interestLifetime < time::milliseconds::zero() ?
                    getDefaultInterestLifetime() : m_interestLifetime;
      }
      m_face.processEvents(m_timeout);
    }
    catch (const std::exception& e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
    }

    if (m_didReceiveNack)
      return 4;

    if (isVerbose && !m_didReceiveData) {
      std::cerr << "TIMEOUT" << std::endl;
      return 3;
    }

    if (!m_didReceiveData)
      return 3;

    return 0;
  }

public:
  bool isVerbose;
  bool mustBeFresh;
  bool wantRightmostChild;
  bool wantPayloadOnly;

private:
  std::string m_programName;
  int m_minSuffixComponents;
  int m_maxSuffixComponents;
  time::milliseconds m_interestLifetime;
  time::milliseconds m_timeout;
  std::string m_prefixName;
  time::steady_clock::TimePoint m_expressInterestTime;
  shared_ptr<Link> m_link;
  bool m_didReceiveData;
  bool m_didReceiveNack;
  Face m_face;
};

int
main(int argc, char* argv[])
{
  NdnPeek program(argv[0]);

  namespace po = boost::program_options;

  po::options_description visibleOptDesc("Allowed options");
  visibleOptDesc.add_options()
    ("help,h", "print help and exit")
    ("version,V", "print version and exit")
    ("fresh,f", po::bool_switch(&program.mustBeFresh),
        "set MustBeFresh")
    ("rightmost,r", po::bool_switch(&program.wantRightmostChild),
        "set ChildSelector to rightmost")
    ("minsuffix,m", po::value<int>()->notifier(bind(&NdnPeek::setMinSuffixComponents, &program, _1)),
        "set MinSuffixComponents")
    ("maxsuffix,M", po::value<int>()->notifier(bind(&NdnPeek::setMaxSuffixComponents, &program, _1)),
        "set MaxSuffixComponents")
    ("lifetime,l", po::value<int>()->notifier(bind(&NdnPeek::setInterestLifetime, &program, _1)),
        "set InterestLifetime (in milliseconds)")
    ("payload,p", po::bool_switch(&program.wantPayloadOnly),
        "print payload only, instead of full packet")
    ("timeout,w", po::value<int>()->notifier(bind(&NdnPeek::setTimeout, &program, _1)),
        "set timeout (in milliseconds)")
    ("verbose,v", po::bool_switch(&program.isVerbose),
        "turn on verbose output")
    ("link-file", po::value<std::string>()->notifier(bind(&NdnPeek::setLink, &program, _1)),
        "set Link from a file")
  ;

  po::options_description hiddenOptDesc("Hidden options");
  hiddenOptDesc.add_options()
    ("prefix", po::value<std::string>(), "Interest name");

  po::options_description optDesc("Allowed options");
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  po::positional_options_description optPos;
  optPos.add("prefix", -1);

  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), vm);
    po::notify(vm);

    if (vm.count("help") > 0) {
      program.usage(std::cout, visibleOptDesc);
      return 0;
    }

    if (vm.count("version") > 0) {
      std::cout << "ndnpeek " << tools::VERSION << std::endl;
      return 0;
    }

    if (vm.count("prefix") > 0) {
      std::string prefixName = vm["prefix"].as<std::string>();
      program.setPrefixName(prefixName);
    }
    else {
      throw std::runtime_error("Required argument 'prefix' is missing");
    }
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    program.usage(std::cerr, visibleOptDesc);
    return 2;
  }

  return program.run();
}

} // namespace peek
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::peek::main(argc, argv);
}
