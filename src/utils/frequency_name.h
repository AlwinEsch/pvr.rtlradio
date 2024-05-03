/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "settings/settings.h"

namespace RTLRADIO
{
namespace UTILS
{

void GetFrequencyName(uint32_t freq,
                      std::string& short_name,
                      std::string& user_name,
                      RTLRADIO::SETTINGS::RegionCode regioncode);

} // namespace UTILS
} // namespace RTLRADIO
