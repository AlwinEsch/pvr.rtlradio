/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "database.h"

#include "exception_control/sqlite_exception.h"
#include "props.h"
#include "utils/log.h"

#include <cassert>

namespace RTLRADIO
{
namespace SETTINGS
{
namespace DB
{

void CConPoolPVR::InitDatabase(sqlite3* instance)
{
  // get the database schema version
  //
  int dbversion = execute_scalar_int(__src_loc_cur__, instance, "PRAGMA user_version");

  // SCHEMA VERSION 0 -> VERSION 1
  //
  if (dbversion == 0)
  {
    // clang-format off

    /**
     * @brief table: channel
     */
    execute_non_query(__src_loc_cur__, instance, "DROP TABLE IF EXISTS channel");
    execute_non_query(__src_loc_cur__, instance, R"sql(
CREATE TABLE channel(
    c01_id                INTEGER NOT NULL,
    c02_subchannelnumber  INTEGER NOT NULL,
    c03_modulation        INTEGER NOT NULL,
    c04_frequency         INTEGER NOT NULL,
    c05_channelnumber     INTEGER NOT NULL,
    c06_name              TEXT NOT NULL,
    c07_usereditname      TEXT NOT NULL,
    c08_provider          INTEGER NOT NULL,
    c09_logourl           TEXT NULL,
    c10_userlogourl       TEXT NULL,
    c11_country           TEXT NULL,
    c12_language          TEXT NULL,
    c13_programmtype      INTEGER NULL,
    c14_transportmode     INTEGER NOT NULL,
    c15_mimetype          TEXT NOT NULL,
    c16_fallbacks         BOOLEAN NOT NULL,
    c17_autogain          INTEGER NOT NULL,
    c18_manualgain        INTEGER NOT NULL,
    c19_freqcorrection    INTEGER NOT NULL,
    c20_notpublic         BOOLEAN NOT NULL,
    c21_visible           BOOLEAN NOT NULL,
PRIMARY KEY (c01_id)
);)sql");

    /**
     * @brief table: provider
     */
    execute_non_query(__src_loc_cur__, instance, "DROP TABLE IF EXISTS provider");
    execute_non_query(__src_loc_cur__, instance, R"sql(
CREATE TABLE provider(
    c01_id                INTEGER NOT NULL,
    c02_name              TEXT NOT NULL,
    c03_logourl           TEXT NULL,
    c04_country           TEXT NULL,
    c05_language          TEXT NULL,
PRIMARY KEY (c01_id)
);)sql");

    /**
     * @brief table: deleted channels
     *
     * Used to prevent on auto scans the add of deleted channels.
     * Manual channel scan reset this entries to use again.
     */
    execute_non_query(__src_loc_cur__, instance, "DROP TABLE IF EXISTS deletedchannel");
    execute_non_query(__src_loc_cur__, instance, R"sql(
CREATE TABLE deletedchannel(
    c01_id                INTEGER NOT NULL,
    c02_subchannelnumber  INTEGER NOT NULL,
    c03_modulation        INTEGER NOT NULL,
    c04_frequency         INTEGER NOT NULL,
    c05_provider          INTEGER NOT NULL,
    c06_channelnumber     INTEGER NOT NULL,
    c07_name              TEXT NOT NULL,
PRIMARY KEY (c01_id)
);)sql");

    /**
     * @brief table: channelscan
     */
    execute_non_query(__src_loc_cur__, instance, "DROP TABLE IF EXISTS channelscan");
    execute_non_query(__src_loc_cur__, instance, R"sql(
CREATE TABLE channelscan(
    c01_id                INTEGER NOT NULL,
    c02_time              INTEGER NOT NULL,
    c03_channelsfound     INTEGER NOT NULL,
PRIMARY KEY (c01_id)
);)sql");
    execute_non_query(__src_loc_cur__, instance, "PRAGMA user_version = 1");
    dbversion = 1;

    // clang-format on
  }
}

int CConPoolPVR::GetProvidersCount(sqlite3* instance)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  return execute_scalar_int(__src_loc_cur__, instance,
                            "SELECT count(DISTINCT c08_provider) FROM channel");
}

void CConPoolPVR::GetProviders(sqlite3* instance, callback_GetProviders const& callback)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  sqlite3_stmt* statement; // SQL statement to execute
  int result; // Result from SQLite function

