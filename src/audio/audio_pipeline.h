#pragma once

#include "./frame.h"
#include "./ring_buffer.h"
#include "utility/span.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <vector>

typedef uint32_t audio_id;
constexpr const audio_id AUDIO_ID_UNDEFINED = std::numeric_limits<audio_id>::max();

constexpr float DEFAULT_AUDIO_SAMPLE_RATE = 48000.0f;
constexpr float DEFAULT_AUDIO_SINK_DURATION = 0.1f;
constexpr float DEFAULT_AUDIO_SOURCE_DURATION = 0.3f;
constexpr size_t DEFAULT_AUDIO_SINK_SAMPLES =
    size_t(DEFAULT_AUDIO_SAMPLE_RATE * DEFAULT_AUDIO_SINK_DURATION);
constexpr size_t DEFAULT_AUDIO_SOURCE_SAMPLES =
    size_t(DEFAULT_AUDIO_SAMPLE_RATE * DEFAULT_AUDIO_SOURCE_DURATION);

class AudioPipelineSource
{
private:
  const audio_id m_id;
  const float m_sampling_rate;

  std::vector<Frame<float>> m_resampling_buffer;
  RingBuffer<Frame<float>> m_ring_buffer;
  std::vector<Frame<float>> m_read_buffer;

  std::mutex m_mutex_ring_buffer;
  std::condition_variable m_cv_ring_buffer;

public:
  explicit AudioPipelineSource(audio_id id,
                               float sampling_rate = DEFAULT_AUDIO_SAMPLE_RATE,
                               size_t buffer_length = DEFAULT_AUDIO_SOURCE_SAMPLES);
  void write(tcb::span<const Frame<int16_t>> src, float src_sampling_rate, bool is_blocking);
  bool read(tcb::span<Frame<float>> dest);
  audio_id get_id() const { return m_id; }
  float get_sampling_rate() const { return m_sampling_rate; }
  void Notify() { m_cv_ring_buffer.notify_one(); }
};

class AudioPipeline
{
private:
  float m_global_gain = 1.0f;
  std::atomic_bool m_single{false};
  std::atomic_bool m_active{false};
  std::shared_ptr<AudioPipelineSource> m_single_source;
  std::vector<std::shared_ptr<AudioPipelineSource>> m_sources;
  std::vector<Frame<float>> m_read_buffer;
  std::mutex m_mutex_sources;

public:
  AudioPipeline() = default;
  void set_active(bool active);
  bool is_active() const { return m_active;  }
  bool source_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate);
  void add_source(std::shared_ptr<AudioPipelineSource>& source)
  {
    auto lock = std::scoped_lock(m_mutex_sources);
    m_sources.push_back(source);
  }
  void set_active_source(audio_id id)
  {
    auto lock = std::scoped_lock(m_mutex_sources);
    auto it = std::find_if(m_sources.begin(), m_sources.end(),
                           [id](const auto& entry) { return entry->get_id() == id; });
    if (it != m_sources.end())
    {
      m_single_source = *it;
      m_single = true;
    }
    else
    {
      m_single = false;
      m_single_source.reset();
    }
  }
  void clear_sources()
  {
    auto lock = std::scoped_lock(m_mutex_sources);
    m_single_source.reset();
    m_sources.clear();
  }
  float& get_global_gain() { return m_global_gain; }

private:
  bool single_source_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate);
  bool mix_sources_to_sink(tcb::span<Frame<float>> dest, float dest_sampling_rate);
};
