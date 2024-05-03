/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "frequency_name.h"

#include "dsp_dab/block_frequencies.h"

#include <fmt/format.h>
#include <kodi/General.h>

#define KHz *(1000)
#define MHz *(1000000)

namespace RTLRADIO
{
namespace UTILS
{

void GetFrequencyName(uint32_t freq,
                      std::string& short_name,
                      std::string& user_name,
                      RTLRADIO::SETTINGS::RegionCode regioncode)
{
  using RTLRADIO::SETTINGS::RegionCode;

  if (freq >= 530 KHz && freq <= 1720 KHz)
  {
    short_name = "MW";
    user_name = kodi::addon::GetLocalizedString(30020);
    return;
  }

  if (regioncode == RegionCode::EUROPE)
  {
    if ((freq >= 47 MHz && freq <= 68 MHz) || (freq >= 174 MHz && freq <= 240 MHz) ||
        (freq >= 1452 MHz && freq <= 1491.5 MHz))
    {
      auto it = std::find_if(block_frequencies.begin(), block_frequencies.end(),
                             [freq](const auto& entry) { return entry.freq == freq; });
      if (it != block_frequencies.end())
      {
        short_name = it->name;
        user_name = fmt::format(kodi::addon::GetLocalizedString(30022), it->name);
        return;
      }

      short_name = "Unknown";
      user_name = kodi::addon::GetLocalizedString(30023);
      return;
    }
  }

  if (regioncode == RegionCode::EUROPE)
  {
    if (freq >= 87.5 MHz && freq <= 108 MHz)
    {
      short_name = "FM";
      user_name = kodi::addon::GetLocalizedString(30021);
      return;
    }
  }
  else if (regioncode == RegionCode::NORTHAMERICA)
  {
    if (freq >= 87.5 MHz && freq <= 107.9 MHz)
    {
      short_name = "FM";
      user_name = kodi::addon::GetLocalizedString(30021);
      return;
    }
  }
  else
  {
    if (freq >= 76 MHz && freq <= 108 MHz)
    {
      short_name = "FM";
      user_name = kodi::addon::GetLocalizedString(30021);
      return;
    }

    if (freq >= 65.9 MHz && freq <= 73.1 MHz)
    {
      short_name = "FM";
      user_name = kodi::addon::GetLocalizedString(30021);
      return;
    }
  }

  short_name = "Unknown";
  user_name = kodi::addon::GetLocalizedString(30023);
}

} // namespace UTILS
} // namespace RTLRADIO
