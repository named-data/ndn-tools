/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2016,  Regents of the University of California,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University.
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
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Wentao Shang
 * @author Steve DiBenedetto
 * @author Andrea Tosatto
 */

#include "consumer.hpp"

namespace ndn {
namespace chunks {

Consumer::Consumer(Validator& validator, bool isVerbose, std::ostream& os)
  : m_validator(validator)
  , m_outputStream(os)
  , m_nextToPrint(0)
  , m_isVerbose(isVerbose)
{
}

void
Consumer::run(unique_ptr<DiscoverVersion> discover, unique_ptr<PipelineInterests> pipeline)
{
  m_discover = std::move(discover);
  m_pipeline = std::move(pipeline);
  m_nextToPrint = 0;
  m_bufferedData.clear();

  m_discover->onDiscoverySuccess.connect(bind(&Consumer::startPipeline, this, _1));
  m_discover->onDiscoveryFailure.connect(bind(&Consumer::onFailure, this, _1));
  m_discover->run();
}

void
Consumer::startPipeline(const Data& data)
{
  m_validator.validate(data,
                       bind(&Consumer::onDataValidated, this, _1),
                       bind(&Consumer::onFailure, this, _2));

  m_pipeline->run(data,
                  bind(&Consumer::onData, this, _1, _2),
                  bind(&Consumer::onFailure, this, _1));
}

void
Consumer::onData(const Interest& interest, const Data& data)
{
  m_validator.validate(data,
                       bind(&Consumer::onDataValidated, this, _1),
                       bind(&Consumer::onFailure, this, _2));
}

void
Consumer::onDataValidated(shared_ptr<const Data> data)
{
  if (data->getContentType() == ndn::tlv::ContentType_Nack) {
    if (m_isVerbose)
      std::cerr << "Application level NACK: " << *data << std::endl;

    m_pipeline->cancel();
    throw ApplicationNackError(*data);
  }

  m_bufferedData[data->getName()[-1].toSegment()] = data;
  writeInOrderData();
}

void
Consumer::onFailure(const std::string& reason)
{
  throw std::runtime_error(reason);
}

void
Consumer::writeInOrderData()
{
  for (auto it = m_bufferedData.begin();
       it != m_bufferedData.end() && it->first == m_nextToPrint;
       it = m_bufferedData.erase(it), ++m_nextToPrint) {
    const Block& content = it->second->getContent();
    m_outputStream.write(reinterpret_cast<const char*>(content.value()), content.value_size());
  }
}

} // namespace chunks
} // namespace ndn
