/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "device_usb.h"

#include "settings/settings.h"
#include "utils/log.h"

extern "C"
{
#include <rtl-sdr.h>
}

namespace RTLRADIO
{
namespace DEVICE
{

std::vector<DeviceInfo> CDeviceUSB::GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings)
{
  const uint32_t totalDevices = rtlsdr_get_device_count();
  if (totalDevices == 0)
    return std::vector<DeviceInfo>();

  auto list = std::vector<DeviceInfo>(size_t(totalDevices));
  constexpr size_t N = 256;
  static char vendor_str[N] = {0};
  static char product_str[N] = {0};
  static char serial_str[N] = {0};
  for (uint32_t i = 0; i < totalDevices; i++)
  {
    rtlsdr_get_device_usb_strings(i, vendor_str, product_str, serial_str);
    const char* name = rtlsdr_get_device_name(i);

    list[i] = DeviceInfo{DeviceSource::USB,
                         i,
                         std::string(name),
                         std::string(vendor_str, strnlen(vendor_str, N)),
                         std::string(product_str, strnlen(product_str, N)),
                         std::string(serial_str, strnlen(serial_str, N))};
  }

  return list;
}

CDeviceUSB::CDeviceUSB(const DeviceInfo& info,
                       const uint32_t blockSize,
                       std::shared_ptr<SETTINGS::CSettings> settings)
  : CDevice(info, blockSize, settings)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Construct");
}

CDeviceUSB::~CDeviceUSB()
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Destruct");
  Close();
}

bool CDeviceUSB::Create()
{
  if (m_device != nullptr)
    Close();

  const uint32_t index = m_info.index;
  int status = 0;

  status = rtlsdr_open(&m_device, index);
  if (status < 0 || m_device == nullptr)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to open device at index {} ({})",
               index, status);
    return false;
  }

  SearchGains();
  SetNearestGain(19.0f);
  SetSamplingFrequency(2048000);

  status = rtlsdr_set_bias_tee(m_device, 0);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to disable bias tee ({})", status);
    return false;
  }

  status = rtlsdr_reset_buffer(m_device);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to reset buffer ({})", status);
    return false;
  }

  m_isRunning = true;

  m_runnerThread = std::make_unique<std::thread>([this]() {
    const int status_read = rtlsdr_read_async(m_device, &CDeviceUSB::rtlsdr_callback,
                                              reinterpret_cast<void*>(this), 0, m_blockSize);
    if (status_read != 0)
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
                 "[device] rtlsdr_read_sync exited with {}", status_read);
  });

  return true;
}

void CDeviceUSB::Close()
{
  m_isRunning = false;
  const int ret = rtlsdr_cancel_async(m_device);
  if (ret != 0)
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "[device] rtlsdr_cancel_async exited with {}", ret);

  if (m_runnerThread)
  {
    if (m_runnerThread->joinable())
      m_runnerThread->join();
    m_runnerThread.reset();
  }

  rtlsdr_close(m_device);
  m_device = nullptr;
}

void CDeviceUSB::SearchGains()
{
  const int total_gains = rtlsdr_get_tuner_gains(m_device, nullptr);
  if (total_gains <= 0)
    return;

  auto qgains = std::vector<int>(size_t(total_gains));
  m_gainList.resize(size_t(total_gains));
  rtlsdr_get_tuner_gains(m_device, qgains.data());
  for (size_t i = 0; i < size_t(total_gains); i++)
  {
    const int qgain = qgains[i];
    const float gain = static_cast<float>(qgain) * 0.1f;
    m_gainList[i] = gain;
  }
}

void CDeviceUSB::SetGain(const float gain)
{
  const int qgain = static_cast<int>(gain * 10.0f);
  int status = 0;

  status = rtlsdr_set_tuner_gain_mode(m_device, 1);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set tuner gain mode to manual ({})", status);
    return;
  }

  status = rtlsdr_set_tuner_gain(m_device, qgain);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set manual gain to {:.1f}dB ({})", gain, status);
    return;
  }

  m_isGainManual = true;
  m_selectedGain = gain;
}

void CDeviceUSB::SetAutoGain()
{
  const int status = rtlsdr_set_tuner_gain_mode(m_device, 0);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set tuner gain mode to automatic ({})", status);
    return;
  }

  m_isGainManual = false;
  m_selectedGain = 0.0f;
}

void CDeviceUSB::SetSamplingFrequency(const uint32_t freq)
{
  const int status = rtlsdr_set_sample_rate(m_device, freq);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set sampling frequency to{} Hz({})", freq, status);
    return;
  }
}

void CDeviceUSB::SetCenterFrequency(const std::string& label, const uint32_t freq)
{
  if (m_callbackOn_SetCenterFrequency != nullptr)
    m_callbackOn_SetCenterFrequency(label, freq);

  const int status = rtlsdr_set_center_freq(m_device, freq);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set center frequency to {}@{}Hz ({})", label, freq, status);

    // Resend notification with original frequency
    if (m_callbackOn_SetCenterFrequency != nullptr)
      m_callbackOn_SetCenterFrequency(m_selectedFrequencyLabel, m_selectedFrequency);

    return;
  }

  m_selectedFrequencyLabel = label;
  m_selectedFrequency = freq;
}

void CDeviceUSB::rtlsdr_callback(uint8_t* buf, uint32_t len, void* ctx)
{
  auto* device = reinterpret_cast<CDeviceUSB*>(ctx);
  auto data = tcb::span<const uint8_t>(buf, size_t(len));
  device->OnData(data);
}

} // namespace DEVICE
} // namespace RTLRADIO
