/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "inputstream_type_dab.h"

#include "device/device.h"
#include "device/device_list.h"
#include "dsp_dab/block_frequencies.h"
#include "dsp_dab/process_lib/dab/database/dab_database.h"
#include "dsp_dab/process_lib/dab/database/dab_database_entities.h"
#include "dsp_dab/process_lib/dab/database/dab_database_updater.h"
#include "settings/settings.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace INSTANCE
{

class CBasicRadioSwitcher
{
private:
  DAB_Parameters m_dab_params;
  std::shared_ptr<InputBuffer<viterbi_bit_t>> m_input_stream = nullptr;
  std::vector<viterbi_bit_t> m_bits_buffer;
  std::map<std::string, std::shared_ptr<BasicRadio>> m_instances;
  std::shared_ptr<BasicRadio> m_selected_instance = nullptr;
  std::mutex m_mutex_selected_instance;
  size_t m_flush_reads = 0;
  std::function<std::shared_ptr<BasicRadio>(const DAB_Parameters&, std::string_view)>
      m_create_instance;

public:
  template<typename F>
  CBasicRadioSwitcher(int transmission_mode, F&& create_instance)
    : m_dab_params(get_dab_parameters(transmission_mode)), m_create_instance(create_instance)
  {
    m_bits_buffer.resize(m_dab_params.nb_frame_bits);
  }
  void set_input_stream(std::shared_ptr<InputBuffer<viterbi_bit_t>> stream)
  {
    m_input_stream = stream;
  }
  void flush_input_stream() { m_flush_reads = 5; }
  void switch_instance(std::string_view key, const uint32_t freq)
  {
    auto lock = std::unique_lock(m_mutex_selected_instance);
    auto res = m_instances.find(std::string(key));
    std::shared_ptr<BasicRadio> new_instance = nullptr;
    if (res != m_instances.end())
    {
      new_instance = res->second;
    }
    else
    {
      new_instance = m_create_instance(m_dab_params, key);
      m_instances.insert({std::string(key), new_instance});
    }

    if (m_selected_instance != new_instance)
    {
      flush_input_stream();
    }

    m_selected_instance = new_instance;
  }
  std::shared_ptr<BasicRadio> get_instance()
  {
    auto lock = std::unique_lock(m_mutex_selected_instance);
    return m_selected_instance;
  }
  void run()
  {
    if (m_input_stream == nullptr)
      return;
    while (true)
    {
      const size_t length = m_input_stream->read(m_bits_buffer);
      if (length != m_bits_buffer.size())
        return;

      auto lock = std::unique_lock(m_mutex_selected_instance);
      if (m_flush_reads > 0)
      {
        m_flush_reads -= 1;
        continue;
      }
      if (m_selected_instance == nullptr)
        continue;
      m_selected_instance->Process(m_bits_buffer);
    }
  }
};

class CDeviceSource
{
private:
  std::shared_ptr<DEVICE::CDevice> m_device = nullptr;
  std::mutex m_mutex_device;
  std::function<void(std::shared_ptr<DEVICE::CDevice>)> m_device_change_callback;

public:
  template<typename F>
  explicit CDeviceSource(F&& device_change_callback)
    : m_device_change_callback(device_change_callback)
  {
  }
  std::shared_ptr<DEVICE::CDevice> get_device()
  {
    auto lock = std::unique_lock(m_mutex_device);
    return m_device;
  }
  void set_device(std::shared_ptr<DEVICE::CDevice> device)
  {
    auto lock = std::unique_lock(m_mutex_device);
    m_device = device;
    m_device_change_callback(m_device);
  }
};

CInputstreamTypeDAB::CInputstreamTypeDAB(std::shared_ptr<SETTINGS::CSettings> settings,
                                         const std::shared_ptr<AudioPipeline>& audioPipeline)
  : IInputstreamType(settings, audioPipeline)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Inputstream instance created");

  const auto transmissionNode = settings->TransmissionNode();

  const auto dab_params = get_dab_parameters(settings->TransmissionNode());
  // ofdm
  m_ofdm_block = std::make_shared<OFDM_Block>(transmissionNode, settings->GetOFDMTotalThreads());
  auto& ofdm_config = m_ofdm_block->get_ofdm_demod().GetConfig();
  ofdm_config.sync.is_coarse_freq_correction = !settings->UseOFDMDisableCoarseFreq();
  // radio switcher
  m_radio_switcher = std::make_shared<CBasicRadioSwitcher>(
      transmissionNode,
      [&](const DAB_Parameters& params, std::string_view channel_name) -> auto
      {
        auto instance = std::make_shared<BasicRadio>(params, m_settings->GetRadioTotalThreads());
        attach_audio_pipeline_to_radio(instance);
        return instance;
      });

  // ofdm input
  m_device_output_buffer = std::make_shared<ThreadedRingBuffer<UTILS::RawIQ>>(
      settings->DataBlockSize() * sizeof(UTILS::RawIQ));
  auto ofdm_convert_raw_iq = std::make_shared<OFDM_Convert_RawIQ>();
  ofdm_convert_raw_iq->set_input_stream(m_device_output_buffer);
  m_ofdm_block->set_input_stream(ofdm_convert_raw_iq);
  // connect ofdm to radio_switcher
  m_ofdm_to_radio_buffer =
      std::make_shared<ThreadedRingBuffer<viterbi_bit_t>>(dab_params.nb_frame_bits * 2);
  m_ofdm_block->set_output_stream(m_ofdm_to_radio_buffer);
  m_radio_switcher->set_input_stream(m_ofdm_to_radio_buffer);
  // device to ofdm
  auto device_list = std::make_shared<DEVICE::CDeviceList>(m_settings);
  m_deviceSource = std::make_shared<CDeviceSource>(
      [&](std::shared_ptr<DEVICE::CDevice> device)
      {
        m_radio_switcher->flush_input_stream();
        if (device == nullptr)
          return;
        if (m_settings->TunerAutoGain())
        {
          device->SetAutoGain();
        }
        else
        {
          device->SetNearestGain(m_settings->GetTunerManualGain());
        }
        device->SetDataCallback(
            [&](tcb::span<const uint8_t> bytes)
            {
              constexpr size_t BYTES_PER_SAMPLE = sizeof(UTILS::RawIQ);
              const size_t total_bytes = bytes.size() - (bytes.size() % BYTES_PER_SAMPLE);
              const size_t total_samples = total_bytes / BYTES_PER_SAMPLE;
              auto raw_iq =
                  tcb::span(reinterpret_cast<const UTILS::RawIQ*>(bytes.data()), total_samples);
              const size_t total_read_samples = m_device_output_buffer->write(raw_iq);
              const size_t total_read_bytes = total_read_samples * BYTES_PER_SAMPLE;
              return total_read_bytes;
            });
        device->SetFrequencyChangeCallback([&](const std::string& label, const uint32_t freq)
                                           { m_radio_switcher->switch_instance(label, freq); });
        device->SetCenterFrequency(m_settings->DeviceLastFrequency());
      });

  m_thread_select_default_tuner = nullptr;

  const size_t default_device_index = m_settings->DeviceDefaultIndex();
  //m_thread_select_default_tuner = std::make_unique<std::thread>([&, device_list,
  //                                                               default_device_index]() {
  device_list->Refresh();
  size_t total_descriptors = 0;
  {
    auto lock = std::unique_lock(device_list->GetMutexDeviceInfos());
    auto descriptors = device_list->GetDeviceList();
    total_descriptors = descriptors.size();
  }
  if (default_device_index >= total_descriptors)
  {
    fprintf(stderr, "ERROR: Device index is greater than the number of devices (%zu >= %zu)\n",
            default_device_index, total_descriptors);
    return;
  }
  auto device = device_list->GetDevice(default_device_index);
  if (device == nullptr)
    return;
  m_deviceSource->set_device(device);
  //});

  m_thread_ofdm_run = std::make_unique<std::thread>(
      [&]()
      {
        m_ofdm_block->run(m_settings->DataBlockSize());
        fprintf(stderr, "ofdm thread finished\n");
        if (m_ofdm_to_radio_buffer != nullptr)
          m_ofdm_to_radio_buffer->close();
      });
  m_thread_radio_switcher = std::make_unique<std::thread>(
      [&]()
      {
        m_radio_switcher->run();
        fprintf(stderr, "radio_switcher thread finished\n");
      });

  const std::vector<std::string> usedSettingValues = {"todo"};

  m_callbackSettingsChangeID = m_settings->SetSettingsChangeCallback(
      usedSettingValues, [&](const std::string& id, const kodi::addon::CSettingValue& settingValue)
      { SetSettingsChangeCallback(id, settingValue); });
}

