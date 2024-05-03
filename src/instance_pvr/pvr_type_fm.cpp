/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr_type_fm.h"

namespace RTLRADIO
{
namespace INSTANCE
{

bool CPVRTypeFM::ScanChannels(
    std::vector<ChannelProps> channelsFound,
    const std::function<bool()>&& funcScanCancelled,
    const std::function<void(unsigned int)>&& funcScanPercentage,
    const std::function<void(const std::string& channel)>&& funcScanChannel,
    const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  return true;
}

CPVRTypeFM::CPVRTypeFM(const std::shared_ptr<SETTINGS::CSettings>& settings,
                       std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
  : IPVRType(Modulation::FM, settings, deviceOutputBuffer)
{
  m_signalProps.filter = false; // Never apply the filter here
  m_signalProps.samplerate = 1600 KHz;
  m_signalProps.bandwidth = 220 KHz;
  m_signalProps.lowcut = -103 KHz;
  m_signalProps.highcut = 103 KHz;

  // Analog signals require a DC offset to be applied to prevent a natural
  // spike from occurring at the center frequency on many RTL-SDR devices
  m_signalProps.offset = (m_signalProps.samplerate / 4);
}

bool CPVRTypeFM::Scan(std::vector<ChannelProps>& channelsFound,
                      const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                      const std::function<bool()>&& funcScanCancelled,
                      const std::function<void(unsigned int)>&& funcScanPercentage,
                      const std::function<void(const std::string& channel)>&& funcScanChannel,
                      const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  return false;
}

bool CPVRTypeFM::Init()
{
  return false;
}

void CPVRTypeFM::Deinit()
{
  return;
}

std::shared_ptr<IPVRRadioSwitcher> CPVRTypeFM::GetRadioSwitcher()
{
  return nullptr;
}

} // namespace INSTANCE
} // namespace RTLRADIO
