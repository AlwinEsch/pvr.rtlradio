/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "inputstream_type_i.h"
#include "props.h"

#include <atomic>
#include <functional>

#include <kodi/addon-instance/Inputstream.h>

namespace RTLRADIO
{

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

namespace INSTANCE
{

class CInputstreamBase
{
public:
  CInputstreamBase(std::shared_ptr<SETTINGS::CSettings> settings);
  ~CInputstreamBase();

  bool Open(const std::string& url,
            const std::string& mimetype,
            unsigned int uniqueId,
            unsigned int frequency,
            unsigned int subchannel,
            Modulation modulation,
            IInputstreamType::AllocateDemuxPacketCB allocPacket);
  void Close();

  bool GetStreamIds(std::vector<unsigned int>& ids);
  bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream);
  void EnableStream(int streamid, bool enable);
  bool OpenStream(int streamid);
  void DemuxReset();
  void DemuxAbort();
  void DemuxFlush();
  DEMUX_PACKET* DemuxRead();
  bool GetTimes(kodi::addon::InputstreamTimes& times);

private:
  std::shared_ptr<SETTINGS::CSettings> m_settings;
  std::unique_ptr<IInputstreamType> m_activeType;

  std::atomic_bool m_active{false};
  std::shared_ptr<AudioPipeline> m_audioPipeline;

  // Settings
  std::string m_url;
  std::string m_mimetype;
  unsigned int m_uniqueId{0};
  unsigned int m_frequency{0};
  unsigned int m_subchannel{0};
  Modulation m_modulation{Modulation::UNDEFINED};
};

} // namespace INSTANCE
} // namespace RTLRADIO