CInputstreamTypeDAB::~CInputstreamTypeDAB()
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Inputstream instance destroyed");

  m_device_output_buffer->close();
  m_ofdm_to_radio_buffer->close();
  if (m_thread_select_default_tuner != nullptr)
    m_thread_select_default_tuner->join();
  m_thread_ofdm_run->join();
  m_thread_radio_switcher->join();
  m_ofdm_block = nullptr;
  m_radio_switcher = nullptr;

  m_settings->ClearSettingsChangeCallback(m_callbackSettingsChangeID);
}

bool CInputstreamTypeDAB::Open(unsigned int uniqueId,
                               unsigned int frequency,
                               unsigned int subchannel,
                               IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__,
             "Open DAB/DAB+ stream on frequency {} MHz and subchannel {}",
             float(frequency) / 1000 / 1000, subchannel);

  m_PTSNext = 0;

  auto it = std::find_if(block_frequencies.begin(), block_frequencies.end(),
                         [&](const auto& value) { return value.freq == frequency; });
  if (it == block_frequencies.end())
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Frequency {} not relates to DAB/DAB+ stream", frequency);
    return false;
  }

  if (!m_deviceSource->get_device())
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "No device given");
    return false;
  }

  if (m_frequency != it->freq)
    m_deviceSource->get_device()->SetCenterFrequency(it->name, it->freq);

  m_uniqueId = uniqueId;
  m_frequency = frequency;
  m_subchannel = subchannel;
  AllocateDemuxPacket = std::move(allocPacket);

  return true;
}

