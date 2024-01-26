/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California.
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

#ifndef NDN_TOOLS_DISSECT_DISSECTOR_HPP
#define NDN_TOOLS_DISSECT_DISSECTOR_HPP

#include "core/common.hpp"

#include <ndn-cxx/encoding/block.hpp>

#include <vector>

namespace ndn::dissect {

struct Options
{
  bool dissectContent = false;
};

class Dissector : noncopyable
{
public:
  Dissector(std::istream& input, std::ostream& output, const Options& options);

  void
  dissect();

private:
  void
  printBranches();

  void
  printType(uint32_t type);

  void
  printBlock(const Block& block);

private:
  const Options& m_options;
  std::istream& m_in;
  std::ostream& m_out;

  // m_branches[i] is true iff the i-th level of the tree has more branches after the current one
  std::vector<bool> m_branches;
};

} // namespace ndn::dissect

#endif // NDN_TOOLS_DISSECT_DISSECTOR_HPP