  // frequency | name
  auto sql = "SELECT * FROM provider ORDER BY c02_name ASC";

  result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result, sqlite3_errmsg(instance));

  try
  {
    // Execute the query and iterate over all returned rows
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
      struct ProviderProps item;
      item.id = static_cast<uint32_t>(sqlite3_column_int(statement, 0));
      item.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
      item.logourl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
      item.country = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3));
      item.language = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));

      callback(item); // Invoke caller-supplied callback
    }

    sqlite3_finalize(statement); // Finalize the SQLite statement
  }
  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

bool CConPoolPVR::ProviderExists(sqlite3* instance, unsigned int uniqueId)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  return execute_scalar_int(__src_loc_cur__, instance,
                            "SELECT EXISTS(SELECT * FROM provider WHERE c01_id = ?1)",
                            uniqueId) == 1;
}

int CConPoolPVR::GetChannelsCount(sqlite3* instance)
{
  if (instance == nullptr)
    return 0;

  // Use the same LEFT OUTER JOIN logic against subchannel to return an accurate count when a digital
  // channel does not have any defined subchannels and will return a .0 channel instance
  return execute_scalar_int(__src_loc_cur__, instance, "SELECT count(*) FROM channel");
}

void CConPoolPVR::GetChannels(sqlite3* instance,
                              enum Modulation modulation,
                              bool prependnumber,
                              callback_GetChannels const& callback)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  sqlite3_stmt* statement; // SQL statement to execute
  int result; // Result from SQLite function

  // frequency | name
  const char* sql;
  if (modulation != Modulation::ALL)
  {
    sql = R"sql(
SELECT * FROM channel
         WHERE c03_modulation = ?1
ORDER BY c05_channelnumber, c02_subchannelnumber ASC
)sql";
  }
  else
  {
    sql = R"sql(
SELECT * FROM channel
ORDER BY c05_channelnumber, c02_subchannelnumber ASC
)sql";
  }

  result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result, sqlite3_errmsg(instance));

  try
  {
    if (modulation != Modulation::ALL)
    {
      // Bind the query parameters
      result = sqlite3_bind_int(statement, 1, static_cast<int>(modulation));
      if (result != SQLITE_OK)
        throw sqlite_exception(__src_loc_cur__, result);
    }

    // Execute the query and iterate over all returned rows
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
      struct ChannelProps item(static_cast<uint32_t>(sqlite3_column_int(statement, 0)));

      // Confirm "ChannelProps" constructor has set the 3 values correct by translate of ID.
      assert(item.subchannelnumber == static_cast<uint32_t>(sqlite3_column_int(statement, 1)));
      assert(static_cast<int>(item.modulation) == sqlite3_column_int(statement, 2));
      assert(item.frequency == static_cast<uint32_t>(sqlite3_column_int(statement, 3)));

      item.channelnumber = static_cast<uint32_t>(sqlite3_column_int(statement, 4));
      item.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
      item.usereditname = reinterpret_cast<const char*>(sqlite3_column_text(statement, 6));
      item.provider_id = static_cast<uint32_t>(sqlite3_column_int(statement, 7));
      item.logourl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 8));
      item.userlogourl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 9));
      item.country = reinterpret_cast<const char*>(sqlite3_column_text(statement, 10));
      item.language = reinterpret_cast<const char*>(sqlite3_column_text(statement, 11));
      item.programmtype = static_cast<enum ProgrammType>(sqlite3_column_int(statement, 12));
      item.transportmode = static_cast<enum TransportMode>(sqlite3_column_int(statement, 13));
      item.mimetype = reinterpret_cast<const char*>(sqlite3_column_text(statement, 14));
      const bool fallbacks = sqlite3_column_int(statement, 15) != 0;
      item.autogain = sqlite3_column_int(statement, 16) != 0;
      item.manualgain = sqlite3_column_int(statement, 17);
      item.freqcorrection = sqlite3_column_int(statement, 18);
      item.notpublic = sqlite3_column_int(statement, 19) != 0;
      item.visible = sqlite3_column_int(statement, 20) != 0;

      //item.provider = reinterpret_cast<const char*>(sqlite3_column_text(statement, 7));

      callback(item); // Invoke caller-supplied callback
    }

    sqlite3_finalize(statement); // Finalize the SQLite statement
  }
  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

