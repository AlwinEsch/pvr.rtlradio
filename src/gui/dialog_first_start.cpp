/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "dialog_first_start.h"

#include "device/device_list.h"
#include "dsp_dab/block_frequencies.h"
#include "instance_pvr/signalmeter.h"
#include "settings/settings.h"
#include "utils/log.h"

#include <kodi/gui/dialogs/FileBrowser.h>
#include <kodi/gui/dialogs/YesNo.h>

namespace RTLRADIO
{
namespace GUI
{

std::shared_ptr<CDialogFirstStart> CDialogFirstStart::Create(
    const std::shared_ptr<SETTINGS::CSettings>& settings)
{
  return std::make_shared<CDialogFirstStart>(settings);
}

CDialogFirstStart::CDialogFirstStart(const std::shared_ptr<SETTINGS::CSettings>& settings)
  : kodi::gui::CWindow("firststartdialog.xml", "skin.estuary", true, true), m_settings(settings)
{
  m_deviceConnection = m_settings->DeviceConnectionType();
  m_deviceDefaultIndex = m_settings->DeviceDefaultIndex();
  m_deviceConnection_TCPHost = m_settings->DeviceConnectionTCPHost();
  m_deviceConnection_TCPPort = m_settings->DeviceConnectionTCPPort();
  m_regionCode = m_settings->GetRegionCode();
  m_MWEnabled = m_settings->ModulationMWEnabled();
  m_FMEnabled = m_settings->ModulationFMEnabled();
  m_DABEnabled = m_settings->ModulationDABEnabled();
  m_HDEnabled = m_settings->ModulationHDEnabled();
  m_WXEnabled = m_settings->ModulationWXEnabled();
}

bool CDialogFirstStart::OnInit()
{
  using namespace kodi::gui::controls;
  using namespace RTLRADIO::SETTINGS;

  auto lock = std::unique_lock(m_mutex);

  m_buttonNext = std::unique_ptr<CButton>(new CButton(this, BUTTON_NEXT));
  m_buttonFinish = std::unique_ptr<CButton>(new CButton(this, BUTTON_FINISH));
  m_buttonBack = std::unique_ptr<CButton>(new CButton(this, BUTTON_BACK));

  m_spinDeviceConnect = std::unique_ptr<CSpin>(new CSpin(this, SPIN_DEVICE_CONNECT));
  m_spinDeviceConnect->SetType(ADDON_SPIN_CONTROL_TYPE_TEXT);
  m_spinDeviceConnect->AddLabel(kodi::addon::GetLocalizedString(30200), 0);
  m_spinDeviceConnect->AddLabel(kodi::addon::GetLocalizedString(30201), 1);
  m_spinDeviceConnect->SetIntValue(int(m_deviceConnection));

  m_spinDeviceConnectUSBIndex =
      std::unique_ptr<CSpin>(new CSpin(this, SPIN_DEVICE_CONNECT_USB_INDEX));
  m_spinDeviceConnectUSBIndex->SetType(ADDON_SPIN_CONTROL_TYPE_TEXT);
  for (const auto& info : m_deviceInfos)
  {
    m_spinDeviceConnectUSBIndex->AddLabel(fmt::format("{}: {}", info.index, info.name), info.index);
  }
  m_spinDeviceConnectUSBIndex->SetIntValue(m_deviceDefaultIndex);
  m_spinDeviceConnectUSBIndex->SetVisible(m_deviceConnection == DeviceConnection::USB);

  m_editDeviceConnectTCPHost =
      std::unique_ptr<CEdit>(new CEdit(this, EDIT_DEVICE_CONNECT_TCP_HOST));
  m_editDeviceConnectTCPHost->SetText(m_deviceConnection_TCPHost);
  m_editDeviceConnectTCPHost->SetVisible(m_deviceConnection == DeviceConnection::RTLTCP);

  m_editDeviceConnectTCPPort =
      std::unique_ptr<CEdit>(new CEdit(this, EDIT_DEVICE_CONNECT_TCP_PORT));
  m_editDeviceConnectTCPPort->SetText(std::to_string(m_deviceConnection_TCPPort));
  m_editDeviceConnectTCPPort->SetInputType(ADDON_INPUT_TYPE_NUMBER,
                                           kodi::addon::GetLocalizedString(30102));
  m_editDeviceConnectTCPPort->SetVisible(m_deviceConnection == DeviceConnection::RTLTCP);

  m_spinRegioncode = std::unique_ptr<CSpin>(new CSpin(this, SPIN_REGIONCODE));
  m_spinRegioncode->SetType(ADDON_SPIN_CONTROL_TYPE_TEXT);
  m_spinRegioncode->AddLabel(kodi::addon::GetLocalizedString(30219), 0);
  m_spinRegioncode->AddLabel(kodi::addon::GetLocalizedString(30221), 2);
  m_spinRegioncode->AddLabel(kodi::addon::GetLocalizedString(30222), 3);
  m_spinRegioncode->AddLabel(kodi::addon::GetLocalizedString(30220), 1);
  m_spinRegioncode->SetIntValue(int(m_regionCode));

  m_spinMWEnabled = std::unique_ptr<CRadioButton>(new CRadioButton(this, RADIO_BUTTON_MW_ENABLED));
  m_spinMWEnabled->SetVisible(true);
  m_spinMWEnabled->SetSelected(m_MWEnabled);

  m_spinFMEnabled = std::unique_ptr<CRadioButton>(new CRadioButton(this, RADIO_BUTTON_FM_ENABLED));
  m_spinFMEnabled->SetVisible(true);
  m_spinFMEnabled->SetSelected(m_FMEnabled);

  m_spinDABEnabled =
      std::unique_ptr<CRadioButton>(new CRadioButton(this, RADIO_BUTTON_DAB_ENABLED));
  m_spinDABEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                               m_regionCode == RegionCode::EUROPE);
  m_spinDABEnabled->SetSelected(m_DABEnabled);

