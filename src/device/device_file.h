/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "device.h"
#include "utils/scalar_condition.h"

#include <vector>

namespace RTLRADIO
{
namespace DEVICE
{

class CDeviceFile : public CDevice
{
public:
  static std::vector<DeviceInfo> GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings);

  explicit CDeviceFile(const DeviceInfo& info,
                       const uint32_t blockSize,
                       std::shared_ptr<SETTINGS::CSettings> settings,
                       const std::string& filename,
                       uint32_t samplerate);
  ~CDeviceFile() override;

  bool Create() override;
  void Close() override;

  void SearchGains() override;
  void SetGain(const float gain) override;
  void SetAutoGain() override;
  void SetSamplingFrequency(const uint32_t freq) override;
  void SetCenterFrequency(const std::string& label, const uint32_t freq) override;

private:
  size_t read(uint8_t* buffer, size_t count) const;
  bool read_async();
  void cancel_async() const;

  std::string m_filename;
  uint32_t const m_samplerate;
  FILE* m_file = nullptr;

  mutable scalar_condition<bool> m_stop{false}; // Flag to stop async
  mutable scalar_condition<bool> m_stopped{true}; // Async stopped condition
};

} // namespace DEVICE
} // namespace RTLRADIO
