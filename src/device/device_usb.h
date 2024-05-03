/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "device.h"

#include <vector>

struct rtlsdr_dev;

namespace RTLRADIO
{
namespace DEVICE
{

class CDeviceUSB : public CDevice
{
public:
  static std::vector<DeviceInfo> GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings);

  explicit CDeviceUSB(const DeviceInfo& info,
                      const uint32_t blockSize,
                      std::shared_ptr<SETTINGS::CSettings> settings);
  ~CDeviceUSB() override;

  bool Create() override;
  void Close() override;

  void SearchGains() override;
  void SetGain(const float gain) override;
  void SetAutoGain() override;
  void SetSamplingFrequency(const uint32_t freq) override;
  void SetCenterFrequency(const std::string& label, const uint32_t freq) override;

private:
  static void rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx);

  struct rtlsdr_dev* m_device{nullptr};
};

} // namespace DEVICE
} // namespace RTLRADIO