  m_spinHDEnabled = std::unique_ptr<CRadioButton>(new CRadioButton(this, RADIO_BUTTON_HD_ENABLED));
  m_spinHDEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                              m_regionCode == RegionCode::NORTHAMERICA);
  m_spinHDEnabled->SetSelected(m_HDEnabled);

  m_spinWXEnabled = std::unique_ptr<CRadioButton>(new CRadioButton(this, RADIO_BUTTON_WX_ENABLED));
  m_spinWXEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                              m_regionCode == RegionCode::NORTHAMERICA);
  m_spinWXEnabled->SetSelected(m_WXEnabled);

  m_progressChannelscan = std::unique_ptr<CProgress>(new CProgress(this, PROGRESS_CHANNELSCAN));
  m_progressChannelscan->SetPercentage(0);

  m_editSignalMeterPower = std::unique_ptr<CEdit>(new CEdit(this, EDIT_SIGNAL_METER_POWER));
  m_editSignalMeterPower->SetText("0.0 dB");

  m_editSignalMeterNoise = std::unique_ptr<CEdit>(new CEdit(this, EDIT_SIGNAL_METER_NOISE));
  m_editSignalMeterNoise->SetText("0.0 dB");

  m_editSignalMeterSNR = std::unique_ptr<CEdit>(new CEdit(this, EDIT_SIGNAL_METER_SNR));
  m_editSignalMeterSNR->SetText("0.0 dB");

  m_channelsCount = std::unique_ptr<CEdit>(new CEdit(this, EDIT_CHANNELS_QTY));

  m_labelProcessName = std::unique_ptr<CLabel>(new CLabel(this, LABEL_PROCESS_NAME));
  m_labelProcessName->SetLabel(kodi::addon::GetLocalizedString(30122));

  m_labelSignalStatus = std::unique_ptr<CLabel>(new CLabel(this, LABEL_SIGNAL_STATUS));
  m_labelSignalStatus->SetLabel(kodi::addon::GetLocalizedString(30127));

  m_processPercentage = std::unique_ptr<CLabel>(new CLabel(this, LABEL_PROCESS_PERCENT));
  m_processPercentage->SetLabel(fmt::format("{} %", 0));

  m_renderSignalMeter =
      std::unique_ptr<CFFTSignalMeterControl>(new CFFTSignalMeterControl(this, RENDER_SIGNALMETER));

  m_buttonChannelEnabled = std::unique_ptr<CRadioButton>(new CRadioButton(this, BUTTON_ENABLE));
  m_editChannelName = std::unique_ptr<CEdit>(new CEdit(this, EDIT_CHANNELNAME));
  m_editChannelPicture = std::unique_ptr<CEdit>(new CEdit(this, EDIT_CHANNELICON));

  // Initialize the signal meter plot properties based on the size of the render control
  m_signalPlotProps.height = m_renderSignalMeter->height();
  m_signalPlotProps.width = m_renderSignalMeter->width();
  m_signalPlotProps.mindb = CFFTSignalMeterControl::FFT_MINDB;
  m_signalPlotProps.maxdb = CFFTSignalMeterControl::FFT_MAXDB;

  ClearList();

  m_currentDialogView = GROUP_1_USER_INFO;

  return kodi::gui::CWindow::OnInit();
}

