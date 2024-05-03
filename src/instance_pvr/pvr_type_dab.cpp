/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "pvr_type_dab.h"

#include "audio/audio_pipeline.h"
#include "dab/dab_misc_info.h"
#include "dsp_dab/app_ofdm_blocks.h"
#include "dsp_dab/block_frequencies.h"
#include "dsp_dab/process_lib/basic_radio/basic_audio_channel.h"
#include "dsp_dab/process_lib/basic_radio/basic_radio.h"
#include "dsp_dab/process_lib/basic_scraper/basic_scraper.h"
#include "dsp_dab/process_lib/dab/constants/dab_parameters.h"
#include "dsp_dab/process_lib/dab/database/dab_database.h"
#include "dsp_dab/process_lib/dab/database/dab_database_entities.h"
#include "dsp_dab/process_lib/dab/database/dab_database_types.h"
#include "pvr_radio_switcher_i.h"
#include "settings/settings.h"
#include "utils/app_io_buffers.h"
#include "utils/log.h"

#include <kodi/tools/StringUtils.h>

namespace RTLRADIO
{
namespace INSTANCE
{

class Radio_Instance
{
private:
  const std::string m_name;
  BasicRadio m_radio;
  //BasicRadioViewController m_view_controller;

public:
  template<typename... T>
  Radio_Instance(std::string_view name, T... args) : m_name(name), m_radio(std::forward<T>(args)...)
  {
  }
  auto& get_radio() { return m_radio; }
  //auto& get_view_controller() { return m_view_controller; }
  std::string_view get_name() const { return m_name; }
};

class CBasicRadioSwitcher : public IPVRRadioSwitcher
{
public:
  template<typename F>
  CBasicRadioSwitcher(int transmission_mode, F&& create_instance)
    : m_dab_params(get_dab_parameters(transmission_mode)), m_create_instance(create_instance)
  {
    m_bits_buffer.resize(m_dab_params.nb_frame_bits);
  }

  void set_input_stream(std::shared_ptr<InputBuffer<viterbi_bit_t>> stream)
  {
    fprintf(stderr, "---> %s\n", __func__);
    m_input_stream = stream;
  }

  void flush_input_stream() override { m_flush_reads = 5; }

  void SwitchInstance(std::string_view key, const uint32_t freq) override
  {
    fprintf(stderr, "---> %s %s\n", __func__, key.data());
    std::shared_ptr<Radio_Instance> new_instance = nullptr;

    auto lock = std::unique_lock(m_mutex_selected_instance);

    auto res = m_instances.find(std::string(key));
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

  std::shared_ptr<Radio_Instance> get_instance()
  {
    auto lock = std::unique_lock(m_mutex_selected_instance);
    return m_selected_instance;
  }

  void stop_running() { m_running = false; }

  void run()
  {
    fprintf(stderr, "-1111--> \n");
    if (m_input_stream == nullptr)
      return;

    fprintf(stderr, "-2222--> \n");
    while (m_running)
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
      m_selected_instance->get_radio().Process(m_bits_buffer);
      //auto& db = m_selected_instance->get_radio().GetDatabase();
      //fprintf(stderr, "---> Services (%i) %i\n", db.services.size(), m_bits_buffer.size());
      //for (auto& service : db.services)
      //{
      //  fprintf(stderr, "   %s\n", service.label.c_str());
      //}
      //fprintf(stderr, "---> subchannels (%i)\n", db.subchannels.size());
      //for (auto& subchannel : db.subchannels)
      //{
      //  auto* channel = m_selected_instance->get_radio().Get_Audio_Channel(subchannel.id);
      //  if (channel == nullptr)
      //    continue;

      //  fprintf(stderr, "  - %s\n", channel->GetDynamicLabel().data());
      //}
    }
  }

private:
  DAB_Parameters m_dab_params;
  std::shared_ptr<InputBuffer<viterbi_bit_t>> m_input_stream = nullptr;
  std::vector<viterbi_bit_t> m_bits_buffer;
  std::map<std::string, std::shared_ptr<Radio_Instance>> m_instances;
  std::shared_ptr<Radio_Instance> m_selected_instance = nullptr;
  std::mutex m_mutex_selected_instance;
  size_t m_flush_reads = 0;
  std::function<std::shared_ptr<Radio_Instance>(const DAB_Parameters&, std::string_view)>
      m_create_instance;
  std::atomic_bool m_running = true;
};

template<typename T, typename F>
static T* find_by_callback(std::vector<T>& vec, F&& func)
{
  for (auto& e : vec)
  {
    if (func(e))
      return &e;
  }
  return nullptr;
}

CPVRTypeDAB::CPVRTypeDAB(const std::shared_ptr<SETTINGS::CSettings>& settings,
                         std::shared_ptr<ThreadedRingBuffer<UTILS::RawIQ>> deviceOutputBuffer)
  : IPVRType(Modulation::DAB, settings, deviceOutputBuffer)
{
  m_signalProps.filter = false; // Never apply the filter here
  m_signalProps.samplerate = 2048 KHz;
  m_signalProps.bandwidth = 1712 KHz;
  m_signalProps.lowcut = -780 KHz;
  m_signalProps.highcut = 780 KHz;
  m_signalProps.offset = 0;
}

CPVRTypeDAB::~CPVRTypeDAB()
{
  Deinit();
}

bool CPVRTypeDAB::Scan(std::vector<ChannelProps>& channelsFound,
                       const std::function<void(const uint32_t freq)>&& funcSetCenterFrequency,
                       const std::function<bool()>&& funcScanCancelled,
                       const std::function<void(unsigned int)>&& funcScanPercentage,
                       const std::function<void(const std::string& channel)>&& funcScanChannel,
                       const std::function<void(const ChannelProps&)>&& funcScanChannelFound)
{
  UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__, "Starting DAB/DAB+ channel scan");

