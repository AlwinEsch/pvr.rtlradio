/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "device_tcp.h"

#include "exception_control/socket_exception.h"
#include "settings/settings.h"
#include "utils/log.h"

#ifdef _WIN32
#include <wchar.h> // Prevents redefinition of WCHAR_MIN by libusb

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

// What a fu.. about all the stupid defined macros on Windows system :(
// Windows have "ERROR" somehere defined, where becomes in conflict with e.g. enum UTILS::LOGLevel::ERROR
#ifdef ERROR
#undef ERROR
#endif
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{

/*!
 * @brief struct device_info
 *
 * Retrieves information about the connected device.
 * 
 * @note Taken from RTL-SDR on rtl_tcp.c.
 */
struct device_info
{
  char magic[4];
  uint32_t tuner_type;
  uint32_t tuner_gain_count;
};

/*!
 * @brief struct device_command
 * 
 * Structure used to send a command to the connected device.
 * 
 * @note Taken from RTL-SDR on rtl_tcp.c.
 */
#ifdef _WIN32
#define __attribute__(x)
#pragma pack(push, 1)
#endif
struct device_command
{
  unsigned char cmd;
  unsigned int param;
} __attribute__((packed));
#ifdef _WIN32
#pragma pack(pop)
#endif

/*!
 * @brief rtl_tcp commands.
 */
//@{
constexpr const unsigned char RTLTCP_SET_CENTER_FREQ = 0x01;
constexpr const unsigned char RTLTCP_SET_SAMPLE_RATE = 0x02;
constexpr const unsigned char RTLTCP_SET_TUNER_GAIN_MODE = 0x03;
constexpr const unsigned char RTLTCP_SET_TUNER_GAIN = 0x04;
constexpr const unsigned char RTLTCP_SET_FREQ_CORRECTION = 0x05;
constexpr const unsigned char RTLTCP_SET_TUNER_IF_GAIN = 0x06;
constexpr const unsigned char RTLTCP_SET_TESTMODE = 0x07;
constexpr const unsigned char RTLTCP_SET_AGC_MODE = 0x08;
constexpr const unsigned char RTLTCP_SET_DIRECT_SAMPLING = 0x09;
constexpr const unsigned char RTLTCP_SET_OFFSET_TUNING = 0x0a;
constexpr const unsigned char RTLTCP_SET_RTL_XTAL_FREQ = 0x0b;
constexpr const unsigned char RTLTCP_SET_TUNER_XTAL_FREQ = 0x0c;
constexpr const unsigned char RTLTCP_SET_GAIN_BY_INDEX = 0x0d;
constexpr const unsigned char RTLTCP_SET_BIAS_TEE = 0x0e;
//@}

// device_info structure size must be 12 bytes in length (rtl_tcp.c)
static_assert(sizeof(device_info) == 12, "device_info structure size must be 12 bytes in length");

} // namespace