bool CDialogFirstStart::OnAction(const kodi::gui::input::CAction& action)
{
  using namespace RTLRADIO::SETTINGS;

  //UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Action Id. {} given", int(action.GetID()));

  switch (action.GetID())
  {
    case ADDON_ACTION_NOOP:
      if (GetFocusId() == EDIT_CHANNELNAME)
      {
        kodi::gui::CWindow::OnAction(action);

        UpdateChannelName();
        return true;
      }
      break;
    case ADDON_ACTION_MOVE_UP:
    case ADDON_ACTION_MOVE_DOWN:
    case ADDON_ACTION_MOUSE_MOVE:
      if (GetFocusId() == LIST_CHANNELS)
      {
        kodi::gui::CWindow::OnAction(action);

        int position = GetCurrentListPosition();
        if (position != m_lastListPosition)
        {
          assert(position < m_listItems.size());
          std::shared_ptr<kodi::gui::CListItem> item = m_listItems[position];

          m_buttonChannelEnabled->SetSelected(item->GetProperty("visible") == "true");
          m_editChannelName->SetText(item->GetLabel());
          m_editChannelPicture->SetText(item->GetArt("icon"));
          m_lastListPosition = position;
        }
        return true;
      }
      break;
    case ADDON_ACTION_MOUSE_LEFT_CLICK:
    case ADDON_ACTION_SELECT_ITEM:
      if (GetFocusId() == EDIT_CHANNELICON)
      {
        UpdateChannelIcon();

        return true;
      }
      else if (GetFocusId() == EDIT_CHANNELNAME)
      {
        kodi::gui::CWindow::OnAction(action);

        UpdateChannelName();
        return true;
      }
      else if (GetFocusId() == BUTTON_ENABLE)
      {
        kodi::gui::CWindow::OnAction(action);

        UpdateChannelEnabled();
        return true;
      }
      break;
    case ADDON_ACTION_PREVIOUS_MENU:
      if (!MenuGoBack())
      {
        if (CancelButtonPress())
          Close();
      }
      return true;

    case ADDON_ACTION_NAV_BACK:
      MenuGoBack();
      return true;
    default:
      break;
  }

  return kodi::gui::CWindow::OnAction(action);
}

