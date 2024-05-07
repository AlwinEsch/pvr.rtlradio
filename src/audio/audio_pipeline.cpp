/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "audio_pipeline.h"

#include "frame.h"
#include "utils/span.h"

#include <assert.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <utility>
#include <vector>

namespace
{

static constexpr size_t DATA_SIZE = sizeof(float) * Frame<float>::TOTAL_AUDIO_CHANNELS;
static constexpr float DATA_GAIN = 1.0f / float(std::numeric_limits<int16_t>::max());

template<typename T, typename U, typename F>
static void audio_map_with_callback(tcb::span<const Frame<T>> src,
                                    tcb::span<Frame<U>> dest,
                                    F&& func)
{
  assert(src.size() == dest.size());
  const size_t N = src.size();
  for (size_t i = 0; i < N; i++)
  {
    func(dest[i], src[i]);
  }
}

template<typename T, typename U, typename F>
static void audio_resample_with_callback(tcb::span<const Frame<T>> src,
                                         tcb::span<Frame<U>> dest,
                                         F&& func)
{
  const size_t N_dest = dest.size();
  const size_t N_src = src.size();
  const float ratio = float(N_src - 1) / float(N_dest - 1);
  for (size_t dest_i = 0; dest_i < N_dest; dest_i++)
  {
    const float src_i = float(dest_i) * ratio;
    const size_t src_i0 = size_t(src_i);
    size_t src_i1 = src_i0 + 1;
    if (src_i1 >= N_src)
    {
      src_i1 = N_src - 1;
    }
    const float k = src_i - float(src_i0);
    const auto v0 = static_cast<const Frame<float>>(src[src_i0]);
    const auto v1 = static_cast<const Frame<float>>(src[src_i1]);
    const auto v2 = v0 * (1.0f - k) + v1 * k;
    func(dest[dest_i], v2);
  }
}

template<typename T, typename F>
static void audio_resample_same_type_with_callback(tcb::span<const Frame<T>> src,
                                                   tcb::span<Frame<T>> dest,
                                                   F&& func)
{
  if (src.size() == dest.size())
  {
    audio_map_with_callback<T, T>(src, dest, std::move(func));
  }
  else
  {
    audio_resample_with_callback<T, T>(src, dest, std::move(func));
  }
}

template<typename T>
static void audio_clamp_inplace(tcb::span<Frame<T>> buf, const T v_min, const T v_max)
{
  const size_t N = buf.size();
  for (size_t i = 0; i < N; i++)
  {
    auto& v = buf[i];
    for (int j = 0; j < Frame<T>::TOTAL_AUDIO_CHANNELS; j++)
    {
      auto& x = v.channels[j];
      x = (x > v_min) ? x : v_min;
      x = (x > v_max) ? v_max : x;
    }
  }
}

} // namespace

AudioPipelineSource::AudioPipelineSource(uint32_t id, float sampling_rate, size_t buffer_length)
  : m_id(id), m_sampling_rate(sampling_rate), m_ring_buffer(buffer_length)
{
}

void AudioPipelineSource::write(tcb::span<const Frame<int16_t>> src,
                                float src_sampling_rate,
                                bool is_blocking)
{
  const size_t resample_length = size_t(float(src.size()) * m_sampling_rate / src_sampling_rate);
  m_resampling_buffer.resize(resample_length);

  if (resample_length == src.size())
  {
    audio_map_with_callback<int16_t, float>(src, m_resampling_buffer,
                                            [](Frame<float>& v_dest, const Frame<int16_t>& v_src) {
                                              v_dest = static_cast<Frame<float>>(v_src) * DATA_GAIN;
                                            });
  }
  else
  {
    audio_resample_with_callback<int16_t, float>(
        src, m_resampling_buffer,
        [](Frame<float>& v_dest, const Frame<float>& v_src) { v_dest = v_src * DATA_GAIN; });
  }

  auto lock = std::unique_lock(m_mutex_ring_buffer);
  if (!is_blocking)
  {
    m_ring_buffer.write_forcefully(m_resampling_buffer);
    return;
  }

  auto read_buffer = tcb::span<const Frame<float>>(m_resampling_buffer);
  while (true)
  {
    const size_t total_read = m_ring_buffer.write(read_buffer);
    read_buffer = read_buffer.subspan(total_read);
    if (read_buffer.empty())
    {
      break;
    }

    m_cv_ring_buffer.wait(lock, [this] { return !m_ring_buffer.is_full(); });
  }
}

bool AudioPipelineSource::read(tcb::span<Frame<float>> dest)
{
  auto lock = std::unique_lock(m_mutex_ring_buffer);
  if (m_ring_buffer.get_total_used() < dest.size())
  {
    return false;
  }
  m_ring_buffer.read(dest);
  lock.unlock();
  m_cv_ring_buffer.notify_one();
  return true;
}

void AudioPipeline::set_active(bool active)
{
  m_active = active;
}

bool AudioPipeline::source_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate)
{
  if (m_single)
    return single_source_to_sink(dest, dest_sampling_rate);
  else
    return mix_sources_to_sink(dest, dest_sampling_rate);
}

bool AudioPipeline::single_source_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate)
{
  std::shared_ptr<AudioPipelineSource> source;

  {
    auto lock = std::scoped_lock(m_mutex_sources);
    source = m_single_source;
  }

  const size_t N_dest = dest.size();
  const size_t N_size = N_dest * DATA_SIZE;
  const float src_sampling_rate = source->get_sampling_rate();
  const size_t N_src = size_t(float(N_dest) * src_sampling_rate / dest_sampling_rate);
  m_read_buffer.resize(N_src);

  if (!source->read(m_read_buffer))
  {
    std::memset(dest.data(), 0, N_size);
    return false;
  }

  if (m_read_buffer.size() == dest.size())
  {
    std::memcpy(dest.data(), m_read_buffer.data(), N_size);
  }
  else
  {
    audio_resample_with_callback<float>(
        m_read_buffer, dest,
        [](Frame<float>& v_dest, const Frame<float>& v_src) { v_dest = v_src; });
  }

  return true;
}

bool AudioPipeline::mix_sources_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate)
{
  const size_t N_dest = dest.size();
  std::memset(dest.data(), 0, N_dest * DATA_SIZE);

  std::vector<std::shared_ptr<AudioPipelineSource>> sources;

  {
    auto lock = std::scoped_lock(m_mutex_sources);
    sources = m_sources;
  }

  size_t total_sources_mixed = 0;
  for (auto& source : sources)
  {
    const float src_sampling_rate = source->get_sampling_rate();
    const size_t N_src = size_t(float(N_dest) * src_sampling_rate / dest_sampling_rate);
    m_read_buffer.resize(N_src);

    if (!source->read(m_read_buffer))
      continue;

    audio_resample_same_type_with_callback<float>(
        m_read_buffer, dest,
        [](Frame<float>& v_dest, const Frame<float>& v_src) { v_dest += v_src; });
    ++total_sources_mixed;
  }

  if (total_sources_mixed > 1)
  {
    const float gain = m_global_gain / std::log10(float(total_sources_mixed * 10.0f));
    for (auto& v : dest)
    {
      v = v * gain;
    }

    audio_clamp_inplace(dest, -1.0f, 1.0f);
  }

  return total_sources_mixed > 0;
}