bool CConPoolPVR::ChannelExists(sqlite3* instance, unsigned int uniqueId)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  return execute_scalar_int(__src_loc_cur__, instance,
                            "SELECT EXISTS(SELECT * FROM channel WHERE c01_id = ?1)",
                            uniqueId) == 1;
}

void CConPoolPVR::ChannelDelete(sqlite3* instance, unsigned int uniqueId)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  // frequency | name
  execute_non_query(__src_loc_cur__, instance, R"sql(
INSERT INTO deletedchannel
       SELECT c01_id, c02_subchannelnumber, c03_modulation, c04_frequency, c08_provider, c05_channelnumber, c06_name
       FROM channel
       WHERE c01_id = ?1 AND NOT EXISTS (SELECT c01_id FROM deletedchannel WHERE deletedchannel.c01_id = channel.c01_id);
)sql",
                    static_cast<int>(uniqueId));
  execute_non_query(__src_loc_cur__, instance, R"sql(
DELETE FROM channel
       WHERE c01_id = ?1;
)sql",
                    static_cast<int>(uniqueId));
}

void CConPoolPVR::ChannelRename(sqlite3* instance,
                                unsigned int uniqueId,
                                const std::string& newName)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  execute_non_query(__src_loc_cur__, instance, R"sql(
UPDATE channel SET c07_usereditname = ?1
               WHERE c01_id = ?2
)sql",
                    newName.c_str(), uniqueId);
}

void CConPoolPVR::GetDeletedChannels(sqlite3* instance, std::vector<ChannelProps>& channels)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  sqlite3_stmt* statement; // SQL statement to execute
  int result; // Result from SQLite function

  // frequency | name
  auto sql = "SELECT * FROM deletedchannel";

  result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result, sqlite3_errmsg(instance));

  try
  {
    // Execute the query and iterate over all returned rows
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
      struct ChannelProps item(static_cast<uint32_t>(sqlite3_column_int(statement, 0)));

      // Confirm "ChannelProps" constructor has set the 3 values correct by translate of ID.
      assert(item.subchannelnumber == static_cast<uint32_t>(sqlite3_column_int(statement, 1)));
      assert(static_cast<int>(item.modulation) == sqlite3_column_int(statement, 2));
      assert(item.frequency == static_cast<uint32_t>(sqlite3_column_int(statement, 3)));

      item.provider_id = static_cast<uint32_t>(sqlite3_column_int(statement, 4));
      item.channelnumber = static_cast<uint32_t>(sqlite3_column_int(statement, 5));
      item.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 6));

      channels.push_back(item);
    }

    sqlite3_finalize(statement); // Finalize the SQLite statement
  }
  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