namespace RTLRADIO
{
namespace DEVICE
{

std::vector<DeviceInfo> CDeviceTCP::GetDeviceList(std::shared_ptr<SETTINGS::CSettings> settings)
{
  // Generate a device name for this instance
  const auto devicename =
      fmt::format("Realtek RTL2832U on {}:{}", settings->DeviceConnectionTCPHost(),
                  settings->DeviceConnectionTCPPort());

  auto list = std::vector<DeviceInfo>(size_t(1));
  list[0] = DeviceInfo{DeviceSource::TCP, 0, devicename, "RTL-SDR", "TCP port connection", "0"};

  return list;
}

CDeviceTCP::CDeviceTCP(const DeviceInfo& info,
                       const uint32_t blockSize,
                       std::shared_ptr<SETTINGS::CSettings> settings)
  : CDevice(info, blockSize, settings)
{
  m_deviceConnection_TCPHost = settings->DeviceConnectionTCPHost();
  m_deviceConnection_TCPPort = settings->DeviceConnectionTCPPort();
}

CDeviceTCP::~CDeviceTCP()
{
  Close();
}

bool CDeviceTCP::Create()
{
  struct addrinfo* addrs = nullptr; // Host device addresses
  struct addrinfo hints = {}; // getaddrinfo() hint values
  hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Preferred socket type
  hints.ai_protocol = IPPROTO_TCP; // TCP protocol
  hints.ai_flags = AI_PASSIVE; // Bindable socket

  // Get the address information to connect to the host
  int result = getaddrinfo(m_deviceConnection_TCPHost.c_str(),
                           std::to_string(m_deviceConnection_TCPPort).c_str(), &hints, &addrs);
  if (result < 0 || addrs == nullptr)
  {
#ifdef _WIN32
    std::string err = gai_strerrorA(result);
#else
    std::string err = gai_strerror(result);
#endif
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to get address and name information (ERROR: {})", err);
    return false;
  }

  int socketFile = -1;

  try
  {
    // Create the TCP/IP socket on which to communicate with the device
    socketFile = static_cast<int>(socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol));
    if (socketFile == -1)
      throw CSocketException(__src_loc_cur__, "Failed to create TCP/IP socket");

    // SO_REUSEADDR
    //
    int reuse = 1;
    result = setsockopt(socketFile, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char const*>(&reuse),
                        sizeof(int));
    if (result == -1)
      throw CSocketException(__src_loc_cur__, "Failed to set socket option SO_REUSEADDR");

    // SO_LINGER
    //
    struct linger linger = {};
    linger.l_onoff = 1;
    linger.l_linger = 0;

    result = setsockopt(socketFile, SOL_SOCKET, SO_LINGER, reinterpret_cast<char const*>(&linger),
                        sizeof(struct linger));
    if (result == -1)
      throw CSocketException(__src_loc_cur__, "Failed to set socket option SO_LINGER");

    // Establish the TCP/IP socket connection
    result = connect(socketFile, addrs->ai_addr, static_cast<int>(addrs->ai_addrlen));
    if (result != 0)
      throw CSocketException(__src_loc_cur__, "Failed to establish the TCP/IP socket connection");

    // TCP_NODELAY
    //
    int nodelay = 1;
    result = setsockopt(socketFile, IPPROTO_TCP, TCP_NODELAY,
                        reinterpret_cast<char const*>(&nodelay), sizeof(int));
    if (result == -1)
      throw CSocketException(__src_loc_cur__, "Failed to set socket option TCP_NODELAY");

      // SO_RCVTIMEO (initial recv())
      //
#ifdef _WIN32
    DWORD timeout = 5000;
#else
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
#endif
    result = setsockopt(socketFile, SOL_SOCKET, SO_RCVTIMEO,
                        reinterpret_cast<char const*>(&timeout), sizeof(timeout));
    if (result == -1)
      throw CSocketException(__src_loc_cur__, "Failed to set socket option SO_RCVTIMEO");

    // Retrieve the device information from the server
    struct device_info deviceinfo = {};
    result = recv(socketFile, reinterpret_cast<char*>(&deviceinfo), sizeof(struct device_info), 0);
    if (result != sizeof(struct device_info))
      throw CSocketException(__src_loc_cur__, "Failed to retrieve server device information");

      // SO_RCVTIMEO (subsequent recv()s)
      //
#ifdef _WIN32
    timeout = 1000;
#else
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
#endif

    result = setsockopt(socketFile, SOL_SOCKET, SO_RCVTIMEO,
                        reinterpret_cast<char const*>(&timeout), sizeof(timeout));
    if (result == -1)
      throw CSocketException(__src_loc_cur__, "Failed to set socket option SO_RCVTIMEO");

    // Parse the provided device information; only care about the tuner device type
    if (memcmp(deviceinfo.magic, "RTL0", 4) == 0)
    {
      m_tunertype = static_cast<rtlsdr_tuner>(ntohl(deviceinfo.tuner_type));
      m_tunerGainCount = deviceinfo.tuner_gain_count;
    }
    else
      throw CSocketException(__src_loc_cur__, "Invalid device information returned from host");

    // Turn off internal digital automatic gain control
    struct device_command command = {RTLTCP_SET_AGC_MODE, 0};
    result =
        send(socketFile, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__,
                             "Failed to turn off internal digital automatic gain control");

    // Release the list of address information
    freeaddrinfo(addrs);
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());

    if (addrs != nullptr)
      freeaddrinfo(addrs);
    if (socketFile != -1)
    {
#ifdef _WIN32
      shutdown(socketFile, SD_BOTH);
      closesocket(socketFile);
#else
      shutdown(socketFile, SHUT_RDWR);
      close(socketFile);
#endif
    }

    return false;
  }

  m_socket = socketFile;

  SearchGains();
  SetNearestGain(19.0f);
  SetSamplingFrequency(2048000);

  int status = 0;

  status = rtltcp_set_bias_tee(0);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to disable bias tee ({})", status);
    return false;
  }

  status = rtltcp_reset_buffer();
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "Failed to reset buffer ({})", status);
    return false;
  }

  return true;
}

void CDeviceTCP::Close()
{
  // Shutdown and close the socket
  if (m_socket != -1)
  {
#ifdef _WIN32
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
#else
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
#endif

    m_socket = -1;
  }
}

