/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "device_file.h"

#include "settings/settings.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace DEVICE
{

std::vector<DeviceInfo> CDeviceFile::GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings)
{
  auto list = std::vector<DeviceInfo>(size_t(1));
  list[0] = DeviceInfo{DeviceSource::TCP, 0, "Test file device", "virtual", "Test file", "0"};

  return list;
}

CDeviceFile::CDeviceFile(const DeviceInfo& info,
                         const uint32_t blockSize,
                         std::shared_ptr<SETTINGS::CSettings> settings,
                         const std::string& filename,
                         uint32_t samplerate)
  : CDevice(info, blockSize, settings), m_filename(filename), m_samplerate(samplerate)
{
}

CDeviceFile::~CDeviceFile()
{
  Close();
}

bool CDeviceFile::Create()
{
  if (m_filename.empty())
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Invalid test filename");
    return false;
  }

  // Attempt to open the target file in read-only binary mode
  m_file = fopen(m_filename.c_str(), "rb");
  if (m_file == nullptr)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to open test filename: '{}'",
               m_filename);
    return false;
  }

  m_runnerThread = std::make_unique<std::thread>([this]() {
    const bool status_read = read_async();
    if (!status_read)
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "read_async exited by error");
  });

  return true;
}

void CDeviceFile::Close()
{
  cancel_async();

  if (m_file != nullptr)
    fclose(m_file);
  m_file = nullptr;
}

void CDeviceFile::SearchGains()
{
}

void CDeviceFile::SetGain(const float gain)
{
}

void CDeviceFile::SetSamplingFrequency(const uint32_t freq)
{
}

void CDeviceFile::SetCenterFrequency(const std::string& label, const uint32_t freq)
{
  if (m_callbackOn_SetCenterFrequency != nullptr)
    m_callbackOn_SetCenterFrequency(label, freq);
}

void CDeviceFile::SetAutoGain()
{
}

size_t CDeviceFile::read(uint8_t* buffer, size_t count) const
{
  assert(m_file != nullptr);
  assert(m_samplerate != 0);

  auto start = std::chrono::steady_clock::now();

  // Synchronously read the requested amount of data from the input file
  size_t read = fread(buffer, sizeof(uint8_t), count, m_file);

  if (read > 0)
  {

    // Determine how long this operation should take to execute to maintain sample rate
    int duration = static_cast<int>(static_cast<double>(read) / ((m_samplerate * 2) / 1000000.0));
    auto end = start + std::chrono::microseconds(duration);

    // Yield until the calculated duration has expired
    while (std::chrono::steady_clock::now() < end)
    {
      std::this_thread::yield();
    }
  }

  return read;
}

bool CDeviceFile::read_async()
{
  std::unique_ptr<uint8_t[]> buffer(new uint8_t[m_blockSize]); // Input data buffer

  m_stop = false;
  m_stopped = false;

  try
  {

    // Continuously read data from the device until the stop condition is set
    while (m_stop.test(true) == false)
    {
      // Try to read enough data to fill the input buffer
      size_t len = read(&buffer[0], m_blockSize);
      auto data = tcb::span<const uint8_t>(&buffer[0], len);
      OnData(data);
    }

    m_stopped = true; // Operation has been stopped
    return true;
  }
  catch (...)
  {
    m_stopped = true;
    return false;
  }
}

void CDeviceFile::cancel_async() const
{
  // If the asynchronous operation is stopped, do nothing
  if (m_stopped.test(true) == true)
    return;

  m_stop = true; // Flag a stop condition
  m_stopped.wait_until_equals(true); // Wait for async to stop
}

} // namespace DEVICE
} // namespace RTLRADIO
