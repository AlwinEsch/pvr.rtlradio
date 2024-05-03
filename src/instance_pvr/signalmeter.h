/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "fastfir.h"
#include "props.h"
#include "utils/span.h"

#include <functional>
#include <memory>

namespace RTLRADIO
{

struct SignalProps
{

  uint32_t samplerate; // Signal sampling rate
  uint32_t bandwidth; // Signal bandwidth
  int32_t lowcut; // low cut from center
  int32_t highcut; // high cut from center
  uint32_t offset; // Signal frequency offset
  bool filter; // Flag to filter the signal
};

struct SignalPlotProps
{
  size_t height; // Plot height
  size_t width; // Plot width
  float mindb; // Plot minimum dB value
  float maxdb; // Plot maximum dB value
};

struct SignalStatus
{
  float power; // Signal power level in dB
  float noise; // Signal noise level in dB
  float snr; // Signal-to-noise ratio in dB
  bool overload; // FFT input data is overloaded
  int32_t lowcut; // Low cut plot index
  int32_t highcut; // High cut plot index
  size_t plotsize; // Size of the signal plot data array
  int const* plotdata; // Pointer to the signal plot data
};

namespace INSTANCE
{

class CSignalMeter
{
public:
  ~CSignalMeter() = default;

  using exception_callback = std::function<void(std::exception const& ex)>;
  using status_callback = std::function<void(struct SignalStatus const& status)>;

  //static std::shared_ptr<CSignalMeter> Create(struct SignalProps const& signalprops,
  //                                            struct SignalPlotProps const& plotprops,
  //                                            uint32_t rate,
  //                                            status_callback const&& onstatus);

  void ProcessInputSamples(tcb::span<const uint8_t> samples);

  //private:
  CSignalMeter(struct SignalProps const& signalprops,
               struct SignalPlotProps const& plotprops,
               uint32_t rate,
               status_callback const&& onstatus);
  CSignalMeter(CSignalMeter const&) = delete;
  CSignalMeter& operator=(CSignalMeter const&) = delete;

  static constexpr size_t const DEFAULT_FFT_SIZE = 512;
  static constexpr size_t const RING_BUFFER_SIZE = (4 MiB); // 1s @ 2048000

  void ProcessSamples();

  //-----------------------------------------------------------------------
  // Member Variables

  struct SignalProps const m_signalprops; // Signal properties
  struct SignalPlotProps const m_plotprops; // Signal plot properties
  status_callback const m_onstatus; // Status callback function

  // FFT
  //
  CFastFIR m_fir; // Finite impulse response filter
  size_t m_fftsize{DEFAULT_FFT_SIZE}; // FFT size (bins)
  CFft m_fft; // FFT instance
  size_t m_fftminbytes{0}; // Number of bytes required to process
  float m_avgpower{NAN}; // Average power level
  float m_avgnoise{NAN}; // Average noise level

  // RING BUFFER
  //
  std::unique_ptr<uint8_t[]> m_buffer; // Input ring buffer
  size_t m_head{0}; // Ring buffer head position
  size_t m_tail{0}; // Ring buffer tail position
};

} // namespace INSTANCE
} // namespace RTLRADIO