  // Get needed settings values
  const auto transmissionNode = m_settings->TransmissionNode();
  const auto OFDMTotalThreads = m_settings->GetOFDMTotalThreads();
  const auto useOFDMDisableCoarseFreq = m_settings->UseOFDMDisableCoarseFreq();
  const auto ofdm_block_size = m_settings->DataBlockSize();
  const auto radioTotalThreads = m_settings->GetRadioTotalThreads();

  // Get in settings selected DAB parameter values
  const auto dab_params = get_dab_parameters(transmissionNode);

  // ofdm
  m_OFDMBlock = std::make_shared<OFDM_Block>(transmissionNode, radioTotalThreads);
  auto& ofdm_config = m_OFDMBlock->get_ofdm_demod().GetConfig();
  ofdm_config.sync.is_coarse_freq_correction = !useOFDMDisableCoarseFreq;

  // ofdm input
  auto ofdm_convert_raw_iq = std::make_shared<OFDM_Convert_RawIQ>();
  ofdm_convert_raw_iq->set_input_stream(m_deviceOutputBuffer);
  m_OFDMBlock->set_input_stream(ofdm_convert_raw_iq);

  m_radioSwitcher = std::make_shared<CBasicRadioSwitcher>(
      transmissionNode,
      [radioTotalThreads](const DAB_Parameters& params, std::string_view channel_name) -> auto {
        return std::make_shared<Radio_Instance>(channel_name, params, radioTotalThreads);
      });

  // connect ofdm to radio_switcher
  m_OFDMToRadioBuffer =
      std::make_shared<ThreadedRingBuffer<viterbi_bit_t>>(dab_params.nb_frame_bits * 2);
  m_OFDMBlock->set_output_stream(m_OFDMToRadioBuffer);
  m_radioSwitcher->set_input_stream(m_OFDMToRadioBuffer);

  auto thread_ofdm_run = std::thread([&, ofdm_block_size]() {
    m_OFDMBlock->run(ofdm_block_size);
    fprintf(stderr, "ofdm thread finished\n");
    if (m_OFDMToRadioBuffer != nullptr)
      m_OFDMToRadioBuffer->close();
  });
  auto thread_radio_switcher = std::thread([&]() {
    m_radioSwitcher->run();
    fprintf(stderr, "radio_switcher thread finished\n");
  });

  std::vector<Service*> service_list;

  const float percentStep = 100.0f / float(block_frequencies.size());
  float currentPercent = 0.0f;
  for (const auto& freq : block_frequencies)
  {
    if (funcScanCancelled())
      break;

    //if (freq.name != "11A")
    //  continue;

    funcScanChannel(freq.name);

    funcSetCenterFrequency(freq.freq);
    m_radioSwitcher->SwitchInstance(freq.name, freq.freq);

    auto& radio = m_radioSwitcher->get_instance()->get_radio();

    bool found = false;
    int cnt = 12;
    while (cnt--)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto lock = std::scoped_lock(radio.GetMutex());
      auto& db = radio.GetDatabase();
      auto& miscInfo = radio.GetMiscInfo();

      if (miscInfo.cif_counter.GetTotalCount() > 0)
      {
        UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__,
                   "Scan found DAB/DAB+ channel {} contains data with {} subchannels, processing "
                   "further...",
                   freq.name, db.services.size());
        found = true;
        break;
      }
    }

