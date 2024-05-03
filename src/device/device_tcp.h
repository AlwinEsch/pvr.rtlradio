/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "device.h"

#include <vector>

#ifdef USB_DEVICE_SUPPORT
#include <rtl-sdr.h>
#else

// Workaround with with copy from "rtl-sdr.h" to become usable over TCP without
// rtl-sdr lib.
enum rtlsdr_tuner
{
  RTLSDR_TUNER_UNKNOWN = 0,
  RTLSDR_TUNER_E4000,
  RTLSDR_TUNER_FC0012,
  RTLSDR_TUNER_FC0013,
  RTLSDR_TUNER_FC2580,
  RTLSDR_TUNER_R820T,
  RTLSDR_TUNER_R828D
};

#endif // USB_DEVICE_SUPPORT

namespace RTLRADIO
{
namespace DEVICE
{

class CDeviceTCP : public CDevice
{
public:
  static std::vector<DeviceInfo> GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings);

  explicit CDeviceTCP(const DeviceInfo& info,
                      const uint32_t blockSize,
                      std::shared_ptr<SETTINGS::CSettings> settings);
  ~CDeviceTCP() override;

  bool Create() override;
  void Close() override;

  void SearchGains() override;
  void SetGain(const float gain) override;
  void SetAutoGain() override;
  void SetSamplingFrequency(const uint32_t freq) override;
  void SetCenterFrequency(const std::string& label, const uint32_t freq) override;

private:
  int rtltcp_set_tuner_gain_mode(bool enable) const;
  int rtltcp_set_tuner_gain(int gain) const;
  int rtltcp_set_center_freq(uint32_t freq) const;
  int rtltcp_set_sample_rate(uint32_t samp_rate) const;
  int rtltcp_reset_buffer() const;
  int rtltcp_set_bias_tee(int on) const;

  std::string m_deviceConnection_TCPHost;
  int m_deviceConnection_TCPPort;

  int m_socket{-1}; // TCP/IP socket
  rtlsdr_tuner m_tunertype{RTLSDR_TUNER_UNKNOWN}; // Tuner type
  uint32_t m_tunerGainCount{0};
};

} // namespace DEVICE
} // namespace RTLRADIO