void CInputstreamTypeDAB::Close()
{
  auto radio = m_radio_switcher->get_instance();
  auto& db = radio->GetDatabase();

  for (auto& subchannel : db.subchannels)
  {
    auto* audio_channel = radio->Get_Audio_Channel(subchannel.id);
    if (audio_channel)
    {
      auto& controls = audio_channel->GetControls();
      controls.StopAll();
    }
  }

  m_audioPipeline->set_active_source(AUDIO_ID_UNDEFINED);
  m_radio_switcher->flush_input_stream();
}

bool CInputstreamTypeDAB::GetStreamIds(std::vector<unsigned int>& ids)
{
  const auto radio = m_radio_switcher->get_instance();

  // Check signal becomes present and wait 2 seconds about.
  int count = 20;
  while (!radio->Ready() && count-- > 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (count == 0)
  {
    UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__,
               "Failed to get signal for frequency {} MHz about DAB/DAB+ subchannel {}",
               float(m_frequency) / 1000 / 1000, m_subchannel);

    return false;
  }

  // Add the two used stream id's, first about adio where use the sub channel id and
  // second about "Dynamic Label" (e.g. like RDS on FM radio) where given with ID3Tag to Kodi.
  ids.push_back(m_uniqueId);
  ids.push_back(STREAM_ID_ID3TAG);

  return true;
}

bool CInputstreamTypeDAB::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  // Set the stream info about audio stream. The data type is on all channels the same.
  if (streamid == m_uniqueId)
  {
    stream.SetStreamType(INPUTSTREAM_TYPE_AUDIO);
    stream.SetCodecName("pcm_f32le");
    stream.SetChannels(2);
    stream.SetSampleRate(STREAM_AUDIO_SAMPLERATE);
    stream.SetBitsPerSample(32);
    stream.SetBitRate(stream.GetSampleRate() * stream.GetChannels() * stream.GetBitsPerSample());

    return true;
  }

  // Set the stream info about ID3 tag stram where gives "Dynamic Label" to Kodi.
  if (streamid == STREAM_ID_ID3TAG)
  {
    stream.SetStreamType(INPUTSTREAM_TYPE_ID3);
    stream.SetCodecName("id3");

    return true;
  }

  return false;
}

