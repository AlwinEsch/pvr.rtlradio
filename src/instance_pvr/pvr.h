/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "dsp_dab/app_ofdm_blocks.h"
#include "pvr_type_i.h"
#include "utils/app_io_buffers.h"
#include "utils/source_location.hpp"

#include <memory>
#include <thread>

#include <kodi/addon-instance/PVR.h>

class AudioPipeline;
class OFDM_Block;
class sqlite_exception;

namespace RTLRADIO
{

namespace DEVICE
{
class CDevice;
class CDeviceList;
} // namespace DEVICE

namespace GUI
{
class CDialogFirstStart;
} // namespace GUI

namespace SETTINGS
{

class CSettings;

namespace DB
{
class CConPool;
} // namespace DB

} // namespace SETTINGS

namespace INSTANCE
{

class CDeviceSource;
class CBasicRadioSwitcher;
class CSignalMeter;

class IDeviceSource
{
public:
  IDeviceSource() = default;
  virtual std::shared_ptr<DEVICE::CDevice> get_device() = 0;
  virtual void set_device(std::shared_ptr<DEVICE::CDevice> device) = 0;
};

class ATTR_DLL_LOCAL CPVR : public kodi::addon::CInstancePVRClient
{
public:
  CPVR(const kodi::addon::IInstanceInfo& instance, std::shared_ptr<SETTINGS::CSettings> settings);
  ~CPVR();

  bool Init();
  void Close();

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR CallSettingsMenuHook(const kodi::addon::PVRMenuhook& menuhook) override;

  PVR_ERROR GetProvidersAmount(int& amount) override;
  PVR_ERROR GetProviders(kodi::addon::PVRProvidersResultSet& results) override;

  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                   kodi::addon::PVRChannelGroupMembersResultSet& results) override;

  PVR_ERROR GetChannelsAmount(int& amount);
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  PVR_ERROR GetChannelStreamProperties(
      const kodi::addon::PVRChannel& channel,
      std::vector<kodi::addon::PVRStreamProperty>& properties) override;
  PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override;

  PVR_ERROR DeleteChannel(const kodi::addon::PVRChannel& channel) override;
  PVR_ERROR RenameChannel(const kodi::addon::PVRChannel& channel) override;
  PVR_ERROR OpenDialogChannelSettings(const kodi::addon::PVRChannel& channel) override;
  PVR_ERROR OpenDialogChannelAdd(const kodi::addon::PVRChannel& channel) override;
  PVR_ERROR OpenDialogChannelScan() override;
  PVR_ERROR CallChannelMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                const kodi::addon::PVRChannel& item) override;

  PVR_ERROR GetEPGForChannel(int channelUid,
                             time_t start,
                             time_t end,
                             kodi::addon::PVREPGTagsResultSet& results) override;
  PVR_ERROR SetEPGMaxPastDays(int pastDays) override;
  PVR_ERROR SetEPGMaxFutureDays(int futureDays) override;

  PVR_ERROR OnSystemSleep() override;
  PVR_ERROR OnSystemWake() override;
  PVR_ERROR OnPowerSavingActivated() override;
  PVR_ERROR OnPowerSavingDeactivated() override;

private:
  bool InitDatabase();
  bool ProcessChannelScan();
  bool SetupRTLSDRDevice();
  bool StartRTLSDRDevice();

  void TestDatabase();

  void SetSettingsChangeCallback(const std::string& id,
                                 const kodi::addon::CSettingValue& settingValue);

  // Exception Helpers
  //
  void HandleGeneralException(const nostd::source_location location);
  template<typename _result>
  _result HandleGeneralException(const nostd::source_location location, _result result);
  void HandleStdException(const nostd::source_location location, std::exception const& ex);
  template<typename _result>
  _result HandleStdException(const nostd::source_location location,
                             std::exception const& ex,
                             _result result);
  void HandleDBException(const nostd::source_location location, sqlite_exception const& dbex);
  template<typename _result>
  _result HandleDBException(const nostd::source_location location,
                            sqlite_exception const& dbex,
                            _result result);

  std::shared_ptr<IDeviceSource> m_deviceSource;

  std::shared_ptr<SETTINGS::CSettings> m_settings;
  std::shared_ptr<DEVICE::CDeviceList> m_deviceList;
  std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> m_deviceOutputBuffer;
  //std::shared_ptr<CBasicRadioSwitcher> m_radioSwitcher;
  //std::shared_ptr<OFDM_Block> m_OFDMBlock;
  //std::shared_ptr<ThreadedRingBuffer<viterbi_bit_t>> m_OFDMToRadioBuffer;
  std::shared_ptr<AudioPipeline> m_audioPipeline;
  std::unique_ptr<std::thread> m_threadSelectDefaultTuner{nullptr};
  std::unique_ptr<std::thread> m_threadOFDMRun{nullptr};
  std::unique_ptr<std::thread> m_threadRadioSwitcher{nullptr};
  int m_callbackSettingsChangeID{-1};

  std::shared_ptr<SETTINGS::DB::CConPool> m_connpool;
  std::unique_ptr<std::thread> m_channelscanThread = nullptr;
  std::atomic<bool> m_channelscanThreadRunning = false;
  std::atomic<bool> m_channelscanSignalmeterSet = false;
  unsigned int m_scansDone = 0;
  bool m_scanWithGUI = false;

  std::shared_ptr<IPVRType> m_activePVRType;
  std::shared_ptr<CSignalMeter> m_activeSignalmeter;
};

} // namespace INSTANCE
} // namespace RTLRADIO
