/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr.h"

#include "audio/audio_pipeline.h"
#include "device/device_list.h"
#include "exception_control/sqlite_exception.h"
#include "exception_control/string_exception.h"
#include "gui/dialog_first_start.h"
#include "pvr_type_dab.h"
#include "pvr_type_fm.h"
#include "pvr_type_hd.h"
#include "pvr_type_mw.h"
#include "pvr_type_wx.h"
#include "settings/database.h"
#include "settings/settings.h"
#include "utils/log.h"

#include <chrono>
#include <functional>
#include <thread>

namespace RTLRADIO
{
namespace INSTANCE
{

void CPVR::TestDatabase()
{
  using namespace SETTINGS::DB;

  try
  {
    CConPool::handle dbhandle(m_connpool);

    const int amount = CConPoolPVR::GetChannelsCount(dbhandle);
    UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "GetChannelsCount: {}", amount);

    auto callback = [&](const struct ChannelProps& item) -> void {
      kodi::addon::PVRChannel channel;

      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "GetChannels: {}", item.usereditname);
    };

    CConPoolPVR::GetChannels(dbhandle, Modulation::DAB, false, callback);
  }
  catch (sqlite_exception const& dbex)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Unable to access the database {} - {} (Source: {}:{})",
               kodi::addon::GetUserPath(m_connpool->GetBaseDBName()), dbex.what(),
               dbex.location().file_name(), dbex.location().line());
  }
}

CPVR::CPVR(const kodi::addon::IInstanceInfo& instance,
           std::shared_ptr<SETTINGS::CSettings> settings)
  : kodi::addon::CInstancePVRClient(instance), m_settings(settings)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Addon instance created");

  const std::vector<std::string> usedSettingValues = {"todo"};

  m_callbackSettingsChangeID = m_settings->SetSettingsChangeCallback(
      usedSettingValues,
      [&](const std::string& id, const kodi::addon::CSettingValue& settingValue) {
        SetSettingsChangeCallback(id, settingValue);
      });
}

CPVR::~CPVR()
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Addon instance destroyed");

  m_settings->ClearSettingsChangeCallback(m_callbackSettingsChangeID);

  Close();
}

bool CPVR::Init()
{
  if (m_settings->DataBlockSize() == 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Data block size cannot be zero");
    return false;
  }

  if (!InitDatabase())
    return false;

  if (!SetupRTLSDRDevice())
    return false;

  // Check an automatic channel scan needs be done
  if (m_settings->ScanAutoEnabled())
    ProcessChannelScan();

  TestDatabase();

  return true;

  m_audioPipeline = std::make_shared<AudioPipeline>();

  // TODO: Make this in a way to allow all types and not only DAB
  m_activePVRType = std::make_shared<CPVRTypeDAB>(m_settings, m_deviceOutputBuffer);

  if (m_activePVRType)
  {
    if (!m_activePVRType->Init())
    {
      return false;
    }
  }

  return true;
}

void CPVR::Close()
{
  if (m_channelscanThread)
  {
    if (m_channelscanThread->joinable())
      m_channelscanThread->join();
    m_channelscanThread.reset();
  }

  if (m_threadSelectDefaultTuner)
  {
    if (m_threadSelectDefaultTuner->joinable())
      m_threadSelectDefaultTuner->join();
    m_threadSelectDefaultTuner.reset();
  }

  m_deviceOutputBuffer.reset();
  m_audioPipeline = nullptr;
  m_deviceSource = nullptr;
  m_deviceList = nullptr;

  return;

  //m_OFDMToRadioBuffer.reset();
  //m_threadOFDMRun->join();
  //m_threadOFDMRun.reset();
  //m_threadRadioSwitcher->join();
  //m_threadRadioSwitcher.reset();
  //m_OFDMBlock = nullptr;
  //m_radioSwitcher = nullptr;
}

bool CPVR::InitDatabase()
{
  using namespace SETTINGS::DB;

  // Create the global database connection pool instance
  try
  {
    m_connpool =
        std::make_shared<CConPoolPVR>(kodi::addon::GetUserPath(), CONNECTIONPOOL_SIZE,
                                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI);
    m_connpool->InitDatabase();
  }
  catch (sqlite_exception const& dbex)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Unable to create/open the database {} - {}",
               kodi::addon::GetUserPath(m_connpool->GetBaseDBName()), dbex.what());
    return false;
  }

  return true;
}

