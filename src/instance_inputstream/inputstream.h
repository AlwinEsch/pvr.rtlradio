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
namespace INSTANCE
{

class CInputstreamBase;

class ATTR_DLL_LOCAL CInputstream : public kodi::addon::CInstanceInputStream
{
public:
  CInputstream(const kodi::addon::IInstanceInfo& instance, CInputstreamBase& base);
  ~CInputstream();

  void GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities) override;
  bool Open(const kodi::addon::InputstreamProperty& props) override;
  void Close() override;

  bool IsRealTimeStream() override { return true; }

  bool GetStreamIds(std::vector<unsigned int>& ids) override;
  bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) override;
  void EnableStream(int streamid, bool enable) override;
  bool OpenStream(int streamid) override;
  void DemuxReset() override;
  void DemuxAbort() override;
  void DemuxFlush() override;
  DEMUX_PACKET* DemuxRead() override;

private:
  CInputstreamBase& m_base;
};

} // namespace INSTANCE
} // namespace RTLRADIO
