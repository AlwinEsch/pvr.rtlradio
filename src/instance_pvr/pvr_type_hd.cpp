/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr_type_hd.h"

namespace RTLRADIO
{
namespace INSTANCE
{

bool CPVRTypeHD::ScanChannels(
    std::vector<ChannelProps> channelsFound,
    std::vector<ProviderProps>& providersFound,
    const std::function<bool()>&& funcScanCancelled,
    const std::function<void(unsigned int)>&& funcScanPercentage,
    const std::function<void(const std::string& channel)>&& funcScanChannel,
    const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return true;
}

CPVRTypeHD::CPVRTypeHD(const std::shared_ptr<SETTINGS::CSettings>& settings,
                       std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
  : IPVRType(Modulation::HD, settings, deviceOutputBuffer)
{
  m_signalProps.filter = false; // Never apply the filter here
  m_signalProps.samplerate = 1488375;
  m_signalProps.bandwidth = 440 KHz;
  m_signalProps.lowcut = -204 KHz;
  m_signalProps.highcut = 204 KHz;
  m_signalProps.offset = 0;
}

bool CPVRTypeHD::Scan(std::vector<ChannelProps>& channelsFound,
                      std::vector<ProviderProps>& providersFound,
                      const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                      const std::function<bool()>&& funcScanCancelled,
                      const std::function<void(unsigned int)>&& funcScanPercentage,
                      const std::function<void(const std::string& channel)>&& funcScanChannel,
                      const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return false;
}

bool CPVRTypeHD::Init()
{
  return false;
}

void CPVRTypeHD::Deinit()
{
  return;
}

std::shared_ptr<IPVRRadioSwitcher> CPVRTypeHD::GetRadioSwitcher()
{
  return nullptr;
}

} // namespace INSTANCE
} // namespace RTLRADIO