// TODO: Not finished and not usable
bool CPVR::ProcessChannelScan()
{
  using std::chrono::system_clock;
  using namespace SETTINGS::DB;

  std::time_t currentTime;
  std::time_t lastScan;
  std::time_t sinceLastScan;
  int channelsCount;

  try
  {
    CConPool::handle dbhandle(m_connpool);

    currentTime = system_clock::to_time_t(system_clock::now());
    lastScan = CConPoolPVR::GetLastScanTime(dbhandle);
    sinceLastScan = currentTime - lastScan;
    channelsCount = CConPoolPVR::GetChannelsCount(dbhandle);
  }
  catch (sqlite_exception const& dbex)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Unable to get the channels database: {}",
               dbex.what());
    return false;
  }

  if (sinceLastScan > m_settings->ScanIntervalTime() || channelsCount == 0)
  {
    bool firstScan = lastScan == 0;
    bool withGUI = false;

    UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Starting channel scan (last scan: {})",
               !firstScan ? std::ctime(&lastScan) : "Is first scan");

    m_channelscanThreadRunning = true;
    m_channelscanThread = std::make_unique<std::thread>([&, firstScan]() {
      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Channelscan thread started");

      try
      {
        // Value "channelsFound" stores all found channels on scan to use later for storage in
        // database.
        std::vector<ChannelProps> channelsFound;

        // TODO check this works in all cases or e.g. this std::unique_ptr<GUI::CDialogFirstStart> m_firstStartDialog = nullptr; needed!
        std::shared_ptr<GUI::CDialogFirstStart> dialog = nullptr;
        if (firstScan)
        {
          UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__,
                     "Opening first start dialog about user set");
          dialog = GUI::CDialogFirstStart::Create(m_settings);
          dialog->Show();
          m_scanWithGUI = true;
        }

        while (m_channelscanThreadRunning)
        {
          m_deviceList->Refresh();
          size_t totalDevices = 0;

          // Load list of available RTL-SDR devices.
          // NOTE: If RTL-SDR over TCP is used then always only one device available
          {
            auto lock = std::unique_lock(m_deviceList->GetMutexDeviceInfos());
            auto deviceInfos = m_deviceList->GetDeviceList();
            totalDevices = deviceInfos.size();

            if (dialog)
              dialog->SetAvailableDeviceInfos(deviceInfos);
          }

          if (dialog)
          {
            while (m_channelscanThreadRunning)
            {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
              if (dialog->Canceled() || dialog->Finished())
              {
                m_channelscanThreadRunning = false;
                fprintf(stderr, "-----------------> 1 a\n");
                break;
              }

              if (dialog->m_currentDialogView == GUI::CDialogFirstStart::GROUP_1_USER_INFO)
                continue;
              if (dialog->m_currentDialogView == GUI::CDialogFirstStart::GROUP_2_SETTINGS)
                continue;
              if (dialog->m_currentDialogView == GUI::CDialogFirstStart::GROUP_3_SCAN)
              {
                if (dialog->m_scanFinished)
                  continue;

                break;
              }
              if (dialog->m_currentDialogView == GUI::CDialogFirstStart::GROUP_4_FINISH)
                continue;
            }

            if (!m_channelscanThreadRunning)
            {
              break;
            }

            UTILS::log(
                UTILS::LOGLevel::DEBUG, __src_loc_cur__,
                "First start dialog about user set done (dialog now continue with search list "
                "about channel scan)");
          }

          /**
         * @brief The wanted scan starting now from here, all the code before was used to set
         * wanted settings.
         */
          /**@{*/

          unsigned int scantypeCnt = 0;
          scantypeCnt += m_settings->ModulationDABEnabled() ? 1 : 0;
          scantypeCnt += m_settings->ModulationFMEnabled() ? 1 : 0;
          scantypeCnt += m_settings->ModulationHDEnabled() ? 1 : 0;
          scantypeCnt += m_settings->ModulationMWEnabled() ? 1 : 0;
          scantypeCnt += m_settings->ModulationWXEnabled() ? 1 : 0;

          /**
         * @brief This is a lambda callback function to let scanner function inform the RTL-SDR
         * system about his wanted scan frequency and to set to it.
         *
         * @param[in] freq The frequency to set in RTL-SDR
         */
          const auto funcSetCenterFrequency = [&](const uint32_t freq) {
            auto device = m_deviceSource->get_device();
            if (device)
              device->SetCenterFrequency(freq);
          };

          /**
         * @brief This is a lambda callback function to let scanner function scan is cancelled
         * about some reason.
         *
         * Mainly can come the cancel if GUI was active and User pressed "Cancel" button, further
         * can cancel come in cases of errors during scan.
         *
         * @return Boolean value if channel scan thread should be stopped and cancelled for some
         * reason.
         */
          const auto funcScanCancelled = [&, dialog]() {
            if (dialog && dialog->Canceled())
              m_channelscanThreadRunning = false;
            return !m_channelscanThreadRunning;
          };

          /**
         * @brief This is a lambda callback function to inform from scanner a channel was found.
         *
         * This mainly used inside GUI dialog (if used) to list the available channels to the user.
         *
         * @param[in] props The channel properties where found.
         */
          const auto funcScanChannelFound = [&, dialog](const ChannelProps& props) {
            UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__,
                       "Found channel {} on Frequency {} Hz with ID {}", props.name,
                       props.frequency, props.subchannelnumber);
            if (dialog)
              dialog->ScanChannelFound(props);
          };

          /**
         * @brief This is a lambda callback function to inform from scanner about current percent
         * of progress.
         *
         * The percent given to here is from one type progress, it becomes then divided by the
         * amount of types where scanned to have complete percent of all.
         *
         * This mainly used inside GUI dialog (if used) to show percent of scan.
         *
         * @param[in] percent The current running scan percent.
         */
          const auto funcScanPercentage = [&, dialog, scantypeCnt](unsigned int percent) {
            if (dialog)
              dialog->ScanPercentage((m_scansDone * 100 + percent) / scantypeCnt);
          };

          /**
         * @brief This is a lambda callback function to inform from scanner about current channel
         * to scan.
         *
         * This channel can be a Frequency or also a channel name e.g. like used on DAB channels.
         *
         * This mainly used inside GUI dialog (if used) to show percent of scan.
         *
         * @param[in] channel Name of channel currently scanned.
         */
          const auto funcScanChannel = [&, dialog](const std::string& channel) {
            if (dialog)
              dialog->ScanChannel(channel);
          };

          const auto funcMeterStatus = [&, dialog](struct SignalStatus const& status) {
            if (!dialog)
              return;
            dialog->MeterStatus(status);
          };

          // Setup the selected RTL-SDR device
          {
            // Check selected device index is higher as available devices.
            const size_t defaultDeviceIndex = m_settings->DeviceDefaultIndex();
            if (defaultDeviceIndex >= totalDevices)
            {
              UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
                         "Device index is greater than the number of devices ({} >= {})",
                         defaultDeviceIndex, totalDevices);
              /// @todo Not use log in case GUI is used and no return from here if GUI active.
              return;
            }

            // Get wanted device control class.
            // NOTE: Within this call becomes also the RTL-SDR device inited for use!
            auto device = m_deviceList->GetDevice(defaultDeviceIndex);
            if (device == nullptr)
            {
              /// @todo Add use in GUI case and not use return then
              return;
            }
            m_deviceSource->set_device(device);

            // Check scan cancelled in background for some reason (e.g. by User in a dialog).
            if (funcScanCancelled())
            {
              return;
            }

            // Do now the scans about any type where set for use inside add-on settings.
            //
            // If count = 0 then nothing todo on scans and goes to the end where exit the function or
            // in case GUI active to allow new select from user.
            if (scantypeCnt != 0)
            {
              m_scansDone = 0;
              funcScanPercentage(0);

              // Load now all type classes where scan needs performed
              std::vector<std::unique_ptr<IPVRType>> toScan;
              toScan.reserve(scantypeCnt);
              if (m_settings->ModulationDABEnabled())
                toScan.emplace_back(
                    std::make_unique<CPVRTypeDAB>(m_settings, m_deviceOutputBuffer));
              if (m_settings->ModulationFMEnabled())
                toScan.emplace_back(std::make_unique<CPVRTypeFM>(m_settings, m_deviceOutputBuffer));
              if (m_settings->ModulationHDEnabled())
                toScan.emplace_back(std::make_unique<CPVRTypeHD>(m_settings, m_deviceOutputBuffer));
              if (m_settings->ModulationMWEnabled())
                toScan.emplace_back(std::make_unique<CPVRTypeMW>(m_settings, m_deviceOutputBuffer));
              if (m_settings->ModulationWXEnabled())
                toScan.emplace_back(std::make_unique<CPVRTypeWX>(m_settings, m_deviceOutputBuffer));

              struct StructScanData
              {
                StructScanData(size_t blockSize) : buffer(blockSize * sizeof(UTILS::RawIQ), 0) {}

                std::atomic_bool running;
                std::vector<uint8_t> buffer;
                const struct SignalProps* props;
                const struct SignalPlotProps* plotProps;
              } scanData(m_settings->DataBlockSize());

              // Go now over all types where needs scanned.
              for (const auto& type : toScan)
              {
                scanData.running = true;
                std::unique_ptr<std::thread> signalThread;
                std::shared_ptr<ThreadedRingBuffer<uint8_t>> input;

                if (dialog)
                {
                  dialog->ScanModulation(type->GetModulationType());

                  input = std::make_shared<ThreadedRingBuffer<uint8_t>>(
                      m_settings->DataBlockSize() * sizeof(UTILS::RawIQ));

                  device->SetDataCallback2(
                      [input](tcb::span<const uint8_t> bytes) { return input->write(bytes); });

                  scanData.props = &type->GetSignalProps();
                  scanData.plotProps = &dialog->GetSignalPlotProps();

                  // Do this hack to use pointer and become atomic_bool available in other thread
                  StructScanData* scanDataPtr = &scanData;

                  signalThread = std::make_unique<std::thread>([&, scanDataPtr, funcMeterStatus]() {
                    auto signalmeter = std::make_unique<CSignalMeter>(
                        *scanDataPtr->props, *scanDataPtr->plotProps, 100, funcMeterStatus);

                    while (scanDataPtr->running)
                    {
                      const size_t length = input->read(scanDataPtr->buffer);
                      signalmeter->ProcessInputSamples(
                          tcb::span<const uint8_t>(scanDataPtr->buffer.data(), length));
                    }
                  });
                }

                type->Scan(channelsFound, funcSetCenterFrequency, funcScanCancelled,
                           funcScanPercentage, funcScanChannel, funcScanChannelFound);

                if (input)
                {
                  scanData.running = false;
                  auto device = m_deviceSource->get_device();
                  if (device)
                  {
                    device->ResetDataCallback2();
                    device->SetIsRunning(false);
                  }
                  input->close();
                }

                if (signalThread)
                {
                  signalThread->join();
                  signalThread.reset();
                }

                if (funcScanCancelled())
                {
                  return;
                }

                funcScanPercentage(100);
                m_scansDone++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
              }
            }

            // Reset selected device to have usable on normal use (outside of this scan)
            if (m_deviceSource->get_device() != nullptr)
            {
              m_deviceSource->set_device(nullptr);
            }
          }

          if (funcScanCancelled())
            return;

          if (dialog)
            dialog->ScanFinished();
          /**@}*/
        }

        if (dialog)
        {
          std::vector<ChannelProps> channelsEdited;
          dialog->GetChannelsEdited(channelsEdited);
          for (const auto& edited : channelsEdited)
          {
            auto it = std::find_if(channelsFound.begin(), channelsFound.end(),
                                   [edited](const auto& entry) { return edited == entry; });
            if (it != channelsFound.end())
            {
              it->visible = edited.visible;
              it->usereditname = edited.usereditname;
              it->userlogourl = edited.userlogourl;
            }
          }

          dialog->Close();
        }

        if (!firstScan || (dialog && !dialog->Canceled()))
        {
          try
          {
            CConPool::handle dbhandle(m_connpool);
            CConPoolPVR::ChannelScanSet(dbhandle, channelsFound);

            time_t currentTime = 0;
            time(&currentTime);
            CConPoolPVR::SetLastScanTime(dbhandle, currentTime, channelsFound.size());
          }
          catch (sqlite_exception const& dbex)
          {
            UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
                       "Unable to set the channels database: {}", dbex.what());
            return;
          }

          kodi::addon::CInstancePVRClient::TriggerChannelUpdate();
        }

        //m_deviceOutputBuffer->close();
        m_deviceSource->set_device(nullptr);
        m_channelscanThreadRunning = false;
      }
      catch (...)
      {
      }

      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Channelscan thread finished");
    });
  }

  return true;
}

