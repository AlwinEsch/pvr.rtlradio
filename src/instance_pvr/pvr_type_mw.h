/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "props.h"
#include "pvr_radio_switcher_i.h"
#include "pvr_type_i.h"

#include <vector>

namespace RTLRADIO
{
namespace INSTANCE
{

class CBasicRadioSwitcher;

class CPVRTypeMW : public IPVRType
{
public:
  static bool ScanChannels(std::vector<ChannelProps> channelsFound,
                           const std::function<bool()>&& funcScanCancelled,
                           const std::function<void(unsigned int)>&& funcScanPercentage,
                           const std::function<void(const std::string& channel)>&& funcScanChannel,
                           const std::function<void(const ChannelProps&)>&& funcScanChannelFound);

  CPVRTypeMW(const std::shared_ptr<SETTINGS::CSettings>& settings,
             std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer);

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
};

} // namespace INSTANCE
} // namespace RTLRADIO
