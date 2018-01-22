/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California.
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

#include "boost-test.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdlib.h>

namespace ndn {
namespace tools {
namespace tests {

class GlobalConfiguration
{
public:
  GlobalConfiguration()
  {
    const char* envHome = ::getenv("HOME");
    if (envHome)
      m_home = envHome;

    boost::filesystem::path dir{TMP_TESTS_PATH};
    dir /= "test-home";
    ::setenv("HOME", dir.c_str(), 1);

    boost::filesystem::create_directories(dir);
    std::ofstream clientConf((dir / ".ndn" / "client.conf").c_str());
    clientConf << "pib=pib-sqlite3" << std::endl
               << "tpm=tpm-file" << std::endl;
  }

  ~GlobalConfiguration()
  {
    if (!m_home.empty())
      ::setenv("HOME", m_home.data(), 1);
  }

private:
  std::string m_home;
};

#if BOOST_VERSION >= 106500
BOOST_TEST_GLOBAL_CONFIGURATION(GlobalConfiguration);
#elif BOOST_VERSION >= 105900
BOOST_GLOBAL_FIXTURE(GlobalConfiguration);
#else
BOOST_GLOBAL_FIXTURE(GlobalConfiguration)
#endif

} // namespace tests
} // namespace tools
} // namespace ndn
