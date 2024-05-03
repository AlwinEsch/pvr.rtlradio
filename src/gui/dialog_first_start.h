/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "control_signalmeter.h"
#include "instance_pvr/signalmeter.h"
#include "settings/settings.h"

#include <atomic>
#include <memory>
#include <mutex>

#include <kodi/gui/ListItem.h>
#include <kodi/gui/Window.h>
#include <kodi/gui/controls/Button.h>
#include <kodi/gui/controls/Edit.h>
#include <kodi/gui/controls/Label.h>
#include <kodi/gui/controls/Progress.h>
#include <kodi/gui/controls/RadioButton.h>
#include <kodi/gui/controls/Spin.h>

namespace RTLRADIO
{

struct ChannelProps;

namespace DEVICE
{
struct DeviceInfo;
}

namespace INSTANCE
{
class CSignalMeter;
}

namespace GUI
{

class CDialogFirstStart : public kodi::gui::CWindow
{
public:
  static std::shared_ptr<CDialogFirstStart> Create(
      const std::shared_ptr<SETTINGS::CSettings>& settings);

  CDialogFirstStart(const std::shared_ptr<SETTINGS::CSettings>& settings);

  void ScanChannelFound(const ChannelProps& props);
  void ScanPercentage(unsigned int percent);
  void ScanModulation(Modulation modulation);
  void ScanChannel(const std::string& channel);
  void ScanFinished();
  void SetAvailableDeviceInfos(const tcb::span<const DEVICE::DeviceInfo>& infos);
  void MeterStatus(struct SignalStatus const& status);

  bool Finished() const { return m_finished; }
  bool Canceled() const { return m_canceled; }
  const struct SignalPlotProps& GetSignalPlotProps() const { return m_signalPlotProps; }
  void GetChannelsFound(std::vector<ChannelProps>& channelsFound);
  void GetChannelsEdited(std::vector<ChannelProps>& channelsEdited);

  static constexpr int GROUP_1_USER_INFO = 2000;
  static constexpr int GROUP_2_SETTINGS = 2001;
  static constexpr int GROUP_3_SCAN = 2002;
  static constexpr int GROUP_4_FINISH = 2003;

  std::atomic_bool m_scanFinished = false;
  std::atomic_int m_currentDialogView = GROUP_1_USER_INFO;

private:
  static constexpr int BUTTON_ENABLE = 32;
  static constexpr int EDIT_CHANNELNAME = 33;
  static constexpr int EDIT_CHANNELICON = 34;

  static constexpr int EDIT_PROVIDER = 40;
  static constexpr int EDIT_MODULATION = 41;

  static constexpr int BUTTON_NEXT = 100;
  static constexpr int BUTTON_FINISH = 101;
  static constexpr int BUTTON_BACK = 102;
  static constexpr int BUTTON_CANCEL = 103;
  static constexpr int LABEL_PROCESS_NAME = 198;
  static constexpr int LABEL_SIGNAL_STATUS = 199;
  static constexpr int SPIN_DEVICE_CONNECT = 200;
  static constexpr int SPIN_DEVICE_CONNECT_USB_INDEX = 201;
  static constexpr int EDIT_DEVICE_CONNECT_TCP_HOST = 202;
  static constexpr int EDIT_DEVICE_CONNECT_TCP_PORT = 203;
  static constexpr int SPIN_REGIONCODE = 205;
  static constexpr int RADIO_BUTTON_MW_ENABLED = 206;
  static constexpr int RADIO_BUTTON_FM_ENABLED = 207;
  static constexpr int RADIO_BUTTON_DAB_ENABLED = 208;
  static constexpr int RADIO_BUTTON_HD_ENABLED = 209;
  static constexpr int RADIO_BUTTON_WX_ENABLED = 210;
  static constexpr int PROGRESS_CHANNELSCAN = 211;
  static constexpr int RENDER_SIGNALMETER = 212;
  static constexpr int EDIT_SIGNAL_METER_POWER = 213;
  static constexpr int EDIT_SIGNAL_METER_NOISE = 214;
  static constexpr int EDIT_SIGNAL_METER_SNR = 215;
  static constexpr int LIST_CHANNELS = 216;
  static constexpr int EDIT_CHANNELS_QTY = 217;
  static constexpr int LABEL_PROCESS_PERCENT = 218;

  static constexpr int GROUP_SCAN_LIST = 2010;

  bool OnInit() override;
  bool OnAction(const kodi::gui::input::CAction& action) override;
  bool OnClick(int controlId) override;

  bool MenuGoForward();
  bool MenuGoBack();
  bool CancelButtonPress();
  void SaveSettings();
  void UpdateChannelEnabled();
  void UpdateChannelName();
  void UpdateChannelIcon();

  std::shared_ptr<SETTINGS::CSettings> const m_settings;

  std::unique_ptr<kodi::gui::controls::CButton> m_buttonNext;
  std::unique_ptr<kodi::gui::controls::CButton> m_buttonFinish;
  std::unique_ptr<kodi::gui::controls::CButton> m_buttonBack;
  std::unique_ptr<kodi::gui::controls::CSpin> m_spinDeviceConnect;
  std::unique_ptr<kodi::gui::controls::CSpin> m_spinDeviceConnectUSBIndex;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editDeviceConnectTCPHost;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editDeviceConnectTCPPort;
  std::unique_ptr<kodi::gui::controls::CSpin> m_spinRegioncode;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_spinMWEnabled;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_spinFMEnabled;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_spinDABEnabled;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_spinHDEnabled;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_spinWXEnabled;
  std::unique_ptr<kodi::gui::controls::CProgress> m_progressChannelscan;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editSignalMeterPower;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editSignalMeterNoise;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editSignalMeterSNR;
  std::unique_ptr<kodi::gui::controls::CEdit> m_channelsCount;
  std::unique_ptr<kodi::gui::controls::CLabel> m_labelProcessName;
  std::unique_ptr<kodi::gui::controls::CLabel> m_labelSignalStatus;
  std::unique_ptr<kodi::gui::controls::CLabel> m_processPercentage;
  std::unique_ptr<kodi::gui::controls::CRadioButton> m_buttonChannelEnabled;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editChannelName;
  std::unique_ptr<kodi::gui::controls::CEdit> m_editChannelPicture;
  std::unique_ptr<CFFTSignalMeterControl> m_renderSignalMeter;

  bool m_canceled{false};
  bool m_finished{false};

  struct SignalPlotProps m_signalPlotProps = {}; // Signal properties
  int m_lastListPosition{-1};

  std::vector<std::pair<size_t, ChannelProps>> m_channelsFound;
  std::vector<std::shared_ptr<kodi::gui::CListItem>> m_listItems;
  std::vector<DEVICE::DeviceInfo> m_deviceInfos;

  std::mutex m_mutex;

  // Setting values
  SETTINGS::DeviceConnection m_deviceConnection;
  unsigned int m_deviceDefaultIndex;
  std::string m_deviceConnection_TCPHost;
  unsigned int m_deviceConnection_TCPPort;
  SETTINGS::RegionCode m_regionCode;
  bool m_MWEnabled;
  bool m_FMEnabled;
  bool m_DABEnabled;
  bool m_HDEnabled;
  bool m_WXEnabled;
};

} // namespace GUI
} // namespace RTLRADIO