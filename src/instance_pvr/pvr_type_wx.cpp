/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr_type_wx.h"

namespace RTLRADIO
{
namespace INSTANCE
{

bool CPVRTypeWX::ScanChannels(
    std::vector<ChannelProps> channelsFound,
    const std::function<bool()>&& funcScanCancelled,
    const std::function<void(unsigned int)>&& funcScanPercentage,
    const std::function<void(const std::string& channel)>&& funcScanChannel,
    const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return true;
}

CPVRTypeWX::CPVRTypeWX(const std::shared_ptr<SETTINGS::CSettings>& settings,
                       std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
  : IPVRType(Modulation::WX, settings, deviceOutputBuffer)
{
  m_signalProps.filter = false; // Never apply the filter here
  m_signalProps.samplerate = 1600 KHz;
  m_signalProps.bandwidth = 200 KHz;
  m_signalProps.lowcut = -8 KHz;
  m_signalProps.highcut = 8 KHz;

  // Analog signals require a DC offset to be applied to prevent a natural
  // spike from occurring at the center frequency on many RTL-SDR devices
  m_signalProps.offset = (m_signalProps.samplerate / 4);
}

bool CPVRTypeWX::Scan(std::vector<ChannelProps>& channelsFound,
                      std::vector<ProviderProps>& providersFound,
                      const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                      const std::function<bool()>&& funcScanCancelled,
                      const std::function<void(unsigned int)>&& funcScanPercentage,
                      const std::function<void(const std::string& channel)>&& funcScanChannel,
                      const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return false;
}

bool CPVRTypeWX::Init()
{
  return false;
}

void CPVRTypeWX::Deinit()
{
  return;
}

std::shared_ptr<IPVRRadioSwitcher> CPVRTypeWX::GetRadioSwitcher()
{
  return nullptr;
}

} // namespace INSTANCE
} // namespace RTLRADIO
