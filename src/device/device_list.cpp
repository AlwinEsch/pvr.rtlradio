/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "device_list.h"

#include "device_file.h"
#include "device_tcp.h"
#ifdef USB_DEVICE_SUPPORT
#include "device_usb.h"
#endif
#include "utils/log.h"

namespace RTLRADIO
{
namespace DEVICE
{

CDeviceList::CDeviceList(std::shared_ptr<SETTINGS::CSettings> settings) : m_settings(settings)
{
  m_deviceConnection = settings->DeviceConnectionType();

  const std::vector<std::string> usedSettingValues = {"device_connection"};

  m_callbackSettingsChangeID = settings->SetSettingsChangeCallback(
      usedSettingValues,
      [&](const std::string& id, const kodi::addon::CSettingValue& settingValue) {
        SetSettingsChangeCallback(id, settingValue);
      });
}

void CDeviceList::Refresh()
{
  std::vector<DeviceInfo> infos;
  if (m_deviceConnection == SETTINGS::DeviceConnection::RTLTCP)
    infos = CDeviceTCP::GetDeviceList(m_settings);
#ifdef USB_DEVICE_SUPPORT
  else if (m_deviceConnection == SETTINGS::DeviceConnection::USB)
    infos = CDeviceUSB::GetDeviceList(m_settings);
#endif
  else
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Unsupported device type '{}' selected",
               int(m_deviceConnection));
    return;
  }

  if (infos.empty())
  {
    {
      auto lock = std::unique_lock(m_mutexDeviceInfos);
      m_deviceInfos.clear();
    }

    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "No devices were found");
    return;
  }

  auto lock = std::unique_lock(m_mutexDeviceInfos);
  m_deviceInfos = infos;
}

std::shared_ptr<CDevice> CDeviceList::GetDevice(size_t index)
{
  auto lock = std::unique_lock(m_mutexDeviceInfos);

  if (index >= m_deviceInfos.size())
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Device at index {} out of bounds", index);
    return nullptr;
  }

  const auto info = m_deviceInfos[index];
  lock.unlock();

  std::shared_ptr<CDevice> device;
  if (m_deviceConnection == SETTINGS::DeviceConnection::RTLTCP)
    device = std::make_shared<CDeviceTCP>(info, 4, m_settings);
#ifdef USB_DEVICE_SUPPORT
  else if (m_deviceConnection == SETTINGS::DeviceConnection::USB)
    device = std::make_shared<CDeviceUSB>(info, 4, m_settings);
#endif
  else
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Unsupported device type '{}' selected",
               int(m_deviceConnection));
    return nullptr;
  }

  if (!device->Create())
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to create RTL-SDR connection");
    return nullptr;
  }

  return device;
}

void CDeviceList::SetSettingsChangeCallback(const std::string& id,
                                            const kodi::addon::CSettingValue& settingValue)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Setting change - ID={}, VALUE='{}'", id,
             settingValue.GetString());

  if (id == "device_connection")
  {
    const SETTINGS::DeviceConnection value = settingValue.GetEnum<SETTINGS::DeviceConnection>();
    if (m_deviceConnection != value)
    {
      m_deviceConnection = value;
      Refresh();
    }
  }
}

} // namespace DEVICE
} // namespace RTLRADIO
