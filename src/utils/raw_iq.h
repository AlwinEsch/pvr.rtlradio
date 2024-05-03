/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <complex>
#include <cstdint>

namespace RTLRADIO
{
namespace UTILS
{

struct RawIQ
{
  uint8_t I;
  uint8_t Q;
  inline std::complex<float> to_c32() const
  {
    constexpr float BIAS = 127.5f;
    return std::complex<float>(static_cast<float>(I) - BIAS, static_cast<float>(Q) - BIAS);
  };
};

} // namespace UTILS
} // namespace RTLRADIO
