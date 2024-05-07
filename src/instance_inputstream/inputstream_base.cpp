/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "inputstream_base.h"

#include "exception_control/exception.h"
#include "inputstream_type_dab.h"
#include "inputstream_type_fm.h"
#include "inputstream_type_hd.h"
#include "inputstream_type_mw.h"
#include "inputstream_type_wx.h"
#include "settings/settings.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstreamBase::CInputstreamBase(std::shared_ptr<SETTINGS::CSettings> settings)
  : m_settings(settings), m_audioPipeline(std::make_shared<AudioPipeline>())
{
}

CInputstreamBase::~CInputstreamBase()
{
  m_audioPipeline = nullptr;
}

bool CInputstreamBase::Open(const std::string& url,
                            const std::string& mimetype,
                            unsigned int uniqueId,
                            unsigned int frequency,
                            unsigned int subchannel,
                            Modulation modulation,
                            IInputstreamType::AllocateDemuxPacketCB allocPacket)
{
  if (m_active)
  {
    UTILS::log(UTILS::LOGLevel::FATAL, __src_loc_cur__,
               "Open should never be called if a stream still active!");
    return false;
  }

  try
  {
    bool ret = false;
    std::unique_ptr<IInputstreamType> activeType;
    if (m_modulation != modulation || !m_activeType)
    {
      m_audioPipeline->clear_sources();

      switch (modulation)
      {
        case Modulation::DAB:
          if (m_settings->ModulationDABEnabled())
            activeType = std::make_unique<CInputstreamTypeDAB>(m_settings, m_audioPipeline);
          else
            return false;
          break;
        case Modulation::FM:
          if (m_settings->ModulationFMEnabled())
            activeType = std::make_unique<CInputstreamTypeFM>(m_settings, m_audioPipeline);
          else
            return false;
          break;
        case Modulation::HD:
          if (m_settings->ModulationHDEnabled())
            activeType = std::make_unique<CInputstreamTypeHD>(m_settings, m_audioPipeline);
          else
            return false;
          break;
        case Modulation::MW:
          if (m_settings->ModulationMWEnabled())
            activeType = std::make_unique<CInputstreamTypeMW>(m_settings, m_audioPipeline);
          else
            return false;
          break;
        case Modulation::WX:
          if (m_settings->ModulationWXEnabled())
            activeType = std::make_unique<CInputstreamTypeWX>(m_settings, m_audioPipeline);
          else
            return false;
          break;
        default:
          UTILS::log(UTILS::LOGLevel::FATAL, __src_loc_cur__,
                     "Creation called with an unknown type: {}", uint8_t(modulation));
          return false;
      }
    }
    else
    {
      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "Reusing previous stream type");
      activeType = std::move(m_activeType);
      m_activeType = nullptr;
    }

    if (!activeType->Open(uniqueId, frequency, subchannel, std::move(allocPacket)))
    {
      m_url.clear();
      m_mimetype.clear();
      m_uniqueId = 0;
      m_frequency = 0;
      m_subchannel = 0;
      m_modulation = Modulation::UNDEFINED;
      return false;
    }

    m_url = url;
    m_mimetype = mimetype;
    m_uniqueId = uniqueId;
    m_frequency = frequency;
    m_subchannel = subchannel;
    m_modulation = modulation;

    m_activeType = std::move(activeType);
    m_active = true;
  }
  catch (sqlite_exception const& dbex)
  {
    return EXCEPTION::HandleDBException(__src_loc_cur__, dbex, false);
  }
  catch (std::exception& ex)
  {
    return EXCEPTION::HandleStdException(__src_loc_cur__, ex, false);
  }
  catch (...)
  {
    return EXCEPTION::HandleGeneralException(__src_loc_cur__, false);
  }

  return true;
}

void CInputstreamBase::Close()
{
  try
  {
    //! @note Kodi calls Close() several times, use m_active to make only one call to modulation target
    if (m_active)
    {
      if (m_activeType)
        m_activeType->Close();

      m_active = false;
    }
  }
  catch (sqlite_exception const& dbex)
  {
    EXCEPTION::HandleDBException(__src_loc_cur__, dbex);
    return;
  }
  catch (std::exception& ex)
  {
    EXCEPTION::HandleStdException(__src_loc_cur__, ex);
    return;
  }
  catch (...)
  {
    EXCEPTION::HandleGeneralException(__src_loc_cur__);
    return;
  }
}

bool CInputstreamBase::GetStreamIds(std::vector<unsigned int>& ids)
{
  return m_activeType ? m_activeType->GetStreamIds(ids) : false;
}

bool CInputstreamBase::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  return m_activeType ? m_activeType->GetStream(streamid, stream) : false;
}

void CInputstreamBase::EnableStream(int streamid, bool enable)
{
  if (m_activeType)
    m_activeType->Close();
}

bool CInputstreamBase::OpenStream(int streamid)
{
  return m_activeType ? m_activeType->OpenStream(streamid) : false;
}

void CInputstreamBase::DemuxReset()
{
  if (m_activeType)
    m_activeType->DemuxReset();
}

void CInputstreamBase::DemuxAbort()
{
  if (m_activeType)
    m_activeType->DemuxAbort();
}

void CInputstreamBase::DemuxFlush()
{
  if (m_activeType)
    m_activeType->DemuxFlush();
}

DEMUX_PACKET* CInputstreamBase::DemuxRead()
{
  return m_activeType ? m_activeType->DemuxRead() : nullptr;
}

bool CInputstreamBase::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return m_activeType ? m_activeType->GetTimes(times) : false;
}

} // namespace INSTANCE
} // namespace RTLRADIO
