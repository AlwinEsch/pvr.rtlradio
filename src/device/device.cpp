/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "device.h"

#include "utils/log.h"

namespace RTLRADIO
{
namespace DEVICE
{

CDevice::CDevice(const DeviceInfo& info,
                 const uint32_t blockSize,
                 std::shared_ptr<SETTINGS::CSettings> settings)
  : m_info(info), m_blockSize(blockSize)
{
}

CDevice::~CDevice()
{
}

void CDevice::SetNearestGain(const float target_gain)
{
  float min_err = 10000.0f;
  float nearest_gain = 0.0f;
  for (const auto& gain : m_gainList)
  {
    const float err = std::abs(gain - target_gain);
    if (err < min_err)
    {
      min_err = err;
      nearest_gain = gain;
    }
  }
  SetGain(nearest_gain);
}

void CDevice::SetCenterFrequency(const uint32_t freq)
{
  m_isRunning = false;
  SetCenterFrequency("Manual", freq);
  m_isRunning = true;
}

void CDevice::OnData(tcb::span<const uint8_t> buf)
{
  if (!m_isRunning)
  {
    return;
  }

  if (m_callbackOn_Data == nullptr)
  {
    return;
  }

  const size_t total_bytes = m_callbackOn_Data(buf);
  if (total_bytes != buf.size() && m_isRunning)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Short write, samples lost, {}/{}, shutting down device!\n", total_bytes,
               buf.size());
    Close();
  }

  if (m_callbackOn_Data2)
    m_callbackOn_Data2(buf);
}

} // namespace DEVICE
} // namespace RTLRADIO
