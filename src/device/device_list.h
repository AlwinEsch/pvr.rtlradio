/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "device.h"
#include "settings/settings.h"
#include "utils/span.h"

#include <memory>

#include <kodi/AddonBase.h>

namespace RTLRADIO
{

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

namespace DEVICE
{

class CDeviceList
{
public:
  CDeviceList(std::shared_ptr<SETTINGS::CSettings> settings);

  void Refresh();
  std::mutex& GetMutexDeviceInfos() { return m_mutexDeviceInfos; }
  tcb::span<const DeviceInfo> GetDeviceList() const { return m_deviceInfos; }
  std::shared_ptr<CDevice> GetDevice(size_t index);

private:
  void SetSettingsChangeCallback(const std::string& id,
                                 const kodi::addon::CSettingValue& settingValue);

  std::vector<DeviceInfo> m_deviceInfos;
  std::shared_ptr<SETTINGS::CSettings> m_settings;
  SETTINGS::DeviceConnection m_deviceConnection;
  std::mutex m_mutexDeviceInfos;
  int m_callbackSettingsChangeID{-1};
};

} // namespace DEVICE
} // namespace RTLRADIO
