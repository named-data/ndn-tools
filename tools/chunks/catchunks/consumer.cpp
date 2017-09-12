/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016-2017, Regents of the University of California,
 *                          Colorado State University,
 *                          University Pierre & Marie Curie, Sorbonne University.
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

Consumer::Consumer(security::v2::Validator& validator, bool isVerbose, std::ostream& os)
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
  m_discover->onDiscoveryFailure.connect([] (const std::string& msg) {
    BOOST_THROW_EXCEPTION(std::runtime_error(msg));
  });
  m_discover->run();
}

void
Consumer::startPipeline(const Data& data)
{
  this->handleData(data);

  m_pipeline->run(data,
    [this] (const Interest&, const Data& data) { this->handleData(data); },
    [] (const std::string& msg) { BOOST_THROW_EXCEPTION(std::runtime_error(msg)); });
}

void
Consumer::handleData(const Data& data)
{
  auto dataPtr = data.shared_from_this();

  m_validator.validate(data,
    [this, dataPtr] (const Data& data) {
      if (data.getContentType() == ndn::tlv::ContentType_Nack) {
        if (m_isVerbose) {
          std::cerr << "Application level NACK: " << data << std::endl;
        }
        m_pipeline->cancel();
        BOOST_THROW_EXCEPTION(ApplicationNackError(data));
      }

      // 'data' passed to callback comes from DataValidationState and was not created with make_shared
      m_bufferedData[getSegmentFromPacket(data)] = dataPtr;
      writeInOrderData();
    },
    [] (const Data& data, const security::v2::ValidationError& error) {
      BOOST_THROW_EXCEPTION(DataValidationError(error));
    });
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
