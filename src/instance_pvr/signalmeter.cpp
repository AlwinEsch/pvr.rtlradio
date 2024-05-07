/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "signalmeter.h"

#include "exception_control/string_exception.h"
#include "utils/align.h"

#include <cassert>
#include <cstring>

namespace RTLRADIO
{
namespace INSTANCE
{

//std::shared_ptr<CSignalMeter> CSignalMeter::Create(struct SignalProps const& signalprops,
//                                                   SignalPlotProps const& plotprops,
//                                                   uint32_t rate,
//                                                   status_callback const&& onstatus)
//{
//  return std::shared_ptr<CSignalMeter>(new CSignalMeter(signalprops, plotprops, rate, onstatus));
//}

CSignalMeter::CSignalMeter(struct SignalProps const& signalprops,
                           SignalPlotProps const& plotprops,
                           uint32_t rate,
                           status_callback const&& onstatus)
  : m_signalprops(signalprops), m_plotprops(plotprops), m_onstatus(onstatus)
{
  // Make sure the ring buffer is going to be big enough for the requested rate
  const size_t bufferrequired =
      static_cast<size_t>((signalprops.samplerate * 2) * (static_cast<float>(rate) / 1000.0f));
  if (bufferrequired > RING_BUFFER_SIZE)
    throw std::invalid_argument("rate");

  // Allocate the input ring buffer
  m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[RING_BUFFER_SIZE]);
  if (!m_buffer)
    throw std::bad_alloc();

  // Calculate the number of bytes required in the input buffer to process and report
  // updated signal statistics at (apporoximately) the requested rate in milliseconds
  const size_t bytespersecond = m_signalprops.samplerate * 2;
  m_fftminbytes =
      align::down(static_cast<size_t>(bytespersecond * (static_cast<float>(rate) / 1000.0f)),
                  static_cast<unsigned int>(m_fftsize));

  // Initialize the finite impulse response filter
  m_fir.SetupParameters(static_cast<float>(m_signalprops.lowcut),
                        static_cast<float>(m_signalprops.highcut),
                        -static_cast<float>(m_signalprops.offset), m_signalprops.samplerate);

