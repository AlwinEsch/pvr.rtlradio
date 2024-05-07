/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "audio/audio_pipeline.h"

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

static constexpr const uint32_t STREAM_AUDIO_SAMPLERATE = uint32_t(DEFAULT_AUDIO_SAMPLE_RATE);
static constexpr const size_t STREAM_FRAMES_PER_BUFFER = DEFAULT_AUDIO_SINK_SAMPLES;
static constexpr const uint64_t NO_PTS_VALUE = std::numeric_limits<uint64_t>::max();
static constexpr const double STREAM_PACKET_DURATION =
    double(STREAM_TIME_BASE) / double(STREAM_AUDIO_SAMPLERATE / STREAM_FRAMES_PER_BUFFER);

class IInputstreamType
{
public:
  using AllocateDemuxPacketCB = std::function<DEMUX_PACKET*(int)>;

  IInputstreamType(std::shared_ptr<SETTINGS::CSettings> settings,
                   const std::shared_ptr<AudioPipeline>& audioPipeline)
    : m_settings(settings), m_audioPipeline(audioPipeline)
  {
  }
  ~IInputstreamType() = default;

  virtual bool Open(unsigned int uniqueId,
                    unsigned int frequency,
                    unsigned int subchannel,
                    IInputstreamType::AllocateDemuxPacketCB allocPacket) = 0;
  virtual void Close() = 0;

  virtual bool GetStreamIds(std::vector<unsigned int>& ids) = 0;
  virtual bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) = 0;
  virtual void EnableStream(int streamid, bool enable) = 0;
  virtual bool OpenStream(int streamid) = 0;
  virtual void DemuxReset() = 0;
  virtual void DemuxAbort() = 0;
  virtual void DemuxFlush() = 0;
  virtual DEMUX_PACKET* DemuxRead() = 0;
  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) { return false; }
  virtual std::string_view GetName() const = 0;

protected:
  std::shared_ptr<SETTINGS::CSettings> m_settings;
  AllocateDemuxPacketCB AllocateDemuxPacket;
  unsigned int m_uniqueId{0};
  unsigned int m_frequency{0};
  unsigned int m_subchannel{0};
  std::shared_ptr<AudioPipeline> m_audioPipeline;
};

} // namespace INSTANCE
} // namespace RTLRADIO