void CConPoolPVR::ChannelScanSet(sqlite3* instance,
                                 const std::vector<ChannelProps>& channels,
                                 const std::vector<ProviderProps>& providers,
                                 bool autoScan)
{
  if (instance == nullptr)
    return;

  std::vector<struct ChannelProps> channelsDeleted;
  std::vector<struct ChannelProps> channelsBefore;
  std::vector<struct ProviderProps> providersBefore;

  try
  {
    // This requires a multi-step operation; start a transaction
    execute_non_query(__src_loc_cur__, instance, "BEGIN IMMEDIATE TRANSACTION");

    // By user started scans clear "deletedchannel" entries in database.
    if (!autoScan)
    {
      execute_non_query(__src_loc_cur__, instance, "DELETE FROM deletedchannel");
    }
    else
    {
      GetDeletedChannels(instance, channelsDeleted);
    }

    //// Get old channels and providers to know on end whats new or what no more available
    GetChannels(instance, channelsBefore);
    GetProviders(instance, providersBefore);

    for (const auto& provider : providers)
    {
      const auto it =
          std::find_if(providersBefore.begin(), providersBefore.end(),
                       [provider](const auto& entry) { return entry.id == provider.id; });
      if (it != providersBefore.end())
      {
        // Remove from known providers before. If the list not empty at end becomes the unused ones removed from Database
        providersBefore.erase(it);

        // clang-format off
        execute_non_query(__src_loc_cur__, instance, R"sql(
REPLACE INTO provider (
    c01_id,
    c02_name,
    c03_logourl,
    c04_country,
    c05_language
)
VALUES (?1, ?2, ?3, ?4, ?5)
)sql",
          provider.id, // 1
          provider.name.c_str(), // 2
          provider.logourl.c_str(), // 3
          provider.country.c_str(), // 4
          provider.language.c_str() // 5
        );
        // clang-format on
      }
      else
      {
        // clang-format off
        execute_non_query(__src_loc_cur__, instance, R"sql(
INSERT INTO provider (
    c01_id,
    c02_name,
    c03_logourl,
    c04_country,
    c05_language
)
VALUES (?1, ?2, ?3, ?4, ?5)
)sql",
          provider.id, // 1
          provider.name.c_str(), // 2
          provider.logourl.c_str(), // 3
          provider.country.c_str(), // 4
          provider.language.c_str() // 5
        );
        // clang-format on
      }
    }

    for (const auto& channel : channels)
    {
      const auto it = std::find_if(channelsBefore.begin(), channelsBefore.end(),
                                   [channel](const auto& entry) { return entry.id == channel.id; });
      if (it != channelsBefore.end())
      {
        // Remove from known channels before. If the list not empty at end becomes the unused ones removed from Database
        channelsBefore.erase(it);

        // clang-format off
        execute_non_query(__src_loc_cur__, instance, R"sql(
REPLACE INTO channel (
    c01_id,
    c02_subchannelnumber,
    c03_modulation,
    c04_frequency,
    c05_channelnumber,
    c06_name,
    c07_usereditname,
    c08_provider,
    c09_logourl,
    c10_userlogourl,
    c11_country,
    c12_language,
    c13_programmtype,
    c14_transportmode,
    c15_mimetype,
    c16_fallbacks,
    c17_autogain,
    c18_manualgain,
    c19_freqcorrection,
    c20_notpublic,
    c21_visible
)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)
)sql",
          channel.id.Id(), // 1
          channel.subchannelnumber, // 2
          uint8_t(channel.modulation), // 3
          channel.frequency, // 4
          channel.channelnumber, // 5
          channel.name.c_str(), // 6
          channel.usereditname.c_str(), // 7
          channel.provider_id, // 8
          channel.logourl.c_str(), // 9
          channel.userlogourl.c_str(), // 10
          channel.country.c_str(), // 11
          channel.language.c_str(), // 12
          uint8_t(channel.programmtype), // 13
          uint8_t(channel.transportmode), // 14
          channel.mimetype.c_str(), // 15
          !channel.fallbacks.empty(), // 16
          channel.autogain, // 17
          channel.manualgain, // 18
          channel.freqcorrection, // 19
          channel.notpublic, // 20
          channel.visible // 21
        );
        // clang-format on
      }
      else
      {
        const auto it =
            std::find_if(channelsDeleted.begin(), channelsDeleted.end(),
                         [channel](const auto& entry) { return entry.id == channel.id; });

        // Skip insert if listed in deleted channels
        if (it != channelsDeleted.end())
        {
          // Remove from deleted list if found, the entries still present at end in "channelsDeleted"
          // are no more available from radio and becomes removed from database.
          channelsDeleted.erase(it);
          continue;
        }

        // clang-format off
        execute_non_query(__src_loc_cur__, instance, R"sql(
INSERT INTO channel (
    c01_id,
    c02_subchannelnumber,
    c03_modulation,
    c04_frequency,
    c05_channelnumber,
    c06_name,
    c07_usereditname,
    c08_provider,
    c09_logourl,
    c10_userlogourl,
    c11_country,
    c12_language,
    c13_programmtype,
    c14_transportmode,
    c15_mimetype,
    c16_fallbacks,
    c17_autogain,
    c18_manualgain,
    c19_freqcorrection,
    c20_notpublic,
    c21_visible
)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21)
)sql",
          channel.id.Id(), // 1
          channel.subchannelnumber, // 2
          uint8_t(channel.modulation), // 3
          channel.frequency, // 4
          channel.channelnumber, // 5
          channel.name.c_str(), // 6
          channel.usereditname.c_str(), // 7
          channel.provider_id, // 8
          channel.logourl.c_str(), // 9
          channel.userlogourl.c_str(), // 10
          channel.country.c_str(), // 11
          channel.language.c_str(), // 12
          uint8_t(channel.programmtype), // 13
          uint8_t(channel.transportmode), // 14
          channel.mimetype.c_str(), // 15
          !channel.fallbacks.empty(), // 16
          channel.autogain, // 17
          channel.manualgain, // 18
          channel.freqcorrection, // 19
          channel.notpublic, // 20
          channel.visible // 21
        );
        // clang-format on
      }
    }

    // Cleanup now channels where no more available
    for (const auto& channel : channelsBefore)
    {
      UTILS::log(UTILS::LOGLevel::INFO, __src_loc_cur__,
                 "Removing no more available channel from database:  {} (Country: '{}', Language: "
                 "'{}', Unique Id: {:X})",
                 channel.name, channel.country, channel.language, channel.id.Id());

      execute_non_query(__src_loc_cur__, instance, "DELETE FROM channel WHERE c01_id = ?1",
                        channel.id.Id());
    }

    // Cleanup now channels where no more available and still listed in deleted channels
    for (const auto& channel : channelsDeleted)
    {
      execute_non_query(__src_loc_cur__, instance, "DELETE FROM deletedchannel WHERE c01_id = ?1",
                        channel.id.Id());
    }

    // Cleanup providers where no more available and still listed
    for (const auto& provider : providersBefore)
    {
      execute_non_query(__src_loc_cur__, instance, "DELETE FROM provider WHERE c01_id = ?1",
                        provider.id);
    }

    // Commit the database transaction
    execute_non_query(__src_loc_cur__, instance, "COMMIT TRANSACTION");
  }
  catch (...)
  {
    // Rollback the transaction on any exception
    try_execute_non_query(instance, "ROLLBACK TRANSACTION");
    throw;
  }
}

void CConPoolPVR::SetLastScanTime(sqlite3* instance, time_t time, size_t channelsfound)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  execute_non_query(__src_loc_cur__, instance,
                    "INSERT INTO channelscan (c02_time, c03_channelsfound) VALUES (?1, ?2)", time,
                    int(channelsfound));
}

time_t CConPoolPVR::GetLastScanTime(sqlite3* instance)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  return static_cast<time_t>(
      execute_scalar_int(__src_loc_cur__, instance, "SELECT MAX(c02_time) FROM channelscan;"));
}

void CConPoolPVR::GetProviders(sqlite3* instance, std::vector<struct ProviderProps>& providers)
{
  GetProviders(instance, [&](const ProviderProps& item) { providers.emplace_back(item); });
}

void CConPoolPVR::GetChannels(sqlite3* instance, std::vector<struct ChannelProps>& channels)
{
  GetChannels(instance, Modulation::ALL, false,
              [&](const ChannelProps& item) { channels.emplace_back(item); });
}

} // namespace DB
} // namespace SETTINGS
} // namespace RTLRADIO
