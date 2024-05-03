/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "dsp_dab/app_ofdm_blocks.h"
#include "pvr_radio_switcher_i.h"
#include "pvr_type_i.h"

class OFDM_Block;
class AudioPipeline;
class BasicRadio;

namespace RTLRADIO
{
namespace INSTANCE
{

class CBasicRadioSwitcher;

class CPVRTypeDAB : public IPVRType
{
public:
  CPVRTypeDAB(const std::shared_ptr<SETTINGS::CSettings>& settings,
              std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer);
  ~CPVRTypeDAB();

  bool Scan(std::vector<ChannelProps>& channelsFound,
            const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
            const std::function<bool()>&& funcScanCancelled,
            const std::function<void(unsigned int)>&& funcScanPercentage,
            const std::function<void(const std::string& channel)>&& funcScanChannel,
            const std::function<void(const ChannelProps&)>&& funcScanChannelFound) override;

  bool Init() override;
  void Deinit() override;

  std::shared_ptr<IPVRRadioSwitcher> GetRadioSwitcher() override;

private:
  static void AttachAudioPipelineToRadio(std::shared_ptr<AudioPipeline> audio_pipeline,
                                         BasicRadio& basic_radio);

  std::shared_ptr<OFDM_Block> m_OFDMBlock;
  std::shared_ptr<ThreadedRingBuffer<viterbi_bit_t>> m_OFDMToRadioBuffer;
  std::shared_ptr<CBasicRadioSwitcher> m_radioSwitcher;
};

} // namespace INSTANCE
} // namespace RTLRADIO