bool CDialogFirstStart::OnClick(int controlId)
{
  using namespace RTLRADIO::SETTINGS;

  //UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Control Id. {} clicked", controlId);

  switch (controlId)
  {
    case BUTTON_NEXT:
      MenuGoForward();
      return true;
    case BUTTON_FINISH:
      m_finished = true;
      m_scanFinished = true;
      Close();
      return true;
    case BUTTON_BACK:
      MenuGoBack();
      return true;
    case BUTTON_CANCEL:
      if (CancelButtonPress())
        Close();
      return true;
    case EDIT_CHANNELICON:
      UpdateChannelIcon();
      return true;
    case SPIN_DEVICE_CONNECT:
      m_deviceConnection = DeviceConnection(m_spinDeviceConnect->GetIntValue());
      m_spinDeviceConnectUSBIndex->SetVisible(m_deviceConnection == DeviceConnection::USB);
      m_editDeviceConnectTCPHost->SetVisible(m_deviceConnection == DeviceConnection::RTLTCP);
      m_editDeviceConnectTCPPort->SetVisible(m_deviceConnection == DeviceConnection::RTLTCP);
      return true;
    case SPIN_DEVICE_CONNECT_USB_INDEX:
      m_deviceDefaultIndex = m_spinDeviceConnectUSBIndex->GetIntValue();
      return true;
    case EDIT_DEVICE_CONNECT_TCP_HOST:
      m_deviceConnection_TCPHost = m_editDeviceConnectTCPHost->GetText();
      return true;
    case EDIT_DEVICE_CONNECT_TCP_PORT:
      m_deviceConnection_TCPPort = atoi(m_editDeviceConnectTCPPort->GetText().c_str());
      return true;
    case SPIN_REGIONCODE:
      m_regionCode = RegionCode(m_spinRegioncode->GetIntValue());
      m_spinDABEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                                   m_regionCode == RegionCode::EUROPE);
      m_spinHDEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                                  m_regionCode == RegionCode::NORTHAMERICA);
      m_spinWXEnabled->SetVisible(m_regionCode == RegionCode::NOTSET ||
                                  m_regionCode == RegionCode::NORTHAMERICA);
      return true;
    case RADIO_BUTTON_MW_ENABLED:
      m_MWEnabled = m_spinMWEnabled->IsSelected();
      return true;
    case RADIO_BUTTON_FM_ENABLED:
      m_FMEnabled = m_spinFMEnabled->IsSelected();
      return true;
    case RADIO_BUTTON_DAB_ENABLED:
      m_DABEnabled = m_spinDABEnabled->IsSelected();
      return true;
    case RADIO_BUTTON_HD_ENABLED:
      m_HDEnabled = m_spinHDEnabled->IsSelected();
      return true;
    case RADIO_BUTTON_WX_ENABLED:
      m_WXEnabled = m_spinWXEnabled->IsSelected();
      return true;
  }

  return kodi::gui::CWindow::OnClick(controlId);
}

bool CDialogFirstStart::MenuGoForward()
{
  if (m_currentDialogView == GROUP_1_USER_INFO)
  {
    SetControlVisible(GROUP_1_USER_INFO, false);
    SetControlVisible(GROUP_2_SETTINGS, true);
    SetControlVisible(BUTTON_BACK, true);
    m_currentDialogView = GROUP_2_SETTINGS;
    return true;
  }
  else if (m_currentDialogView == GROUP_2_SETTINGS)
  {
    SetControlVisible(GROUP_2_SETTINGS, false);
    SetControlVisible(GROUP_3_SCAN, true);
    m_currentDialogView = GROUP_3_SCAN;

    m_buttonBack->SetEnabled(false);
    m_buttonNext->SetEnabled(false);
    SetControlVisible(GROUP_SCAN_LIST, true);

    // Reset all available to one found
    for (auto& entry : m_listItems)
    {
      entry->SetProperty("amount_found", std::to_string(1));
    }

    SaveSettings();
    return true;
  }
  else if (m_currentDialogView == GROUP_3_SCAN)
  {
    SetControlVisible(GROUP_3_SCAN, false);
    SetControlVisible(GROUP_4_FINISH, true);
    SetControlVisible(BUTTON_NEXT, false);
    SetControlVisible(BUTTON_FINISH, true);
    m_currentDialogView = GROUP_4_FINISH;

    if (!m_listItems.empty())
    {
      int position = GetCurrentListPosition();
      assert(position < m_listItems.size());

      std::shared_ptr<kodi::gui::CListItem> item = m_listItems[position];

      m_buttonChannelEnabled->SetSelected(item->GetProperty("visible") == "true");
      m_editChannelName->SetText(item->GetLabel());
      m_editChannelPicture->SetText(item->GetArt("icon"));
    }

    return true;
  }

  return false;
}

