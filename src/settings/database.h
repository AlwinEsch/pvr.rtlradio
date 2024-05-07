/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "database_control.h"
#include "props.h"

namespace RTLRADIO
{
namespace SETTINGS
{
namespace DB
{

using callback_GetChannels = std::function<void(const struct ChannelProps& channel)>;
using callback_GetProviders = std::function<void(const struct ProviderProps& provider)>;

class CConPoolPVR : public CConPool
{
public:
  CConPoolPVR(const std::string& connstr, size_t poolsize, int flags)
    : CConPool(connstr, poolsize, flags)
  {
  }
  ~CConPoolPVR() = default;

  std::string GetBaseDBName() const override { return "channels.db"; }
  void InitDatabase(sqlite3* instance) override;

  static void SetLastScanTime(sqlite3* instance, time_t time, size_t channelsfound);
  static time_t GetLastScanTime(sqlite3* instance);

  static int GetProvidersCount(sqlite3* instance);
  static void GetProviders(sqlite3* instance, callback_GetProviders const& callback);
  static bool ProviderExists(sqlite3* instance, unsigned int uniqueId);

  static int GetChannelsCount(sqlite3* instance);
  static void GetChannels(sqlite3* instance,
                          enum Modulation modulation,
                          bool prependnumber,
                          callback_GetChannels const& callback);
  static bool ChannelExists(sqlite3* instance, unsigned int uniqueId);
  static void ChannelDelete(sqlite3* instance, unsigned int uniqueId);
  static void ChannelRename(sqlite3* instance, unsigned int uniqueId, const std::string& newName);
  static void GetDeletedChannels(sqlite3* instance, std::vector<ChannelProps>& channels);

  static void ChannelScanSet(sqlite3* instance,
                             const std::vector<ChannelProps>& channels,
                             const std::vector<ProviderProps>& providers,
                             bool autoScan);

private:
  static void GetProviders(sqlite3* instance, std::vector<struct ProviderProps>& providers);
  static void GetChannels(sqlite3* instance, std::vector<struct ChannelProps>& channels);
};

} // namespace DB
} // namespace SETTINGS
} // namespace RTLRADIO
