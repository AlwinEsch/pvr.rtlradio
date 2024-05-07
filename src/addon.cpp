/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "addon.h"

#include "instance_inputstream/inputstream.h"
#include "instance_inputstream/inputstream_base.h"
#include "instance_pvr/pvr.h"
#include "settings/settings.h"
#include "utils/log.h"

namespace RTLRADIO
{

CAddon::CAddon()
{
  m_settings = std::make_shared<SETTINGS::CSettings>();
  m_inputstreamBase = std::make_unique<INSTANCE::CInputstreamBase>(m_settings);
}

CAddon::~CAddon() = default;

ADDON_STATUS CAddon::Create()
{
  m_settings->LoadSettings();

  return ADDON_STATUS_OK;
}

ADDON_STATUS CAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
                                    KODI_ADDON_INSTANCE_HDL& hdl)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__,
             "Addon instance creation for type {} (ID='{}')",
             kodi::addon::GetTypeName(instance.GetType()), instance.GetID());

  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    INSTANCE::CPVR* pvr = new INSTANCE::CPVR(instance, m_settings);
    if (!pvr->Init())
    {
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "PVR addon instance creation failed");
      return ADDON_STATUS_PERMANENT_FAILURE;
    }

    hdl = m_activePVRInstance = pvr;
    return ADDON_STATUS_OK;
  }
  else if (instance.IsType(ADDON_INSTANCE_INPUTSTREAM))
  {
    hdl = new INSTANCE::CInputstream(instance, *m_inputstreamBase.get());
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

void CAddon::DestroyInstance(const kodi::addon::IInstanceInfo& instance,
                             const KODI_ADDON_INSTANCE_HDL hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
    m_activePVRInstance = nullptr;
}

ADDON_STATUS CAddon::SetSetting(const std::string& settingName,
                                const kodi::addon::CSettingValue& settingValue)
{
  return m_settings->SetSetting(settingName, settingValue);
}

} // namespace RTLRADIO

ADDONCREATOR(RTLRADIO::CAddon)