bool CDialogFirstStart::MenuGoBack()
{
  if (m_currentDialogView == GROUP_1_USER_INFO)
  {
    return false;
  }
  else if (m_currentDialogView == GROUP_2_SETTINGS)
  {
    SetControlVisible(GROUP_2_SETTINGS, false);
    SetControlVisible(GROUP_1_USER_INFO, true);
    SetControlVisible(BUTTON_BACK, false);
    m_currentDialogView = GROUP_1_USER_INFO;

    SaveSettings();
    return true;
  }
  else if (m_currentDialogView == GROUP_3_SCAN)
  {
    SetControlVisible(GROUP_3_SCAN, false);
    SetControlVisible(GROUP_2_SETTINGS, true);
    m_currentDialogView = GROUP_2_SETTINGS;
    m_scanFinished = false;
    SetControlVisible(GROUP_SCAN_LIST, false);
    return true;
  }
  else if (m_currentDialogView == GROUP_4_FINISH)
  {
    SetControlVisible(GROUP_4_FINISH, false);
    SetControlVisible(GROUP_3_SCAN, true);
    SetControlVisible(BUTTON_NEXT, true);
    SetControlVisible(BUTTON_FINISH, false);
    m_currentDialogView = GROUP_3_SCAN;

    return true;
  }

  return false;
}

bool CDialogFirstStart::CancelButtonPress()
{
  bool canceled = false;
  const bool ret = kodi::gui::dialogs::YesNo::ShowAndGetInput(
      kodi::addon::GetLocalizedString(30600), kodi::addon::GetLocalizedString(30606), canceled);
  if (ret)
  {
    m_canceled = true;
    Close();
  }
  return m_canceled;
}

void CDialogFirstStart::SaveSettings()
{
  using namespace RTLRADIO::SETTINGS;

  m_settings->SetDeviceConnectionType(m_deviceConnection);
  m_settings->SetRegionCode(m_regionCode);
  m_settings->SetDeviceDefaultIndex(m_deviceDefaultIndex);
  m_settings->SetDeviceConnectionTCPHost(m_deviceConnection_TCPHost);
  m_settings->SetDeviceConnectionTCPPort(m_deviceConnection_TCPPort);
  m_settings->SetModulationMWEnabled(m_MWEnabled);
  m_settings->SetModulationFMEnabled(m_FMEnabled);
  m_settings->SetModulationDABEnabled(
      m_DABEnabled && (m_regionCode == RegionCode::NOTSET || m_regionCode == RegionCode::EUROPE));
  m_settings->SetModulationHDEnabled(m_HDEnabled && (m_regionCode == RegionCode::NOTSET ||
                                                     m_regionCode == RegionCode::NORTHAMERICA));
  m_settings->SetModulationWXEnabled(m_WXEnabled && (m_regionCode == RegionCode::NOTSET ||
                                                     m_regionCode == RegionCode::NORTHAMERICA));
}

