/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "settings.h"

namespace RTLRADIO
{
namespace SETTINGS
{

CSettings::CSettings()
{
}

CSettings::~CSettings()
{
}

ADDON_STATUS CSettings::LoadSettings()
{

  return ADDON_STATUS_OK;
}

ADDON_STATUS CSettings::SetSetting(const std::string& settingName,
                                   const kodi::addon::CSettingValue& settingValue)
{
  //auto lock = std::unique_lock(m_mutex);

  fprintf(stderr, "SetSetting %s %s\n", settingName.c_str(), settingValue.GetString().c_str());
  return ADDON_STATUS_OK;
}

DeviceConnection CSettings::DeviceConnectionType() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_deviceConnection;
}

void CSettings::SetDeviceConnectionType(DeviceConnection deviceConnection)
{
  auto lock = std::unique_lock(m_mutex);

  if (deviceConnection != m_deviceConnection)
  {
    kodi::addon::SetSettingEnum<DeviceConnection>("device_connection", deviceConnection);
    m_deviceConnection = deviceConnection;
  }
}

RegionCode CSettings::GetRegionCode() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_regionCode;
}

void CSettings::SetRegionCode(RegionCode region)
{
  auto lock = std::unique_lock(m_mutex);

  if (region != m_regionCode)
  {
    kodi::addon::SetSettingEnum<RegionCode>("region_regioncode", region);
    m_regionCode = region;
  }
}

unsigned int CSettings::DeviceDefaultIndex() const
{
  auto lock = std::unique_lock(m_mutex);

  return (m_deviceConnection == DeviceConnection::USB) ? m_deviceDefaultIndex : 0;
}

void CSettings::SetDeviceDefaultIndex(unsigned int index)
{
  auto lock = std::unique_lock(m_mutex);

  if (index != m_deviceDefaultIndex)
  {
    kodi::addon::SetSettingInt("device_connection_usb_index", index);
    m_deviceDefaultIndex = index;
  }
}

std::string CSettings::DeviceConnectionTCPHost() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_deviceConnection_TCPHost;
}

void CSettings::SetDeviceConnectionTCPHost(std::string host)
{
  auto lock = std::unique_lock(m_mutex);

  if (host != m_deviceConnection_TCPHost)
  {
    kodi::addon::SetSettingString("device_connection_tcp_host", host);
    m_deviceConnection_TCPHost = host;
  }
}

unsigned int CSettings::DeviceConnectionTCPPort() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_deviceConnection_TCPPort;
}

void CSettings::SetDeviceConnectionTCPPort(unsigned int port)
{
  auto lock = std::unique_lock(m_mutex);

  if (port != m_deviceConnection_TCPPort)
  {
    kodi::addon::SetSettingBoolean("device_connection_tcp_port", port);
    m_deviceConnection_TCPPort = port;
  }
}

unsigned int CSettings::GetEnabledModulationQty() const
{
  auto lock = std::unique_lock(m_mutex);

  unsigned int qty = 0;
  qty += m_modulationMWEnabled;
  qty += m_modulationFMEnabled;
  qty += m_modulationDABEnabled;
  qty += m_modulationHDEnabled;
  qty += m_modulationWXEnabled;
  return qty;
}

void CSettings::GetEnabledModulationTypes(std::vector<Modulation>& types) const
{
  auto lock = std::unique_lock(m_mutex);

  if (m_modulationMWEnabled)
    types.push_back(Modulation::MW);
  if (m_modulationFMEnabled)
    types.push_back(Modulation::FM);
  if (m_modulationDABEnabled)
    types.push_back(Modulation::DAB);
  if (m_modulationHDEnabled)
    types.push_back(Modulation::HD);
  if (m_modulationWXEnabled)
    types.push_back(Modulation::WX);
}

bool CSettings::ModulationMWEnabled() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_modulationMWEnabled;
}

void CSettings::SetModulationMWEnabled(bool enabled)
{
  auto lock = std::unique_lock(m_mutex);

  if (enabled != m_modulationMWEnabled)
  {
    kodi::addon::SetSettingBoolean("mwradio_enable", enabled);
    m_modulationMWEnabled = enabled;
  }
}

bool CSettings::ModulationFMEnabled() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_modulationFMEnabled;
}

void CSettings::SetModulationFMEnabled(bool enabled)
{
  auto lock = std::unique_lock(m_mutex);

  if (enabled != m_modulationFMEnabled)
  {
    kodi::addon::SetSettingBoolean("fmradio_enable", enabled);
    m_modulationFMEnabled = enabled;
  }
}

bool CSettings::ModulationDABEnabled() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_modulationDABEnabled;
}

void CSettings::SetModulationDABEnabled(bool enabled)
{
  auto lock = std::unique_lock(m_mutex);

  if (enabled != m_modulationDABEnabled)
  {
    kodi::addon::SetSettingBoolean("dabradio_enable", enabled);
    m_modulationDABEnabled = enabled;
  }
}

bool CSettings::ModulationHDEnabled() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_modulationHDEnabled;
}

void CSettings::SetModulationHDEnabled(bool enabled)
{
  auto lock = std::unique_lock(m_mutex);

  if (enabled != m_modulationHDEnabled)
  {
    kodi::addon::SetSettingBoolean("hdradio_enable", enabled);
    m_modulationHDEnabled = enabled;
  }
}

bool CSettings::ModulationWXEnabled() const
{
  auto lock = std::unique_lock(m_mutex);

  return m_modulationWXEnabled;
}

void CSettings::SetModulationWXEnabled(bool enabled)
{
  auto lock = std::unique_lock(m_mutex);

  if (enabled != m_modulationWXEnabled)
  {
    kodi::addon::SetSettingBoolean("wxradio_enable", enabled);
    m_modulationWXEnabled = enabled;
  }
}

void CSettings::SetDeviceLastFrequency(unsigned int deviceLastFrequency)
{
  auto lock = std::unique_lock(m_mutex);

  if (deviceLastFrequency != m_deviceLastFrequency)
  {
    kodi::addon::SetSettingInt("device_last_freq", deviceLastFrequency);
    m_deviceLastFrequency = deviceLastFrequency;
  }
}

} // namespace SETTINGS
} // namespace RTLRADIO
