/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "inputstream_type_i.h"

namespace RTLRADIO
{
namespace INSTANCE
{

class CInputstreamTypeWX : public IInputstreamType
{
public:
  CInputstreamTypeWX(std::shared_ptr<SETTINGS::CSettings> settings,
                     const std::shared_ptr<AudioPipeline>& audioPipeline);
  ~CInputstreamTypeWX();

  bool Open(unsigned int uniqueId,
            unsigned int frequency,
            unsigned int subchannel,
            IInputstreamType::AllocateDemuxPacketCB allocPacket) override;
  void Close() override;

  bool GetStreamIds(std::vector<unsigned int>& ids) override;
  bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) override;
  void EnableStream(int streamid, bool enable) override;
  bool OpenStream(int streamid) override;
  void DemuxReset() override;
  void DemuxAbort() override;
  void DemuxFlush() override;
  DEMUX_PACKET* DemuxRead() override;
  bool GetTimes(kodi::addon::InputstreamTimes& times) override;

  std::string_view GetName() const override { return "WX radio"; }

private:
};

} // namespace INSTANCE
} // namespace RTLRADIO