//! @note This function currently unused, in Kodi seems this type only be used to select subtitles
//! in stream.
//!
//! @todo Maybe add in Kodi's Inputstream/PVR a way to make channel switch by stream id's and not
//! with recreate of inputstream instances. About here on DAB can be this nice as one stream brings
//! some channels and then can switch without any delay or audio interruption.
void CInputstreamTypeDAB::EnableStream(int streamid, bool enable)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Enable streamid: {:X}, enable: {}", streamid,
             enable ? "yes" : "no");
}

bool CInputstreamTypeDAB::OpenStream(int streamid)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Open stream id: {:X}", streamid);

  if (streamid == m_uniqueId)
  {
    auto* audio_channel = m_radio_switcher->get_instance()->Get_Audio_Channel(m_subchannel);
    if (!audio_channel)
    {
      UTILS::log(UTILS::LOGLevel::ERROR, __src_loc_cur__, "No audio channel given");
      return false;
    }

    audio_channel->GetControls().RunAll();
    m_audioPipeline->set_active_source(m_uniqueId);

    return true;
  }

  if (streamid == STREAM_ID_ID3TAG)
  {
    //! @todo Implement RDS support
    return true;
  }

  return false;
}

void CInputstreamTypeDAB::DemuxReset()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

void CInputstreamTypeDAB::DemuxAbort()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

void CInputstreamTypeDAB::DemuxFlush()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

DEMUX_PACKET* CInputstreamTypeDAB::DemuxRead()
{
  double duration =
      double(STREAM_TIME_BASE) / double(STREAM_AUDIO_SAMPLERATE / STREAM_FRAMES_PER_BUFFER);

  DEMUX_PACKET* packet;
  if (!m_unusedPacket)
  {
    packet = AllocateDemuxPacket(STREAM_FRAMES_PER_BUFFER * sizeof(float) * 2);
  }
  else
  {
    packet = m_unusedPacket;
    m_unusedPacket = nullptr;
  }

  const auto write_buffer = tcb::span<Frame<float>>(reinterpret_cast<Frame<float>*>(packet->pData),
                                                    STREAM_FRAMES_PER_BUFFER);

  bool ret = m_audioPipeline->source_to_sink(write_buffer, STREAM_AUDIO_SAMPLERATE);
  if (ret)
  {
    packet->iStreamId = m_uniqueId;
    packet->iSize = STREAM_FRAMES_PER_BUFFER * sizeof(float) * 2;
    packet->duration = STREAM_PACKET_DURATION;
    packet->pts = m_PTSNext;
    packet->dts = m_PTSNext;

    m_PTSNext = m_PTSNext + STREAM_PACKET_DURATION;
  }
  else
  {
    //fprintf(stderr, "%s %f\n", __PRETTY_FUNCTION__, m_PTSNext);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_unusedPacket = packet;

    return AllocateDemuxPacket(0);
  }

  return packet;
}

bool CInputstreamTypeDAB::GetTimes(kodi::addon::InputstreamTimes& times)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return false;
}

void CInputstreamTypeDAB::attach_audio_pipeline_to_radio(
    const std::shared_ptr<BasicRadio>& basic_radio)
{
  if (m_audioPipeline == nullptr)
    return;
  basic_radio->On_Audio_Channel().Attach(
      [this](subchannel_id_t subchannel_id, Basic_Audio_Channel& channel)
      {
        auto& controls = channel.GetControls();

        const CChannelId id(m_frequency, subchannel_id, Modulation::DAB);
        auto audio_source = std::make_shared<AudioPipelineSource>(id.Id());
        m_audioPipeline->add_source(audio_source);
        channel.OnAudioData().Attach(
            [this, &controls, audio_source](BasicAudioParams params, tcb::span<const uint8_t> buf)
            {
              if (!controls.GetIsPlayAudio())
              {
                return;
              }
              auto frame_ptr = reinterpret_cast<const Frame<int16_t>*>(buf.data());
              const size_t total_frames = buf.size() / sizeof(Frame<int16_t>);
              auto frame_buf = tcb::span(frame_ptr, total_frames);
              const bool is_blocking = m_audioPipeline->is_active();
              audio_source->write(frame_buf, float(params.frequency), is_blocking);
            });
      });
}

void CInputstreamTypeDAB::SetSettingsChangeCallback(const std::string& id,
                                                    const kodi::addon::CSettingValue& settingValue)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

} // namespace INSTANCE
} // namespace RTLRADIO
