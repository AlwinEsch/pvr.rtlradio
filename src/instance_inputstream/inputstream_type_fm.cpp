/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "inputstream_type_fm.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstreamTypeFM::CInputstreamTypeFM(std::shared_ptr<SETTINGS::CSettings> settings,
                                       const std::shared_ptr<AudioPipeline>& audioPipeline)
  : IInputstreamType(settings, audioPipeline)
{
}

CInputstreamTypeFM::~CInputstreamTypeFM()
{
}

bool CInputstreamTypeFM::Open(unsigned int uniqueId,
                              unsigned int frequency,
                              unsigned int subchannel,
                              IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  AllocateDemuxPacket = std::move(allocPacket);

  return false;
}

void CInputstreamTypeFM::Close()
{
}

bool CInputstreamTypeFM::GetStreamIds(std::vector<unsigned int>& ids)
{
  return false;
}

bool CInputstreamTypeFM::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return false;
}

void CInputstreamTypeFM::EnableStream(int streamid, bool enable)
{
}

bool CInputstreamTypeFM::OpenStream(int streamid)
{
  return false;
}

void CInputstreamTypeFM::DemuxReset()
{
}

void CInputstreamTypeFM::DemuxAbort()
{
}

void CInputstreamTypeFM::DemuxFlush()
{
}

DEMUX_PACKET* CInputstreamTypeFM::DemuxRead()
{
  return nullptr;
}

bool CInputstreamTypeFM::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return false;
}

} // namespace INSTANCE
} // namespace RTLRADIO
