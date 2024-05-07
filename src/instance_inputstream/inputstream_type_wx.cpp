/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "inputstream_type_wx.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstreamTypeWX::CInputstreamTypeWX(std::shared_ptr<SETTINGS::CSettings> settings,
                                       const std::shared_ptr<AudioPipeline>& audioPipeline)
  : IInputstreamType(settings, audioPipeline)
{
}

CInputstreamTypeWX::~CInputstreamTypeWX()
{
}

bool CInputstreamTypeWX::Open(unsigned int uniqueId,
                              unsigned int frequency,
                              unsigned int subchannel,
                              IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  AllocateDemuxPacket = std::move(allocPacket);
  return false;
}

void CInputstreamTypeWX::Close()
{
}

bool CInputstreamTypeWX::GetStreamIds(std::vector<unsigned int>& ids)
{
  return false;
}

bool CInputstreamTypeWX::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return false;
}

void CInputstreamTypeWX::EnableStream(int streamid, bool enable)
{
}

bool CInputstreamTypeWX::OpenStream(int streamid)
{
  return false;
}

void CInputstreamTypeWX::DemuxReset()
{
}

void CInputstreamTypeWX::DemuxAbort()
{
}

void CInputstreamTypeWX::DemuxFlush()
{
}

DEMUX_PACKET* CInputstreamTypeWX::DemuxRead()
{
  return nullptr;
}

bool CInputstreamTypeWX::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return false;
}

} // namespace INSTANCE
} // namespace RTLRADIO
