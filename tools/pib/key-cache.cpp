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

#include "key-cache.hpp"

namespace ndn {
namespace pib {

KeyCacheEntry::KeyCacheEntry(const Name& name, shared_ptr<PublicKey> key)
  : name(name)
  , key(key)
{
  BOOST_ASSERT(static_cast<bool>(key));
}

KeyCache::KeyCache(size_t capacity)
  : m_capacity(capacity)
{
}

void
KeyCache::insert(const Name& name, shared_ptr<PublicKey> key)
{
  // check if key exist
  KeyContainer::index<byName>::type::iterator it = m_keys.get<byName>().find(name);
  if (it != m_keys.get<byName>().end()) {
    adjustLru(it);
    return;
  }

  // evict key when capacity is reached
  if (size() >= m_capacity)
    evictKey();

  // insert entry
  m_keys.insert(KeyCacheEntry(name, key));
}

shared_ptr<PublicKey>
KeyCache::find(const Name& name) const
{
  // check if key exist
  KeyContainer::index<byName>::type::iterator it = m_keys.get<byName>().find(name);
  if (it == m_keys.get<byName>().end())
    return shared_ptr<PublicKey>();
  else {
    // adjust lru
    shared_ptr<PublicKey> key = it->key;
    adjustLru(it);
    return key;
  }
}

void
KeyCache::erase(const Name& name)
{
  // check if key exist
  KeyContainer::index<byName>::type::iterator it = m_keys.get<byName>().find(name);
  if (it != m_keys.get<byName>().end()) {
    m_keys.erase(it);
  }
}

size_t
KeyCache::size() const
{
  return m_keys.size();
}

void
KeyCache::evictKey()
{
  if (!m_keys.get<byUsedTime>().empty()) {
    KeyContainer::index<byUsedTime>::type::iterator it = m_keys.get<byUsedTime>().begin();
    m_keys.erase(m_keys.project<0>(it));
  }
}

void
KeyCache::adjustLru(KeyContainer::iterator it) const
{
  KeyCacheEntry entry = std::move(*it);
  m_keys.erase(it);
  m_keys.insert(entry);
}

} // namespace pib
} // namespace ndn