void CDialogFirstStart::ScanChannelFound(const ChannelProps& props)
{
  if (props.transportmode != TransportMode::STREAM_MODE_AUDIO)
    return;

  // Check exact same channel already in and ignore if.
  {
    auto it = std::find_if(m_channelsFound.begin(), m_channelsFound.end(),
                           [props](const auto& listProps) { return props == listProps.second; });
    if (it != m_channelsFound.end())
      return;
  }

  // Check same channel already in and but with different frequency or modulation.
  {
    auto it = std::find_if(
        m_channelsFound.begin(), m_channelsFound.end(), [props](const auto& listProps) {
          const auto& entry = listProps.second;
          if (props.name != entry.name)
            return false;
          if (props.subchannelnumber > 0 && entry.subchannelnumber > 0 &&
              props.subchannelnumber != entry.subchannelnumber)
            return false;
          if (!props.country.empty() && !entry.country.empty() && props.country != entry.country)
            return false;
          if (!props.language.empty() && !entry.language.empty() &&
              props.language != entry.language)
            return false;
          return true;
        });
    if (it != m_channelsFound.end())
    {
      auto item = std::find_if(m_listItems.begin(), m_listItems.end(), [it](const auto& item) {
        return item.get()->GetProperty("number") == std::to_string(it->first);
      });
      if (item != m_listItems.end())
      {
        const int amount_found = atoi(item->get()->GetProperty("amount_found").c_str()) + 1;
        item->get()->SetProperty("amount_found", std::to_string(amount_found));
      }
      return;
    }
  }

  size_t nextNo = m_listItems.size();

  std::map<ProgrammType, int> genre_names = {{ProgrammType::NONE, 29940},
                                             {ProgrammType::NEWS, 29941},
                                             {ProgrammType::CURRENT_AFFAIRS, 29942},
                                             {ProgrammType::INFORMATION, 29943},
                                             {ProgrammType::SPORT, 29944},
                                             {ProgrammType::EDUCATION, 29945},
                                             {ProgrammType::DRAMA, 29946},
                                             {ProgrammType::ARTS, 29947},
                                             {ProgrammType::SCIENCE, 29948},
                                             {ProgrammType::TALK, 29949},
                                             {ProgrammType::POP_MUSIC, 29950},
                                             {ProgrammType::ROCK_MUSIC, 29951},
                                             {ProgrammType::EASY_LISTENING, 29952},
                                             {ProgrammType::LIGHT_CLASSICAL, 29953},
                                             {ProgrammType::CLASSICAL_MUSIC, 29954},
                                             {ProgrammType::MUSIC, 29955},
                                             {ProgrammType::WEATHER, 29956},
                                             {ProgrammType::FINANCE, 29957},
                                             {ProgrammType::CHILDREN, 29958},
                                             {ProgrammType::FACTUAL, 29959},
                                             {ProgrammType::RELIGION, 29960},
                                             {ProgrammType::PHONE_IN, 29961},
                                             {ProgrammType::TRAVEL, 29962},
                                             {ProgrammType::LEISURE, 29963},
                                             {ProgrammType::JAZZ_AND_BLUES, 29964},
                                             {ProgrammType::COUNTRY_MUSIC, 29965},
                                             {ProgrammType::NATIONAL_MUSIC, 29966},
                                             {ProgrammType::OLDIES_MUSIC, 29967},
                                             {ProgrammType::FOLK_MUSIC, 29968},
                                             {ProgrammType::DOCUMENTARY, 29969}};

  const std::string id = std::to_string(nextNo);

  std::shared_ptr<kodi::gui::CListItem> item = std::make_shared<kodi::gui::CListItem>();
  item->SetLabel(props.name);
  item->SetLabel2(props.provider);
  item->SetArt("icon", props.userlogourl);
  item->SetProperty("number", id);
  item->SetProperty("amount_found", std::to_string(1));

  item->SetProperty("provider", props.provider);
  if (size_t(props.programmtype) < genre_names.size())
    item->SetProperty("programmtype",
                      kodi::addon::GetLocalizedString(genre_names[props.programmtype]));
  if (props.modulation == Modulation::MW)
  {
    item->SetProperty("modulation", "MW");
    item->SetProperty("frequency",
                      fmt::format(kodi::addon::GetLocalizedString(30140), props.frequency / 1000));
  }
  else if (props.modulation == Modulation::FM)
  {
    item->SetProperty("modulation", "FM");
    item->SetProperty("frequency", fmt::format(kodi::addon::GetLocalizedString(30141),
                                               float(props.frequency) / 1000.0f / 1000.0f));
  }
  else if (props.modulation == Modulation::DAB)
  {
    std::string block = "?";
    auto it = std::find_if(block_frequencies.begin(), block_frequencies.end(),
                           [props](const auto& entry) { return props.frequency == entry.freq; });
    if (it != block_frequencies.end())
      block = it->name;

    item->SetProperty("modulation", "DAB/DAB+");
    item->SetProperty("frequency", fmt::format(kodi::addon::GetLocalizedString(30142), block,
                                               float(props.frequency) / 1000.0f / 1000.0f));
  }
  else if (props.modulation == Modulation::HD)
  {
    item->SetProperty("modulation", "HD");
    item->SetProperty("frequency", fmt::format(kodi::addon::GetLocalizedString(30141),
                                               float(props.frequency) / 1000.0f / 1000.0f));
  }
  else if (props.modulation == Modulation::WX)
  {
    item->SetProperty("modulation", "WX");
    item->SetProperty("frequency", fmt::format(kodi::addon::GetLocalizedString(30141),
                                               float(props.frequency) / 1000.0f / 1000.0f));
  }

  item->SetProperty("visible",
                    props.transportmode == TransportMode::STREAM_MODE_AUDIO ? "true" : "false");

  m_listItems.emplace_back(item);
  std::sort(m_listItems.begin(), m_listItems.end(), [](const auto& a, const auto& b) {
    return (a->GetLabel().compare(b->GetLabel()) < 0);
  });

  auto it = std::find_if(m_listItems.begin(), m_listItems.end(),
                         [id](std::shared_ptr<kodi::gui::CListItem>& item) {
                           return item->GetProperty("number") == id;
                         });

  AddListItem(item, it - m_listItems.begin());

  m_channelsFound.emplace_back(nextNo, props);

  m_channelsCount->SetText(fmt::format(kodi::addon::GetLocalizedString(30128), m_listItems.size()));
}

