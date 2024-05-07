/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "dsp_dab/app_ofdm_blocks.h"
#include "dsp_dab/process_lib/basic_radio/basic_audio_channel.h"
#include "dsp_dab/process_lib/basic_radio/basic_radio.h"
#include "inputstream_type_i.h"
#include "props.h"
#include "utils/app_io_buffers.h"
#include "utils/raw_iq.h"
#include "utils/span.h"

#include <atomic>
#include <memory>

class AudioPipeline;

namespace RTLRADIO
{
namespace INSTANCE
{

class CDeviceSource;
class CBasicRadioSwitcher;

class CInputstreamTypeDAB : public IInputstreamType
{
public:
  CInputstreamTypeDAB(std::shared_ptr<SETTINGS::CSettings> settings,
                      const std::shared_ptr<AudioPipeline>& audioPipeline);
  ~CInputstreamTypeDAB();

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

  std::string_view GetName() const override { return "DAB/DAB+ radio"; }

private:
  void SetSettingsChangeCallback(const std::string& id,
                                 const kodi::addon::CSettingValue& settingValue);

  void attach_audio_pipeline_to_radio(const std::shared_ptr<BasicRadio>& basic_radio);

  static constexpr const uint32_t STREAM_ID_ID3TAG = 0x1;

  int m_callbackSettingsChangeID{-1};

  std::shared_ptr<OFDM_Block> m_ofdm_block;
  std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> m_device_output_buffer;
  std::shared_ptr<CBasicRadioSwitcher> m_radio_switcher;
  std::shared_ptr<CDeviceSource> m_deviceSource;
  std::shared_ptr<ThreadedRingBuffer<viterbi_bit_t>> m_ofdm_to_radio_buffer;
  std::unique_ptr<std::thread> m_thread_select_default_tuner;
  std::unique_ptr<std::thread> m_thread_ofdm_run;
  std::unique_ptr<std::thread> m_thread_radio_switcher;
  double m_PTSNext;

  // Settings
  std::string url;
  std::string mimetype;
  unsigned int m_uniqueId;
  unsigned int m_frequency;
  unsigned int m_subchannel;
  Modulation m_modulation;

  DEMUX_PACKET* m_unusedPacket = nullptr;
};

} // namespace INSTANCE
} // namespace RTLRADIO
