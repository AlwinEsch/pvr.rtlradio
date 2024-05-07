/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "inputstream_type_hd.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstreamTypeHD::CInputstreamTypeHD(std::shared_ptr<SETTINGS::CSettings> settings,
                                       const std::shared_ptr<AudioPipeline>& audioPipeline)
  : IInputstreamType(settings, audioPipeline)
{
}

CInputstreamTypeHD::~CInputstreamTypeHD()
{
}

bool CInputstreamTypeHD::Open(unsigned int uniqueId,
                              unsigned int frequency,
                              unsigned int subchannel,
                              IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  AllocateDemuxPacket = std::move(allocPacket);

  return false;
}

void CInputstreamTypeHD::Close()
{
}

bool CInputstreamTypeHD::GetStreamIds(std::vector<unsigned int>& ids)
{
  return false;
}

bool CInputstreamTypeHD::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return false;
}

void CInputstreamTypeHD::EnableStream(int streamid, bool enable)
{
}

bool CInputstreamTypeHD::OpenStream(int streamid)
{
  return false;
}

void CInputstreamTypeHD::DemuxReset()
{
}

void CInputstreamTypeHD::DemuxAbort()
{
}

void CInputstreamTypeHD::DemuxFlush()
{
}

DEMUX_PACKET* CInputstreamTypeHD::DemuxRead()
{
  return nullptr;
}

bool CInputstreamTypeHD::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return false;
}

} // namespace INSTANCE
} // namespace RTLRADIO
