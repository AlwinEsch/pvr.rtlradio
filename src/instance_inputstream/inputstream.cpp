/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "inputstream.h"

#include "settings/settings.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace INSTANCE
{

CInputstream::CInputstream(const kodi::addon::IInstanceInfo& instance,
                           std::shared_ptr<SETTINGS::CSettings> settings)
  : kodi::addon::CInstanceInputStream(instance), m_settings(settings)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  const std::vector<std::string> usedSettingValues = {"todo"};

  m_callbackSettingsChangeID = m_settings->SetSettingsChangeCallback(
      usedSettingValues,
      [&](const std::string& id, const kodi::addon::CSettingValue& settingValue) {
        SetSettingsChangeCallback(id, settingValue);
      });
}

CInputstream::~CInputstream()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  m_settings->ClearSettingsChangeCallback(m_callbackSettingsChangeID);
}

void CInputstream::GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

bool CInputstream::Open(const kodi::addon::InputstreamProperty& props)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return true;
}

void CInputstream::Close()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

int CInputstream::ReadStream(uint8_t* buffer, unsigned int bufferSize)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return -1;
}

int64_t CInputstream::SeekStream(int64_t position, int whence /* = SEEK_SET*/)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return -1;
}

int64_t CInputstream::PositionStream()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return -1;
}

int64_t CInputstream::LengthStream()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return -1;
}

int CInputstream::GetBlockSize()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return 0;
}

bool CInputstream::GetStreamIds(std::vector<unsigned int>& ids)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return false;
}

bool CInputstream::GetStream(int streamid, kodi::addon::InputstreamInfo& stream)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return false;
}

void CInputstream::EnableStream(int streamid, bool enable)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

bool CInputstream::OpenStream(int streamid)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return false;
}

void CInputstream::DemuxReset()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

void CInputstream::DemuxAbort()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

void CInputstream::DemuxFlush()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

DEMUX_PACKET* CInputstream::DemuxRead()
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return nullptr;
}

bool CInputstream::DemuxSeekTime(double time, bool backwards, double& startpts)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
  return false;
}

void CInputstream::SetSettingsChangeCallback(const std::string& id,
                                             const kodi::addon::CSettingValue& settingValue)
{
  fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

} // namespace INSTANCE
} // namespace RTLRADIO