    if (found)
    {
      cnt = 20;
      while (cnt--)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto lock = std::scoped_lock(radio.GetMutex());

        auto& db = radio.GetDatabase();

        found = true;
        for (auto& service : db.services)
        {
          if (!service.is_complete)
          {
            found = false;
            break;
          }
        }
      }

      if (found)
      {
        auto lock = std::scoped_lock(radio.GetMutex());

        auto& db = radio.GetDatabase();

        uint32_t subchannelnumber = 1;
        for (auto& subchannel : db.subchannels)
        {
          auto* service_component =
              find_by_callback(db.service_components, [&subchannel](const auto& e) {
                return e.subchannel_id == subchannel.id;
              });
          Service* service = nullptr;
          if (service_component)
          {
            service = find_by_callback(db.services, [&service_component](const auto& e) {
              return e.reference == service_component->service_reference;
            });
          }
          std::string service_label = service ? service->label : "";
          std::string ensemble_label = db.ensemble.label;
          kodi::tools::StringUtils::Trim(service_label);
          kodi::tools::StringUtils::Trim(ensemble_label);

          UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__,
                     "DAB/DAB+ subchannel found: {} ({}) (channel: {}/{})", service_label,
                     db.ensemble.label, subchannel.id, freq.name);

          ChannelProps props(freq.freq, subchannel.id, Modulation::DAB);

          props.channelnumber = freq.number;
          props.name = service_label;
          props.usereditname = service_label;
          props.provider = kodi::tools::StringUtils::Trim(ensemble_label);
          props.logourl = "";
          props.userlogourl = props.logourl;
          props.programmtype =
              service ? ProgrammType(service->programme_type) : ProgrammType::UNDEFINED;
          //props.country = service
          //                                 ? CountryCode(service->country_id)
          //                                 : ProgrammType::UNDEFINED;
          props.language = LanguageCode(service->language);
          props.transportmode = TransportMode(service_component->transport_mode);
          props.datatype = "AAC";
          props.visible = (TransportMode(service_component->transport_mode) ==
                           TransportMode::STREAM_MODE_AUDIO);
          funcScanChannelFound(props);

          auto it =
              std::find_if(channelsFound.begin(), channelsFound.end(), [props](const auto& entry) {
                if (props.name != entry.name)
                  return false;
                if (props.subchannelnumber > 0 && entry.subchannelnumber > 0 &&
                    props.subchannelnumber != entry.subchannelnumber)
                  return false;
                if (props.country != CountryCode::UNDEFINED &&
                    entry.country != CountryCode::UNDEFINED && props.country != entry.country)
                  return false;
                if (props.language != LanguageCode::UNDEFINED &&
                    entry.language != LanguageCode::UNDEFINED && props.language != entry.language)
                  return false;
                return true;
              });
          if (it != channelsFound.end())
          {
            const auto& fb = it->fallbacks;
            auto it2 = std::find_if(fb.begin(), fb.end(), [props](const auto& entry) {
              return (entry.frequency == props.frequency) &&
                     (entry.modulation == props.modulation) &&
                     (entry.service_id == props.subchannelnumber);
            });

            if (it2 == fb.end())
            {
              it->fallbacks.emplace_back(props.frequency, props.modulation, props.subchannelnumber);
            }
          }
          else
          {
            channelsFound.push_back(props);
          }
        }
      }
    }

    currentPercent += percentStep;
    funcScanPercentage(uint32_t(currentPercent));
  }

  std::sort(service_list.begin(), service_list.end(),
            [](const auto* a, const auto* b) { return (a->label.compare(b->label) < 0); });

  //for (auto& service : service_list)
  //{
  //  UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__,
  //             "DAB/DAB+ subchannel found: {} (channel: {})");
  //}

  m_radioSwitcher->flush_input_stream();

  m_OFDMBlock->stop_running();
  thread_ofdm_run.join();
  m_radioSwitcher->stop_running();
  thread_radio_switcher.join();

  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Finished DAB/DAB+ channel scan");

  return true;
}