  // Initialize the fast fourier transform instance
  m_fft.SetFFTParams(static_cast<int>(m_fftsize), false, 0.0, m_signalprops.samplerate);
  m_fft.SetFFTAve(50);
}

void CSignalMeter::ProcessInputSamples(tcb::span<const uint8_t> samples)
{
  size_t byteswritten = 0; // Bytes written into ring buffer

  if (samples.empty())
    return;

  // Ensure there is enough space in the ring buffer to satisfy the operation
  size_t available = (m_head < m_tail) ? m_tail - m_head : (RING_BUFFER_SIZE - m_head) + m_tail;
  if (samples.size() > available)
    throw string_exception("Insufficient ring buffer space to accomodate input");

  // Write the input data into the ring buffer
  size_t cb = samples.size();
  while (cb > 0)
  {
    // If the head is behind the tail linearly, take the data between them otherwise
    // take the data between the end of the buffer and the head
    const size_t chunk =
        (m_head < m_tail) ? std::min(cb, m_tail - m_head) : std::min(cb, RING_BUFFER_SIZE - m_head);
    memcpy(&m_buffer[m_head], &samples[byteswritten], chunk);

    m_head += chunk; // Increment the head position
    byteswritten += chunk; // Increment number of bytes written
    cb -= chunk; // Decrement remaining bytes

    // If the head has reached the end of the buffer, reset it back to zero
    if (m_head >= RING_BUFFER_SIZE)
      m_head = 0;
  }

  assert(byteswritten == samples.size()); // Verify all bytes were written
  ProcessSamples(); // Process the available input samples
}

void CSignalMeter::ProcessSamples()
{
  // Allocate a heap buffer to store the converted I/Q samples for the FFT
  std::unique_ptr<TYPECPX[]> samples(new TYPECPX[m_fftsize]);
  if (!samples)
    throw std::bad_alloc();

  assert((m_fftminbytes % m_fftsize) == 0);

  // Determine how much data is available in the ring buffer for processing as I/Q samples
  size_t available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
  while (available >= m_fftminbytes)
  {
    // Push all of the input samples into the FFT
    for (size_t iterations = 0; iterations < (available / m_fftsize); iterations++)
    {

      // Convert the raw 8-bit I/Q samples into scaled complex I/Q samples
      for (size_t index = 0; index < m_fftsize; index++)
      {

        // The FFT expects the I/Q samples in the range of -32767.0 through +32767.0
        // (32767.0 / 127.5) = 256.9960784313725
        samples[index] = {
            (static_cast<float>(m_buffer[m_tail]) - 127.5f) * 256.9960784313725f, // I
            (static_cast<float>(m_buffer[m_tail + 1]) - 127.5f) * 256.9960784313725f, // Q
        };

        m_tail += 2;
        if (m_tail >= RING_BUFFER_SIZE)
          m_tail = 0;
      }

      size_t numsamples = m_fftsize;

      // If specified, filter out everything but the desired bandwidth
      if (m_signalprops.filter)
      {

        numsamples = m_fir.ProcessData(static_cast<int>(m_fftsize), &samples[0], &samples[0]);
        assert(numsamples == m_fftsize);
      }

      // Push the current set of samples through the fast fourier transform
      m_fft.PutInDisplayFFT(static_cast<int32_t>(numsamples), &samples[0]);

      // Recalculate the amount of available data in the ring buffer after the read operation
      available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
    }

    // Convert the FFT into an integer-based signal plot
    std::unique_ptr<int[]> plot(new int[m_plotprops.width + 1]);
    bool overload = m_fft.GetScreenIntegerFFTData(
        static_cast<int32_t>(m_plotprops.height), static_cast<int32_t>(m_plotprops.width),
        static_cast<float>(m_plotprops.maxdb), static_cast<float>(m_plotprops.mindb),
        -(static_cast<int32_t>(m_signalprops.bandwidth) / 2) - m_signalprops.offset,
        (static_cast<int32_t>(m_signalprops.bandwidth) / 2) - m_signalprops.offset, &plot[0]);

    // Determine how many Hertz are represented by each measurement in the signal plot and the center point
    float hzper =
        static_cast<float>(m_plotprops.width) / static_cast<float>(m_signalprops.bandwidth);
    int32_t center = static_cast<int>(m_plotprops.width) / 2;

    // Initialize a new signal_status structure with the proper signal type for the low/high cut indexes
    struct SignalStatus status = {};

    // Power is measured at the center frequency and smoothed
    float power = ((m_plotprops.mindb - m_plotprops.maxdb) *
                   (static_cast<float>(plot[center]) / static_cast<float>(m_plotprops.height))) +
                  m_plotprops.maxdb;
    status.power = m_avgpower =
        (std::isnan(m_avgpower)) ? power : 0.85f * m_avgpower + 0.15f * power;

    // Noise is measured at the low and high cuts, averaged, and smoothed
    status.lowcut = std::max(0, center + static_cast<int32_t>(m_signalprops.lowcut * hzper));
    status.highcut = std::min(static_cast<int32_t>(m_plotprops.width - 1),
                              center + static_cast<int32_t>(m_signalprops.highcut * hzper));
    float noise = ((m_plotprops.mindb - m_plotprops.maxdb) *
                   (static_cast<float>((plot[status.lowcut] + plot[status.highcut]) / 2.0f) /
                    static_cast<float>(m_plotprops.height))) +
                  m_plotprops.maxdb;
    status.noise = m_avgnoise =
        (std::isnan(m_avgnoise)) ? noise : 0.85f * m_avgnoise + 0.15f * noise;

    status.snr = m_avgpower + -m_avgnoise; // SNR in dB
    status.overload = overload; // Overload flag
    status.plotsize = m_plotprops.width; // Plot width
    status.plotdata = &plot[0]; // Plot data

    // Invoke the callback to report the updated metrics and signal plot
    m_onstatus(status);

    // Recalculate the amount of available data in the ring buffer
    available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
  }
}

} // namespace INSTANCE
} // namespace RTLRADIO
