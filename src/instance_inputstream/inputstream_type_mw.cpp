/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "inputstream_type_mw.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstreamTypeMW::CInputstreamTypeMW(std::shared_ptr<SETTINGS::CSettings> settings,
                                       const std::shared_ptr<AudioPipeline>& audioPipeline)
  : IInputstreamType(settings, audioPipeline)
{
}

CInputstreamTypeMW::~CInputstreamTypeMW()
{
}

bool CInputstreamTypeMW::Open(unsigned int uniqueId,
                              unsigned int frequency,
                              unsigned int subchannel,
                              IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  AllocateDemuxPacket = std::move(allocPacket);

  return false;
}

void CInputstreamTypeMW::Close()
{
}

bool CInputstreamTypeMW::GetStreamIds(std::vector<unsigned int>& ids)
{
  return false;
}

bool CInputstreamTypeMW::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return false;
}

void CInputstreamTypeMW::EnableStream(int streamid, bool enable)
{
}

bool CInputstreamTypeMW::OpenStream(int streamid)
{
  return false;
}

void CInputstreamTypeMW::DemuxReset()
{
}

void CInputstreamTypeMW::DemuxAbort()
{
}

void CInputstreamTypeMW::DemuxFlush()
{
}

DEMUX_PACKET* CInputstreamTypeMW::DemuxRead()
{
  return nullptr;
}

bool CInputstreamTypeMW::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return false;
}

} // namespace INSTANCE
} // namespace RTLRADIO
