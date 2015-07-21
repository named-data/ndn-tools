/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "response-cache.hpp"

namespace ndn {
namespace pib {

using std::map;

ResponseCache::ResponseCache()
{
}


shared_ptr<const Data>
ResponseCache::find(const Name& dataName, bool hasVersion) const
{
  if (!hasVersion) {
    Storage::const_iterator it = m_storage.find(dataName);
    if (it != m_storage.end())
      return it->second;
    else
      return shared_ptr<const Data>();
  }
  else {
    Storage::const_iterator it = m_storage.find(dataName.getPrefix(-1));
    if (it != m_storage.end() && it->second->getName() == dataName)
      return it->second;
    else
      return shared_ptr<const Data>();
  }
}

void
ResponseCache::insert(const Data& data)
{
  data.getName().at(-1).toVersion(); // ensures last component is version
  m_storage[data.getName().getPrefix(-1)] = data.shared_from_this();
}

void
ResponseCache::erase(const Name& dataNameWithoutVersion)
{
  m_storage.erase(dataNameWithoutVersion);
}

void
ResponseCache::clear()
{
  m_storage.clear();
}

} // namespace pib
} // namespace ndn
