/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "props.h"
#include "signalmeter.h"
#include "utils/app_io_buffers.h"
#include "utils/raw_iq.h"

#include <functional>
#include <map>
#include <memory>
#include <string_view>

namespace RTLRADIO
{

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

namespace INSTANCE
{

class IPVRRadioSwitcher;

class IPVRType
{
public:
  IPVRType(Modulation modulationType,
           const std::shared_ptr<SETTINGS::CSettings>& settings,
           std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
    : m_settings(settings),
      m_deviceOutputBuffer(deviceOutputBuffer),
      m_modulationType(modulationType)
  {
  }

  virtual bool Scan(std::vector<ChannelProps>& channelsFound,
                    const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                    const std::function<bool()>&& funcScanCancelled,
                    const std::function<void(unsigned int)>&& funcScanPercentage,
                    const std::function<void(const std::string& channel)>&& funcScanChannel,
                    const std::function<void(const ChannelProps&)>&& funcScanChannelFound) = 0;

  virtual bool Init() = 0;
  virtual void Deinit() = 0;

  const struct SignalProps& GetSignalProps() const { return m_signalProps; }
  const Modulation GetModulationType() const { return m_modulationType; }

  virtual std::shared_ptr<IPVRRadioSwitcher> GetRadioSwitcher() = 0;

protected:
  std::shared_ptr<SETTINGS::CSettings> m_settings;
  std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> m_deviceOutputBuffer;
  struct SignalProps m_signalProps = {};

private:
  const Modulation m_modulationType;
};

} // namespace INSTANCE
} // namespace RTLRADIO
