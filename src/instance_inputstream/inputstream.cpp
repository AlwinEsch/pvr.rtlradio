/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "inputstream.h"

#include "inputstream_base.h"
#include "props.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstream::CInputstream(const kodi::addon::IInstanceInfo& instance, CInputstreamBase& base)
  : kodi::addon::CInstanceInputStream(instance), m_base(base)
{
}

CInputstream::~CInputstream() = default;

void CInputstream::GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities)
{
  uint32_t mask = INPUTSTREAM_SUPPORTS_IDEMUX;
  //mask |= INPUTSTREAM_SUPPORTS_IPOSTIME;
  mask |= INPUTSTREAM_SUPPORTS_IDISPLAYTIME;
  mask |= INPUTSTREAM_SUPPORTS_ITIME;

  capabilities.SetMask(mask);
}

bool CInputstream::Open(const kodi::addon::InputstreamProperty& props)
{
  assert(props.GetPropertiesAmount() >= 4);

  // Settings
  const std::string url = props.GetURL();
  const std::string mimetype = props.GetMimeType();
  unsigned int uniqueId;
  unsigned int frequency;
  unsigned int subchannel;
  Modulation modulation;

  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Open inputstream:");
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, " - URL:      {}", url);
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, " - Mimetype: {}", mimetype);

  uint8_t cnt = 0;
  for (const auto& property : props.GetProperties())
  {
    if (property.second.empty())
    {
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Property {} given empty",
                 property.first);
      return false;
    }

    if (property.first == PVR_STREAM_PROPERTY_UNIQUEID)
    {
      uniqueId = std::atoi(property.second.c_str());
      ++cnt;
    }
    else if (property.first == PVR_STREAM_PROPERTY_FREQUENCY)
    {
      frequency = std::atoi(property.second.c_str());
      ++cnt;
    }
    else if (property.first == PVR_STREAM_PROPERTY_SUBCHANNEL)
    {
      subchannel = std::atoi(property.second.c_str());
      ++cnt;
    }
    else if (property.first == PVR_STREAM_PROPERTY_MODULATION)
    {
      modulation = static_cast<Modulation>(std::atoi(property.second.c_str()));
      ++cnt;
    }
    UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, " - Property: {}={}", property.first,
               property.second);
  }

  if (cnt < 4)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "One or more required properties missing");
    return false;
  }

  return m_base.Open(
      url, mimetype, uniqueId, frequency, subchannel,
      modulation, [&](int size) -> auto { return AllocateDemuxPacket(size); });
}

void CInputstream::Close()
{
  m_base.Close();
}

bool CInputstream::GetStreamIds(std::vector<unsigned int>& ids)
{
  return m_base.GetStreamIds(ids);
}

bool CInputstream::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return m_base.GetStream(streamid, stream);
}

void CInputstream::EnableStream(int streamid, bool enable)
{
  m_base.EnableStream(streamid, enable);
}

bool CInputstream::OpenStream(int streamid)
{
  return m_base.OpenStream(streamid);
}

void CInputstream::DemuxReset()
{
  m_base.DemuxReset();
}

void CInputstream::DemuxAbort()
{
  m_base.DemuxAbort();
}

void CInputstream::DemuxFlush()
{
  m_base.DemuxFlush();
}

DEMUX_PACKET* CInputstream::DemuxRead()
{
  return m_base.DemuxRead();
}

} // namespace INSTANCE
} // namespace RTLRADIO
