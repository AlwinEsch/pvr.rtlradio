/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include <memory>
#include <string_view>

namespace RTLRADIO
{
namespace INSTANCE
{

class IPVRRadioSwitcher
{
public:
  IPVRRadioSwitcher() = default;

  virtual void SwitchInstance(std::string_view key, const uint32_t freq) = 0;
  virtual void flush_input_stream() = 0;
};

} // namespace INSTANCE
} // namespace RTLRADIO