bool CPVRTypeDAB::Init()
{
  // Get needed settings values
  const auto transmissionNode = m_settings->TransmissionNode();
  const auto OFDMTotalThreads = m_settings->GetOFDMTotalThreads();
  const auto useOFDMDisableCoarseFreq = m_settings->UseOFDMDisableCoarseFreq();
  const auto radioTotalThreads = m_settings->GetRadioTotalThreads();

  // Get in settings selected DAB parameter values
  const auto dab_params = get_dab_parameters(transmissionNode);

  // ofdm
  m_OFDMBlock = std::make_shared<OFDM_Block>(transmissionNode, radioTotalThreads);
  auto& ofdm_config = m_OFDMBlock->get_ofdm_demod().GetConfig();
  ofdm_config.sync.is_coarse_freq_correction = !useOFDMDisableCoarseFreq;

  // ofdm input
  auto ofdm_convert_raw_iq = std::make_shared<OFDM_Convert_RawIQ>();
  ofdm_convert_raw_iq->set_input_stream(m_deviceOutputBuffer);
  m_OFDMBlock->set_input_stream(ofdm_convert_raw_iq);

  // radio switcher
  m_radioSwitcher = std::make_shared<CBasicRadioSwitcher>(
      m_settings->TransmissionNode(),
      [&](const DAB_Parameters& params, std::string_view channel_name) -> auto {
        fprintf(stderr, "--CBasicRadioSwitcher-> %s\n", __func__);
        auto instance = std::make_shared<Radio_Instance>(channel_name, params,
                                                         m_settings->GetRadioTotalThreads());
        auto& radio = instance->get_radio();
        //AttachAudioPipelineToRadio(m_audioPipeline, radio);
        //if (m_settings->ScraperEnable())
        {
          //auto dir = fmt::format("{}/{}", m_settings->ScraperOutput(), channel_name);
          //auto scraper = std::make_shared<BasicScraper>(dir);
          //fprintf(stderr, "basic_scraper is writing to folder '%s'\n", dir.c_str());
          //BasicScraper::attach_to_radio(scraper, radio);
          //if (!m_settings->ScraperDisableAuto())
          {
            radio.On_Audio_Channel().Attach(
                [](subchannel_id_t subchannel_id, Basic_Audio_Channel& channel) {
                  auto& controls = channel.GetControls();
                  controls.SetIsDecodeAudio(true);
                  controls.SetIsDecodeData(true);
                  controls.SetIsPlayAudio(false);
                });
          }
        }
        return instance;
      });

  //// connect ofdm to radio_switcher
  //m_OFDMToRadioBuffer =
  //    std::make_shared<ThreadedRingBuffer<viterbi_bit_t>>(dab_params.nb_frame_bits * 2);
  //m_OFDMBlock->set_output_stream(m_OFDMToRadioBuffer);
  //m_radioSwitcher->set_input_stream(m_OFDMToRadioBuffer);

  return false;
}

void CPVRTypeDAB::Deinit()
{
  //m_deviceOutputBuffer->close();
  //ofdm_to_radio_buffer->close();
  //thread_ofdm_run.join();
  //thread_radio_switcher.join();
  m_OFDMBlock = nullptr;
  m_radioSwitcher = nullptr;

  return;
}

std::shared_ptr<IPVRRadioSwitcher> CPVRTypeDAB::GetRadioSwitcher()
{
  return std::dynamic_pointer_cast<IPVRRadioSwitcher>(m_radioSwitcher);
}

void CPVRTypeDAB::AttachAudioPipelineToRadio(std::shared_ptr<AudioPipeline> audio_pipeline,
                                             BasicRadio& basic_radio)
{
  if (audio_pipeline == nullptr)
    return;
  basic_radio.On_Audio_Channel().Attach(
      [audio_pipeline](subchannel_id_t subchannel_id, Basic_Audio_Channel& channel) {
        auto& controls = channel.GetControls();
        auto audio_source = std::make_shared<AudioPipelineSource>();
        audio_pipeline->add_source(audio_source);
        channel.OnAudioData().Attach([&controls, audio_source, audio_pipeline](
                                         BasicAudioParams params, tcb::span<const uint8_t> buf) {
          if (!controls.GetIsPlayAudio())
            return;
          auto frame_ptr = reinterpret_cast<const Frame<int16_t>*>(buf.data());
          const size_t total_frames = buf.size() / sizeof(Frame<int16_t>);
          auto frame_buf = tcb::span(frame_ptr, total_frames);
          const bool is_blocking = audio_pipeline->get_sink() != nullptr;
          audio_source->write(frame_buf, float(params.frequency), is_blocking);
        });
      });
}

} // namespace INSTANCE
} // namespace RTLRADIO
