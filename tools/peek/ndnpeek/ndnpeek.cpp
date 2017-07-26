/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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
 * @author Zhuo Li <zhuoli@email.arizona.edu>
 */

#include "ndnpeek.hpp"

namespace ndn {
namespace peek {

NdnPeek::NdnPeek(Face& face, const PeekOptions& options)
  : m_face(face)
  , m_options(options)
  , m_timeout(options.timeout)
  , m_resultCode(ResultCode::TIMEOUT)
{
  if (m_timeout < time::milliseconds::zero()) {
    m_timeout = m_options.interestLifetime < time::milliseconds::zero() ?
                DEFAULT_INTEREST_LIFETIME : m_options.interestLifetime;
  }
}

time::milliseconds
NdnPeek::getTimeout() const
{
  return m_timeout;
}

ResultCode
NdnPeek::getResultCode() const
{
  return m_resultCode;
}

void
NdnPeek::start()
{
  m_face.expressInterest(createInterest(),
                         bind(&NdnPeek::onData, this, _2),
                         bind(&NdnPeek::onNack, this, _2),
                         nullptr);
  m_expressInterestTime = time::steady_clock::now();
}

Interest
NdnPeek::createInterest() const
{
  Interest interest(m_options.prefix);

  if (m_options.minSuffixComponents >= 0)
    interest.setMinSuffixComponents(m_options.minSuffixComponents);

  if (m_options.maxSuffixComponents >= 0)
    interest.setMaxSuffixComponents(m_options.maxSuffixComponents);

  if (m_options.interestLifetime >= time::milliseconds::zero())
    interest.setInterestLifetime(m_options.interestLifetime);

  if (m_options.link != nullptr)
    interest.setForwardingHint(m_options.link->getDelegationList());

  if (m_options.mustBeFresh)
    interest.setMustBeFresh(true);

  if (m_options.wantRightmostChild)
    interest.setChildSelector(1);

  if (m_options.isVerbose) {
    std::cerr << "INTEREST: " << interest << std::endl;
  }

  return interest;
}

void
NdnPeek::onData(const Data& data)
{
  m_resultCode = ResultCode::DATA;

  if (m_options.isVerbose) {
    std::cerr << "DATA, RTT: "
              << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_expressInterestTime).count()
              << "ms" << std::endl;
  }

  if (m_options.wantPayloadOnly) {
    const Block& block = data.getContent();
    std::cout.write(reinterpret_cast<const char*>(block.value()), block.value_size());
  }
  else {
    const Block& block = data.wireEncode();
    std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
  }
}

void
NdnPeek::onNack(const lp::Nack& nack)
{
  m_resultCode = ResultCode::NACK;
  lp::NackHeader header = nack.getHeader();

  if (m_options.isVerbose) {
    std::cerr << "NACK, RTT: "
              << time::duration_cast<time::milliseconds>(time::steady_clock::now() - m_expressInterestTime).count()
              << "ms" << std::endl;
  }

  if (m_options.wantPayloadOnly) {
    std::cout << header.getReason() << std::endl;
  }
  else {
    const Block& block = header.wireEncode();
    std::cout.write(reinterpret_cast<const char*>(block.wire()), block.size());
  }
}

} // namespace peek
} // namespace ndn