bool CPVR::SetupRTLSDRDevice()
{
  class CDeviceSource : public IDeviceSource
  {
  public:
    CDeviceSource(std::function<void(std::shared_ptr<DEVICE::CDevice>)>&& deviceChangeCallback)
      : m_deviceChangeCallback(deviceChangeCallback)
    {
    }
    std::shared_ptr<DEVICE::CDevice> get_device() override
    {
      auto lock = std::unique_lock(m_mutex_device);
      return m_device;
    }
    void set_device(std::shared_ptr<DEVICE::CDevice> device) override
    {
      auto lock = std::unique_lock(m_mutex_device);
      m_device = device;
      m_deviceChangeCallback(m_device);
    }

  private:
    std::shared_ptr<DEVICE::CDevice> m_device = nullptr;
    std::mutex m_mutex_device;
    std::function<void(std::shared_ptr<DEVICE::CDevice>)> m_deviceChangeCallback;
  };

  //----------------------------------------------------------------------------

  m_deviceOutputBuffer = std::make_shared<ThreadedRingBuffer<UTILS::RawIQ>>(
      m_settings->DataBlockSize() * sizeof(UTILS::RawIQ));
  m_deviceList = std::make_shared<DEVICE::CDeviceList>(m_settings);
  m_deviceSource = std::make_shared<CDeviceSource>([&](std::shared_ptr<DEVICE::CDevice> device) {
    if (m_activePVRType)
      m_activePVRType->GetRadioSwitcher()->flush_input_stream();

    if (device == nullptr)
      return;

    if (m_settings->TunerAutoGain())
    {
      device->SetAutoGain();
    }
    else
    {
      device->SetNearestGain(m_settings->GetTunerManualGain());
    }

    device->SetDataCallback([&](tcb::span<const uint8_t> bytes) {
      constexpr size_t BYTES_PER_SAMPLE = sizeof(UTILS::RawIQ);
      const size_t total_bytes = bytes.size() - (bytes.size() % BYTES_PER_SAMPLE);
      const size_t total_samples = total_bytes / BYTES_PER_SAMPLE;
      auto raw_iq = tcb::span(reinterpret_cast<const UTILS::RawIQ*>(bytes.data()), total_samples);
      const size_t total_read_samples = m_deviceOutputBuffer->write(raw_iq);
      const size_t total_read_bytes = total_read_samples * BYTES_PER_SAMPLE;
      return total_read_bytes;
    });

    device->SetFrequencyChangeCallback([&](const std::string& label, const uint32_t freq) {
      if (m_activePVRType)
        m_activePVRType->GetRadioSwitcher()->SwitchInstance(label, freq);
    });

    device->SetCenterFrequency(m_settings->DeviceLastFrequency());
  });

  return true;
}