void CDeviceTCP::SearchGains()
{
  /*!
   * All gain values are expressed in tenths of a dB
   * 
   * Code partly taken from librtlsdr.c.
   */
  //@{
  static const int e4k_gains[] = {-10, 15, 40, 65, 90, 115, 140, 165, 190, 215, 240, 290, 340, 420};
  static const int fc0012_gains[] = {-99, -40, 71, 179, 192};
  static const int fc0013_gains[] = {-99, -73, -65, -63, -60, -58, -54, 58,  61,  63,  65, 67,
                                     68,  70,  71,  179, 181, 182, 184, 186, 188, 191, 197};
  static const int fc2580_gains[] = {0 /* no gain values */};
  static const int r82xx_gains[] = {0,   9,   14,  27,  37,  77,  87,  125, 144, 157,
                                    166, 197, 207, 229, 254, 280, 297, 328, 338, 364,
                                    372, 386, 402, 421, 434, 439, 445, 480, 496};
  static const int unknown_gains[] = {0 /* no gain values */};

  const int* ptr = nullptr;
  int len = 0;

  if (m_socket == -1)
    return;

  switch (m_tunertype)
  {
    case RTLSDR_TUNER_E4000:
      ptr = e4k_gains;
      len = sizeof(e4k_gains);
      break;
    case RTLSDR_TUNER_FC0012:
      ptr = fc0012_gains;
      len = sizeof(fc0012_gains);
      break;
    case RTLSDR_TUNER_FC0013:
      ptr = fc0013_gains;
      len = sizeof(fc0013_gains);
      break;
    case RTLSDR_TUNER_FC2580:
      ptr = fc2580_gains;
      len = sizeof(fc2580_gains);
      break;
    case RTLSDR_TUNER_R820T:
    case RTLSDR_TUNER_R828D:
      ptr = r82xx_gains;
      len = sizeof(r82xx_gains);
      break;
    default:
      ptr = unknown_gains;
      len = sizeof(unknown_gains);
      break;
  }
  //@}

  const size_t total_gains = len / sizeof(int);

  assert(m_tunerGainCount == total_gains);

  if (total_gains <= 0)
    return;

  for (size_t i = 0; i < total_gains; i++)
  {
    const int qgain = ptr[i];
    const float gain = static_cast<float>(qgain) * 0.1f;
    m_gainList[i] = gain;
  }
}

void CDeviceTCP::SetGain(const float gain)
{
  const int qgain = static_cast<int>(gain * 10.0f);
  int status = 0;

  status = rtltcp_set_tuner_gain_mode(true);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set tuner gain mode to manual ({})", status);
    return;
  }

  status = rtltcp_set_tuner_gain(qgain);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set manual gain to {:.1f}dB ({})", gain, status);
    return;
  }

  m_isGainManual = true;
  m_selectedGain = gain;
}

void CDeviceTCP::SetSamplingFrequency(const uint32_t freq)
{
  const int status = rtltcp_set_sample_rate(freq);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set sampling frequency to{} Hz({})", freq, status);
    return;
  }
}

void CDeviceTCP::SetCenterFrequency(const std::string& label, const uint32_t freq)
{
  if (m_callbackOn_SetCenterFrequency != nullptr)
    m_callbackOn_SetCenterFrequency(label, freq);

  const int status = rtltcp_set_center_freq(freq);
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

void CDeviceTCP::SetAutoGain()
{
  const int status = rtltcp_set_tuner_gain_mode(false);
  if (status < 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to set tuner gain mode to automatic ({})", status);
    return;
  }

  m_isGainManual = false;
  m_selectedGain = 0.0f;
}

int CDeviceTCP::rtltcp_set_tuner_gain_mode(bool enable) const
{
  assert(m_socket != -1);

  try
  {
    struct device_command command = {RTLTCP_SET_TUNER_GAIN_MODE, htonl((enable) ? 0 : 1)};
    int result =
        send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }

  return 0;
}

int CDeviceTCP::rtltcp_set_tuner_gain(int gain) const
{
  assert(m_socket != -1);

  try
  {
    struct device_command command = {RTLTCP_SET_TUNER_GAIN, htonl(gain)};
    int result =
        send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }

  return 0;
}

int CDeviceTCP::rtltcp_set_center_freq(uint32_t freq) const
{
  assert(m_socket != -1);

  try
  {
    struct device_command command = {RTLTCP_SET_CENTER_FREQ, htonl(freq)};
    int result =
        send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }

  return 0;
}

int CDeviceTCP::rtltcp_set_sample_rate(uint32_t samp_rate) const
{
  assert(m_socket != -1);

  try
  {
    struct device_command command = {RTLTCP_SET_SAMPLE_RATE, htonl(samp_rate)};
    int result =
        send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }

  return 0;
}

int CDeviceTCP::rtltcp_reset_buffer() const
{
  assert(m_socket != -1);

  /*
   * TODO: reset buffer not supported in network interface!
  try
  {
    struct device_command command = { RTLTCP_RESET_BUFFER, htonl(0) };
    int result =
      send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }
  */

  return 0;
}

int CDeviceTCP::rtltcp_set_bias_tee(int on) const
{
  assert(m_socket != -1);

  try
  {
    struct device_command command = {RTLTCP_SET_BIAS_TEE, htonl(on)};
    int result =
        send(m_socket, reinterpret_cast<char const*>(&command), sizeof(struct device_command), 0);
    if (result != sizeof(struct device_command))
      throw CSocketException(__src_loc_cur__, "send() failed");
  }
  catch (CSocketException& ex)
  {
    UTILS::LOG(UTILS::LOGLevel::ERROR, ex.what());
    return -1;
  }

  return 0;
}

} // namespace DEVICE
} // namespace RTLRADIO
