/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California.
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
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "tools/pib/key-cache.hpp"

#include "tests/test-common.hpp"

namespace ndn {
namespace pib {
namespace tests {

BOOST_AUTO_TEST_SUITE(PibKeyCache)

BOOST_AUTO_TEST_CASE(Basic)
{
  KeyCache keyCache(3);

  Name name1("/1");
  Name name2("/2");
  Name name3("/3");
  Name name4("/4");

  auto key1 = make_shared<PublicKey>();
  auto key2 = make_shared<PublicKey>();
  auto key3 = make_shared<PublicKey>();
  auto key4 = make_shared<PublicKey>();

  keyCache.insert(name1, key1);
  keyCache.insert(name2, key2);
  keyCache.insert(name3, key3);

  BOOST_CHECK_EQUAL(keyCache.size(), 3);
  BOOST_CHECK(static_cast<bool>(keyCache.find(name1)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name2)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name3)));
  BOOST_CHECK(!static_cast<bool>(keyCache.find(name4)));

  keyCache.insert(name1, key1);
  keyCache.insert(name4, key4);
  BOOST_CHECK_EQUAL(keyCache.size(), 3);
  BOOST_CHECK(static_cast<bool>(keyCache.find(name1)));
  BOOST_CHECK(!static_cast<bool>(keyCache.find(name2)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name3)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name4)));

  keyCache.erase(name1);
  BOOST_CHECK_EQUAL(keyCache.size(), 2);
  BOOST_CHECK(!static_cast<bool>(keyCache.find(name1)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name3)));
  BOOST_CHECK(static_cast<bool>(keyCache.find(name4)));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace pib
} // namespace ndn