bool CPVR::StartRTLSDRDevice()
{
  const size_t defaultDeviceIndex = m_settings->DeviceDefaultIndex();
  m_threadSelectDefaultTuner = std::make_unique<std::thread>([&, defaultDeviceIndex]() {
    m_deviceList->Refresh();
    size_t totalDevices = 0;
    {
      auto lock = std::unique_lock(m_deviceList->GetMutexDeviceInfos());
      auto deviceInfos = m_deviceList->GetDeviceList();
      totalDevices = deviceInfos.size();
    }
    if (defaultDeviceIndex >= totalDevices)
    {
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
                 "Device index is greater than the number of devices ({} >= {})",
                 defaultDeviceIndex, totalDevices);
      return;
    }
    auto device = m_deviceList->GetDevice(defaultDeviceIndex);
    if (device == nullptr)
      return;
    m_deviceSource->set_device(device);
  });

  return true;
}

void CPVR::HandleGeneralException(const nostd::source_location location)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Failed due to an exception");
}

template<typename _result>
_result CPVR::HandleGeneralException(const nostd::source_location location, _result result)
{
  HandleGeneralException(location);
  return result;
}

void CPVR::HandleStdException(const nostd::source_location location, std::exception const& ex)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Failed due to an exception: {}", ex.what());
}

