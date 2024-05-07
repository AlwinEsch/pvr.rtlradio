/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/span.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace RTLRADIO
{

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

enum class DeviceSource : int
{
  TCP,
  USB
};

namespace DEVICE
{

struct DeviceInfo
{
  DeviceSource source;
  uint32_t index;

  std::string name;
  std::string vendor;
  std::string product;
  std::string serial;
};

class CDevice
{
public:
  explicit CDevice(const DeviceInfo& info,
                   const uint32_t blockSize,
                   std::shared_ptr<SETTINGS::CSettings> settings);
  virtual ~CDevice();

  virtual bool Create() = 0;
  virtual void Close() = 0;
  bool IsRunning() const { return m_isRunning; }
  void SetIsRunning(bool running) { m_isRunning = running; }
  const DeviceInfo& GetDeviceInfo() { return m_info; } // GetDescriptor
  int GetBlockSize() const { return m_blockSize; }
  const auto& GetGainList() { return m_gainList; }
  bool GetIsGainManual() const { return m_isGainManual; }
  float GetSelectedGain() const { return m_selectedGain; }
  uint32_t GetSelectedFrequency() const { return m_selectedFrequency; }
  const auto& GetSelectedFrequencyLabel() const { return m_selectedFrequencyLabel; }

  void SetNearestGain(const float target_gain);
  void SetCenterFrequency(const uint32_t freq);
  template<typename F>
  void SetDataCallback(F&& func)
  {
    m_callbackOn_Data = std::move(func);
  }

  template<typename F>
  void SetDataCallback2(F&& func)
  {
    m_isRunning = false;
    m_callbackOn_Data2 = std::move(func);
    m_isRunning = true;
  }

  void ResetDataCallback2()
  {
    m_isRunning = false;
    m_callbackOn_Data2 = nullptr;
    m_isRunning = true;
  }

  template<typename F>
  void SetFrequencyChangeCallback(F&& func)
  {
    m_callbackOn_SetCenterFrequency = std::move(func);
  }

  virtual void SearchGains() = 0;
  virtual void SetGain(const float gain) = 0;
  virtual void SetAutoGain() = 0;
  virtual void SetSamplingFrequency(const uint32_t freq) = 0;
  virtual void SetCenterFrequency(const std::string& label, const uint32_t freq) = 0;

protected:
  void OnData(tcb::span<const uint8_t> buf);

  const DeviceInfo m_info;
  uint32_t m_blockSize;
  std::atomic_bool m_isRunning = false;
  std::unique_ptr<std::thread> m_runnerThread;

  std::vector<float> m_gainList;
  bool m_isGainManual{false};
  float m_selectedGain{0.0f};
  uint32_t m_selectedFrequency{0};
  std::string m_selectedFrequencyLabel;

  std::function<size_t(tcb::span<const uint8_t>)> m_callbackOn_Data = nullptr;
  std::function<size_t(tcb::span<const uint8_t>)> m_callbackOn_Data2 = nullptr;
  std::function<void(const std::string&, const uint32_t)> m_callbackOn_SetCenterFrequency = nullptr;

private:
  // Don't allow move & copy about this class
  CDevice(CDevice&) = delete;
  CDevice(CDevice&&) = delete;
  CDevice& operator=(CDevice&) = delete;
  CDevice& operator=(CDevice&&) = delete;
};

} // namespace DEVICE
} // namespace RTLRADIO
