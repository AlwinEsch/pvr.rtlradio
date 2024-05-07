/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include <memory>

#include <kodi/AddonBase.h>

namespace RTLRADIO
{

namespace INSTANCE
{
class CPVR;
class CInputstreamBase;
} // namespace INSTANCE

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

class ATTR_DLL_LOCAL CAddon : public kodi::addon::CAddonBase
{
public:
  CAddon();
  ~CAddon();

  //! kodi::addon::CAddonBase functions
  //@{
  ADDON_STATUS Create() override;

  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override;
  void DestroyInstance(const kodi::addon::IInstanceInfo& instance,
                       const KODI_ADDON_INSTANCE_HDL hdl) override;

  ADDON_STATUS SetSetting(const std::string& settingName,
                          const kodi::addon::CSettingValue& settingValue) override;
  //@}

private:
  std::shared_ptr<SETTINGS::CSettings> m_settings;
  std::unique_ptr<INSTANCE::CInputstreamBase> m_inputstreamBase;
  INSTANCE::CPVR* m_activePVRInstance{nullptr};
};

} // namespace RTLRADIO
