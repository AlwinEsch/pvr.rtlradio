/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr_type_mw.h"

namespace RTLRADIO
{
namespace INSTANCE
{

bool CPVRTypeMW::ScanChannels(
    std::vector<ChannelProps> channelsFound,
    const std::function<bool()>&& funcScanCancelled,
    const std::function<void(unsigned int)>&& funcScanPercentage,
    const std::function<void(const std::string& channel)>&& funcScanChannel,
    const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return true;
}

CPVRTypeMW::CPVRTypeMW(const std::shared_ptr<SETTINGS::CSettings>& settings,
                       std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
  : IPVRType(Modulation::MW, settings, deviceOutputBuffer)
{
  m_signalProps.filter = false; // Never apply the filter here
}

bool CPVRTypeMW::Scan(std::vector<ChannelProps>& channelsFound,
                      std::vector<ProviderProps>& providersFound,
                      const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                      const std::function<bool()>&& funcScanCancelled,
                      const std::function<void(unsigned int)>&& funcScanPercentage,
                      const std::function<void(const std::string& channel)>&& funcScanChannel,
                      const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return false;
}

bool CPVRTypeMW::Init()
{
  return false;
}

void CPVRTypeMW::Deinit()
{
  return;
}

std::shared_ptr<IPVRRadioSwitcher> CPVRTypeMW::GetRadioSwitcher()
{
  return nullptr;
}

} // namespace INSTANCE
} // namespace RTLRADIO