void CDialogFirstStart::ScanPercentage(unsigned int percent)
{
  m_progressChannelscan->SetPercentage(float(percent));
  m_processPercentage->SetLabel(fmt::format("{} %", percent));
}

void CDialogFirstStart::ScanModulation(Modulation modulation)
{
  std::string type;
  if (modulation == Modulation::MW)
  {
    type = "MW";
  }
  else if (modulation == Modulation::FM)
  {
    type = "FM";
  }
  else if (modulation == Modulation::DAB)
  {
    type = "DAB/DAB+";
  }
  else if (modulation == Modulation::HD)
  {
    type = "HD";
  }
  else if (modulation == Modulation::WX)
  {
    type = "WX";
  }

  if (!type.empty())
    m_labelProcessName->SetLabel(
        fmt::format("{} - {}", kodi::addon::GetLocalizedString(30122), type));
  else
    m_labelProcessName->SetLabel(kodi::addon::GetLocalizedString(30122));

  m_labelSignalStatus->SetLabel(kodi::addon::GetLocalizedString(30127));
}

void CDialogFirstStart::ScanChannel(const std::string& channel)
{
  m_labelSignalStatus->SetLabel(fmt::format(kodi::addon::GetLocalizedString(30129), channel));
}

void CDialogFirstStart::ScanFinished()
{
  m_buttonBack->SetEnabled(true);
  m_buttonNext->SetEnabled(true);
  m_labelProcessName->SetLabel(kodi::addon::GetLocalizedString(30130));
  m_scanFinished = true;
}

void CDialogFirstStart::UpdateChannelEnabled()
{
  int position = GetCurrentListPosition();
  assert(position < m_listItems.size());

  const bool selected = m_buttonChannelEnabled->IsSelected();

  std::shared_ptr<kodi::gui::CListItem> item = m_listItems[position];
  item->SetProperty("visible", selected ? "true" : "false");

  const int number = atoi(item->GetProperty("number").c_str());
  auto& props = m_channelsFound[number].second;
  props.visible = selected;
}

void CDialogFirstStart::UpdateChannelName()
{
  int position = GetCurrentListPosition();
  assert(position < m_listItems.size());

  std::shared_ptr<kodi::gui::CListItem> item = m_listItems[position];

  const auto name = m_editChannelName->GetText();
  if (name.empty())
  {
    m_editChannelName->SetText(item->GetLabel());
    return;
  }

  const int number = atoi(item->GetProperty("number").c_str());
  auto& props = m_channelsFound[number].second;
  props.usereditname = number;

  item->SetLabel(name);
}

