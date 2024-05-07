/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "props.h"

#include <functional>
#include <map>
#include <mutex>

#include <kodi/AddonBase.h>

namespace RTLRADIO
{
namespace SETTINGS
{

// device_connection
//
// Defines the RTL-SDR device connection type
enum class DeviceConnection : int
{
  USB = 0, // Locally connected USB device
  RTLTCP = 1, // Device connected via rtl_tcp
};

// regioncode
//
// Defines the possible region codes
enum class RegionCode : uint8_t
{
  NOTSET = 0, // Unknown
  WORLD = 1, // FM
  NORTHAMERICA = 2, // FM/HD/WX
  EUROPE = 3, // FM/DAB
};

class ATTR_DLL_LOCAL CSettings
{
public:
  CSettings();
  ~CSettings();

  ADDON_STATUS LoadSettings();
  ADDON_STATUS SetSetting(const std::string& settingName,
                          const kodi::addon::CSettingValue& settingValue);

  DeviceConnection DeviceConnectionType() const;
  void SetDeviceConnectionType(DeviceConnection deviceConnection);

  unsigned int DeviceDefaultIndex() const;
  void SetDeviceDefaultIndex(unsigned int index);

  std::string DeviceConnectionTCPHost() const;
  void SetDeviceConnectionTCPHost(std::string host);

  unsigned int DeviceConnectionTCPPort() const;
  void SetDeviceConnectionTCPPort(unsigned int port);

  RegionCode GetRegionCode() const;
  void SetRegionCode(RegionCode region);

  unsigned int GetEnabledModulationQty() const;
  void GetEnabledModulationTypes(std::vector<Modulation>& types) const;

  bool ModulationMWEnabled() const;
  void SetModulationMWEnabled(bool enabled);

  bool ModulationFMEnabled() const;
  void SetModulationFMEnabled(bool enabled);

  bool ModulationDABEnabled() const;
  void SetModulationDABEnabled(bool enabled);

  bool ModulationHDEnabled() const;
  void SetModulationHDEnabled(bool enabled);

  bool ModulationWXEnabled() const;
  void SetModulationWXEnabled(bool enabled);

  unsigned int DeviceLastFrequency() const { return m_deviceLastFrequency; }
  void SetDeviceLastFrequency(unsigned int deviceLastFrequency);

  unsigned int TransmissionNode() const { return m_transmissionNode; }
  std::string TunerDefaultChannel() const { return m_tunerDefaultChannel; }
  bool TunerAutoGain() const { return m_tunerAutoGain; }
  float GetTunerManualGain() const { return m_tunerManualGain; }
  bool UseOFDMDisableCoarseFreq() const { return m_OFDMDisableCoarseFreq; }
  unsigned int GetOFDMTotalThreads() const { return m_OFDMTotalThreads; }
  size_t DataBlockSize() const { return m_dataBlockSize; }
  unsigned int GetRadioTotalThreads() const { return m_radioTotalThreads; }
  bool ScraperEnable() const { return m_scraperEnable; }
  std::string ScraperOutput() const { return m_scraperOutput; }
  bool ScraperDisableAuto() const { return m_scraperDisableAuto; }
  bool ScanAutoEnabled() const { return m_scanAutoEnabled; }
  time_t ScanIntervalTime() const { return m_scanIntervalTime; }

  template<typename F>
  int SetSettingsChangeCallback(std::vector<std::string> handledSettingValues, F&& func)
  {
    if (handledSettingValues.empty())
      return -1;

    auto lock = std::unique_lock(m_mutex);

    for (const auto value : handledSettingValues)
      m_callbackSettingsChange[value] =
          std::make_pair(m_nextCallbackSettingsChangeID, std::move(func));

    return m_nextCallbackSettingsChangeID++;
  }

  bool ClearSettingsChangeCallback(int callbackSettingsChangeID)
  {
    if (callbackSettingsChangeID < 0)
      return false;

    auto lock = std::unique_lock(m_mutex);

    for (auto it = m_callbackSettingsChange.begin(); it != m_callbackSettingsChange.end();)
    {
      if (it->second.first == callbackSettingsChangeID)
        it = m_callbackSettingsChange.erase(it);
      else
        ++it;
    }

    return true;
  }

private:
  std::map<
      std::string,
      std::pair<int, std::function<void(const std::string&, const kodi::addon::CSettingValue&)>>>
      m_callbackSettingsChange;
  int m_nextCallbackSettingsChangeID{0};
  mutable std::mutex m_mutex;

  bool m_modulationMWEnabled{false};
  bool m_modulationFMEnabled{false};
  bool m_modulationDABEnabled{true};
  bool m_modulationHDEnabled{false};
  bool m_modulationWXEnabled{false};

  RegionCode m_regionCode{RegionCode::EUROPE};
  unsigned int m_deviceLastFrequency{216928000};
  DeviceConnection m_deviceConnection{DeviceConnection::USB};
  unsigned int m_deviceDefaultIndex{0};
  std::string m_deviceConnection_TCPHost{"127.0.0.1"};
  unsigned int m_deviceConnection_TCPPort{1234};
  std::string m_tunerDefaultChannel{"11A"};
  unsigned int m_transmissionNode{1};
  bool m_tunerAutoGain{false};
  float m_tunerManualGain{19.0f};
  bool m_OFDMDisableCoarseFreq{false};
  unsigned int m_OFDMTotalThreads{1};
  size_t m_dataBlockSize{65536};
  unsigned int m_radioTotalThreads{1};
  bool m_scraperEnable{false};
  std::string m_scraperOutput{"data/scraper_tuner"};
  bool m_scraperDisableAuto{false};
  bool m_scanAutoEnabled{true};
  time_t m_scanIntervalTime{60 * 60 * 24};
};

} // namespace SETTINGS
} // namespace RTLRADIO
