/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/addon-instance/Inputstream.h>

namespace RTLRADIO
{

namespace SETTINGS
{
class CSettings;
} // namespace SETTINGS

namespace INSTANCE
{

class CInputstream : public kodi::addon::CInstanceInputStream
{
public:
  CInputstream(const kodi::addon::IInstanceInfo& instance,
               std::shared_ptr<SETTINGS::CSettings> settings);
  ~CInputstream();

  void GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities) override;
  bool Open(const kodi::addon::InputstreamProperty& props) override;
  void Close() override;
  int ReadStream(uint8_t* buffer, unsigned int bufferSize) override;
  int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  int64_t PositionStream() override;
  int64_t LengthStream() override;
  int GetBlockSize() override;

  bool GetStreamIds(std::vector<unsigned int>& ids) override;
  bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) override;
  void EnableStream(int streamid, bool enable) override;
  bool OpenStream(int streamid) override;
  void DemuxReset() override;
  void DemuxAbort() override;
  void DemuxFlush() override;
  DEMUX_PACKET* DemuxRead() override;
  bool DemuxSeekTime(double time, bool backwards, double& startpts) override;

private:
  void SetSettingsChangeCallback(const std::string& id,
                                 const kodi::addon::CSettingValue& settingValue);

  std::shared_ptr<SETTINGS::CSettings> m_settings;
  int m_callbackSettingsChangeID{-1};
};

} // namespace INSTANCE
} // namespace RTLRADIO