void CDialogFirstStart::UpdateChannelIcon()
{
  int position = GetCurrentListPosition();
  assert(position < m_listItems.size());

  std::shared_ptr<kodi::gui::CListItem> item = m_listItems[position];

  const int number = atoi(item->GetProperty("number").c_str());
  auto& props = m_channelsFound[number].second;

  std::string path = props.userlogourl;
  bool ret = kodi::gui::dialogs::FileBrowser::ShowAndGetImage(
      "local|network|removable", kodi::addon::GetLocalizedString(30136), path);
  if (!ret || path.empty() || !kodi::vfs::FileExists(path))
    return;

  m_editChannelPicture->SetText(path);

  item->SetArt("icon", path);

  props.userlogourl = path;
}

void CDialogFirstStart::SetAvailableDeviceInfos(const tcb::span<const DEVICE::DeviceInfo>& infos)
{
  using namespace kodi::gui::controls;
  using namespace RTLRADIO::SETTINGS;

  auto lock = std::unique_lock(m_mutex);

  m_deviceInfos.clear();
  for (const auto& info : infos)
    m_deviceInfos.emplace_back(info);

  if (m_spinDeviceConnectUSBIndex)
  {
    m_spinDeviceConnectUSBIndex->SetType(ADDON_SPIN_CONTROL_TYPE_TEXT);
    for (const auto& info : m_deviceInfos)
      m_spinDeviceConnectUSBIndex->AddLabel(fmt::format("{} - {}", info.index, info.name),
                                            info.index);
    m_spinDeviceConnectUSBIndex->SetIntValue(m_deviceDefaultIndex);
    m_spinDeviceConnectUSBIndex->SetVisible(m_deviceConnection == DeviceConnection::USB);
  }
}

void CDialogFirstStart::MeterStatus(struct SignalStatus const& status)
{
  bool signallock = false; // Signal lock
  bool muxlock = false; // Multiplex lock
  char strbuf[64] = {}; // snprintf() text buffer

  // Signal Meter
  //
  m_renderSignalMeter->Update(status, signallock, muxlock);

  // Signal Strength
  //
  if (!std::isnan(status.power))
  {

    snprintf(strbuf, std::extent<decltype(strbuf)>::value,
             kodi::addon::GetLocalizedString(30335).c_str(), // %.1f dB
             status.power);
    m_editSignalMeterPower->SetText(strbuf);
  }
  else
    m_editSignalMeterPower->SetText(kodi::addon::GetLocalizedString(10006)); // "N/A"

  if (!std::isnan(status.noise))
  {

    snprintf(strbuf, std::extent<decltype(strbuf)>::value,
             kodi::addon::GetLocalizedString(30335).c_str(), // %.1f dB
             status.noise);
    m_editSignalMeterNoise->SetText(strbuf);
  }
  else
    m_editSignalMeterNoise->SetText(kodi::addon::GetLocalizedString(10006)); // "N/A"

  // Signal-to-noise
  //
  if (!std::isnan(status.snr))
  {

    snprintf(strbuf, std::extent<decltype(strbuf)>::value,
             kodi::addon::GetLocalizedString(30336).c_str(), // %d dB
             static_cast<int>(status.snr));
    m_editSignalMeterSNR->SetText(strbuf);
  }
  else
    m_editSignalMeterSNR->SetText(kodi::addon::GetLocalizedString(10006)); // "N/A"
}

void CDialogFirstStart::GetChannelsFound(std::vector<ChannelProps>& channelsFound)
{
  channelsFound.clear();
  channelsFound.reserve(m_channelsFound.size());
  for (const auto& channel : m_channelsFound)
    channelsFound.emplace_back(channel.second);
}

void CDialogFirstStart::GetChannelsEdited(std::vector<ChannelProps>& channelsEdited)
{
  channelsEdited.clear();
  for (const auto& channel : m_channelsFound)
  {
    if (channel.second.usereditname != channel.second.name ||
        channel.second.userlogourl != channel.second.logourl ||
        (channel.second.transportmode == TransportMode::STREAM_MODE_AUDIO &&
         !channel.second.visible))
      channelsEdited.emplace_back(channel.second);
  }
}

} // namespace GUI
} // namespace RTLRADIO