template<typename _result>
_result CPVR::HandleStdException(const nostd::source_location location,
                                 std::exception const& ex,
                                 _result result)
{
  HandleStdException(location, ex);
  return result;
}

void CPVR::HandleDBException(const nostd::source_location location, sqlite_exception const& dbex)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Database error: {} - Source: {}({},{})",
             dbex.what(), dbex.location().file_name(), dbex.location().line(),
             dbex.location().column());
}

template<typename _result>
_result CPVR::HandleDBException(const nostd::source_location location,
                                sqlite_exception const& dbex,
                                _result result)
{
  HandleDBException(location, dbex);
  return result;
}

PVR_ERROR CPVR::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsRadio(true);
  capabilities.SetSupportsProviders(true);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsChannelScan(true);
  capabilities.SetSupportsChannelSettings(true);
  capabilities.SetSupportsEPG(true);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetBackendName(std::string& name)
{
  name.assign(VERSION_PRODUCTNAME_ANSI);

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetBackendVersion(std::string& version)
{
  version.assign(STR(PVRRTLRADIO_VERSION));

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::CallSettingsMenuHook(const kodi::addon::PVRMenuhook& menuhook)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetProvidersAmount(int& amount)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  amount = 1;
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetProviders(kodi::addon::PVRProvidersResultSet& results)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  kodi::addon::PVRProvider provider;
  provider.SetUniqueId(1);
  provider.SetName("DAB/DAB+");
  provider.SetType(PVR_PROVIDER_TYPE_OTHER);
  results.Add(provider);

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannelGroupsAmount(int& amount)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  try
  {
    amount = m_settings->GetEnabledModulationQty();
  }
  catch (std::exception& ex)
  {
    return HandleStdException(__src_loc_cur__, ex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (...)
  {
    return HandleGeneralException(__src_loc_cur__, PVR_ERROR::PVR_ERROR_FAILED);
  }

  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Amount returned: {}", amount);

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  // Don't do if channel scan is active
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  try
  {
    // Get list of in settings enabled types
    std::vector<Modulation> types;
    m_settings->GetEnabledModulationTypes(types);

    // Codes 30450 - 30454 from strings.po are used
    constexpr const uint32_t langCodeStart = 30450;

    // Go over all from settings given enabled modulation types
    for (const auto& type : types)
    {
      kodi::addon::PVRChannelGroup group;
      group.SetGroupName(kodi::addon::GetLocalizedString(langCodeStart + uint32_t(type)));
      group.SetIsRadio(true);
      group.SetPosition(uint32_t(type));
      results.Add(group);

      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Channel group added: Pos.: {}, Name: {}",
                 group.GetPosition(), group.GetGroupName());
    }
  }
  catch (std::exception& ex)
  {
    return HandleStdException(__src_loc_cur__, ex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (...)
  {
    return HandleGeneralException(__src_loc_cur__, PVR_ERROR::PVR_ERROR_FAILED);
  }

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                       kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  kodi::addon::PVRChannelGroupMember kodiGroupMember;
  kodiGroupMember.SetGroupName(group.GetGroupName());
  kodiGroupMember.SetChannelUniqueId(1);
  kodiGroupMember.SetChannelNumber(1);
  kodiGroupMember.SetSubChannelNumber(1);

  results.Add(kodiGroupMember);

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannelsAmount(int& amount)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  using namespace SETTINGS::DB;

  try
  {
    amount = CConPoolPVR::GetChannelsCount(CConPool::handle(m_connpool));
  }
  catch (sqlite_exception const& dbex)
  {
    return HandleDBException(__src_loc_cur__, dbex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (std::exception& ex)
  {
    return HandleStdException(__src_loc_cur__, ex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (...)
  {
    return HandleGeneralException(__src_loc_cur__, PVR_ERROR::PVR_ERROR_FAILED);
  }

  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Amount given: {}", amount);

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  // The PVR only supports radio channels
  if (!radio)
    return PVR_ERROR::PVR_ERROR_NO_ERROR;

  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  using namespace SETTINGS::DB;

  try
  {
    auto callback = [&](const struct ChannelProps& item) -> void {
      kodi::addon::PVRChannel channel;

      channel.SetUniqueId(item.id.Id());
      channel.SetIsRadio(true);
      channel.SetChannelNumber(item.channelnumber);
      channel.SetSubChannelNumber(item.subchannelnumber);
      channel.SetIsHidden(!item.visible);
      if (!item.usereditname.empty())
        channel.SetChannelName(item.usereditname);
      else if (!item.name.empty())
        channel.SetChannelName(item.name);
      if (!item.userlogourl.empty())
        channel.SetIconPath(item.userlogourl);
      else if (!item.logourl.empty())
        channel.SetIconPath(item.logourl);
      //channel.SetMimeType(...);
      //channel.SetOrder(...);
      //channel.SetEncryptionSystem(...);
      //channel.SetHasArchive(...);
      //channel.SetClientProviderUid(...);

      results.Add(channel);

      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__,
                 "Channel added: {}/{}: {} (Hidden: {}, Unique Id: {:X})",
                 channel.GetChannelNumber(), channel.GetSubChannelNumber(),
                 channel.GetChannelName(), channel.GetIsHidden() ? "yes" : "no",
                 channel.GetUniqueId());
    };

    CConPool::handle dbhandle(m_connpool);

    if (m_settings->ModulationDABEnabled())
      CConPoolPVR::GetChannels(dbhandle, Modulation::DAB, false, callback);
    if (m_settings->ModulationFMEnabled())
      CConPoolPVR::GetChannels(dbhandle, Modulation::FM, false, callback);
    if (m_settings->ModulationHDEnabled())
      CConPoolPVR::GetChannels(dbhandle, Modulation::HD, false, callback);
    if (m_settings->ModulationMWEnabled())
      CConPoolPVR::GetChannels(dbhandle, Modulation::MW, false, callback);
    if (m_settings->ModulationWXEnabled())
      CConPoolPVR::GetChannels(dbhandle, Modulation::WX, false, callback);
  }
  catch (sqlite_exception const& dbex)
  {
    return HandleDBException(__src_loc_cur__, dbex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (std::exception& ex)
  {
    return HandleStdException(__src_loc_cur__, ex, PVR_ERROR::PVR_ERROR_FAILED);
  }
  catch (...)
  {
    return HandleGeneralException(__src_loc_cur__, PVR_ERROR::PVR_ERROR_FAILED);
  }

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
                                           std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "pvr.rtlradio");
  properties.emplace_back("pvr.rtlradio.channel", std::to_string(channel.GetUniqueId()));
  properties.emplace_back("pvr.rtlradio.provider", std::to_string(channel.GetClientProviderUid()));

  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::DeleteChannel(const kodi::addon::PVRChannel& channel)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::RenameChannel(const kodi::addon::PVRChannel& channel)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OpenDialogChannelSettings(const kodi::addon::PVRChannel& channel)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OpenDialogChannelAdd(const kodi::addon::PVRChannel& channel)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OpenDialogChannelScan()
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::CallChannelMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                    const kodi::addon::PVRChannel& item)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::GetEPGForChannel(int channelUid,
                                 time_t start,
                                 time_t end,
                                 kodi::addon::PVREPGTagsResultSet& results)
{
  if (m_channelscanThreadRunning)
    return PVR_ERROR::PVR_ERROR_REJECTED;
  ;

  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::SetEPGMaxPastDays(int pastDays)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::SetEPGMaxFutureDays(int futureDays)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OnSystemSleep()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OnSystemWake()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OnPowerSavingActivated()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

PVR_ERROR CPVR::OnPowerSavingDeactivated()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

void CPVR::SetSettingsChangeCallback(const std::string& id,
                                     const kodi::addon::CSettingValue& settingValue)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

} // namespace INSTANCE
} // namespace RTLRADIO
