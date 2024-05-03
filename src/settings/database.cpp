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
  int dbversion = execute_scalar_int(instance, "PRAGMA user_version");

  // SCHEMA VERSION 0 -> VERSION 1
  //
  if (dbversion == 0)
  {
    // clang-format off

    /**
     * @brief table: channel
     */
    execute_non_query(instance, "DROP TABLE IF EXISTS channel");
    execute_non_query(instance, R"sql(
CREATE TABLE channel(
    c01_id                INTEGER NOT NULL,
    c02_subchannelnumber  INTEGER NOT NULL,
    c03_modulation        INTEGER NOT NULL,
    c04_frequency         INTEGER NOT NULL,
    c05_channelnumber     INTEGER NOT NULL,
    c06_name              TEXT NOT NULL,
    c07_usereditname      TEXT NOT NULL,
    c08_provider          TEXT NOT NULL,
    c09_logourl           TEXT NULL,
    c10_userlogourl       TEXT NULL,
    c11_country           INTEGER NULL,
    c12_language          INTEGER NULL,
    c13_programmtype      INTEGER NULL,
    c14_transportmode     INTEGER NOT NULL,
    c15_datatype          TEXT NOT NULL,
    c16_fallbacks         BOOLEAN NOT NULL,
    c17_autogain          INTEGER NOT NULL,
    c18_manualgain        INTEGER NOT NULL,
    c19_freqcorrection    INTEGER NOT NULL,
    c20_visible           BOOLEAN NOT NULL,
PRIMARY KEY (c01_id)
);)sql");

    /**
     * @brief table: channelscan
     */
    execute_non_query(instance, "DROP TABLE IF EXISTS channelscan");
    execute_non_query(instance, R"sql(
CREATE TABLE channelscan(
    c01_id                INTEGER NOT NULL,
    c02_time              INTEGER NOT NULL,
    c03_channelsfound     INTEGER NOT NULL,
PRIMARY KEY (c01_id)
);)sql");
    execute_non_query(instance, "PRAGMA user_version = 1");
    dbversion = 1;

    // clang-format on
  }
}

int CConPoolPVR::GetChannelsCount(sqlite3* instance)
{
  if (instance == nullptr)
    return 0;

  // Use the same LEFT OUTER JOIN logic against subchannel to return an accurate count when a digital
  // channel does not have any defined subchannels and will return a .0 channel instance
  return execute_scalar_int(instance, "SELECT count(*) FROM channel");
}

void CConPoolPVR::GetChannels(sqlite3* instance,
                              enum Modulation modulation,
                              bool prependnumber,
                              callback_GetChannels const& callback)
{
  if (instance == nullptr)
    return;

  sqlite3_stmt* statement; // SQL statement to execute
  int result; // Result from SQLite function

  // frequency | name
  auto sql = R"sql(
SELECT * FROM channel
         WHERE c03_modulation = ?1
ORDER BY c05_channelnumber, c02_subchannelnumber ASC
)sql";

  result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result, sqlite3_errmsg(instance));

  try
  {
    // Bind the query parameters
    result = sqlite3_bind_int(statement, 1, static_cast<int>(modulation));
    if (result != SQLITE_OK)
      throw sqlite_exception(__src_loc_cur__, result);

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
      item.provider = reinterpret_cast<const char*>(sqlite3_column_text(statement, 7));
      item.logourl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 8));
      item.userlogourl = reinterpret_cast<const char*>(sqlite3_column_text(statement, 9));
      item.country = static_cast<enum CountryCode>(sqlite3_column_int(statement, 10));
      item.language = static_cast<enum LanguageCode>(sqlite3_column_int(statement, 11));
      item.programmtype = static_cast<enum ProgrammType>(sqlite3_column_int(statement, 12));
      item.transportmode = static_cast<enum TransportMode>(sqlite3_column_int(statement, 13));
      item.datatype = reinterpret_cast<const char*>(sqlite3_column_text(statement, 14));
      const bool fallbacks = sqlite3_column_int(statement, 15) != 0;
      item.autogain = sqlite3_column_int(statement, 16) != 0;
      item.manualgain = sqlite3_column_int(statement, 17);
      item.freqcorrection = sqlite3_column_int(statement, 18);
      item.visible = sqlite3_column_int(statement, 19) != 0;

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

  return execute_scalar_int(instance, "SELECT EXISTS(SELECT * FROM channel WHERE c01_id = ?1)",
                            uniqueId) == 1;
}

void CConPoolPVR::ChannelScanSet(sqlite3* instance, const std::vector<ChannelProps>& channels)
{
  if (instance == nullptr)
    return;

  for (const auto& channel : channels)
  {
    if (!ChannelExists(instance, channel.id.Id()))
    {
      // clang-format off
      execute_non_query(instance, R"sql(
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
    c15_datatype,
    c16_fallbacks,
    c17_autogain,
    c18_manualgain,
    c19_freqcorrection,
    c20_visible
)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)
)sql",
        channel.id.Id(), // 1
        channel.subchannelnumber, // 2
        uint8_t(channel.modulation), // 3
        channel.frequency, // 4
        channel.channelnumber, // 5
        channel.name.c_str(), // 6
        channel.usereditname.c_str(), // 7
        channel.provider.c_str(), // 8
        channel.logourl.c_str(), // 9
        channel.userlogourl.c_str(), // 10
        uint32_t(channel.country), // 11
        uint32_t(channel.language), // 12
        uint8_t(channel.programmtype), // 13
        uint8_t(channel.transportmode), // 14
        channel.datatype.c_str(), // 15
        !channel.fallbacks.empty(), // 16
        channel.autogain, // 17
        channel.manualgain, // 18
        channel.freqcorrection, // 19
        channel.visible // 20
      );
      // clang-format on
    }
    else
    {
      // clang-format off
      execute_non_query(instance, R"sql(
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
    c15_datatype,
    c16_fallbacks,
    c17_autogain,
    c18_manualgain,
    c19_freqcorrection,
    c20_visible
)
VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20)
)sql",
        channel.id.Id(), // 1
        channel.subchannelnumber, // 2
        uint8_t(channel.modulation), // 3
        channel.frequency, // 4
        channel.channelnumber, // 5
        channel.name.c_str(), // 6
        channel.usereditname.c_str(), // 7
        channel.provider.c_str(), // 8
        channel.logourl.c_str(), // 9
        channel.userlogourl.c_str(), // 10
        uint32_t(channel.country), // 11
        uint32_t(channel.language), // 12
        uint8_t(channel.programmtype), // 13
        uint8_t(channel.transportmode), // 14
        channel.datatype.c_str(), // 15
        !channel.fallbacks.empty(), // 16
        channel.autogain, // 17
        channel.manualgain, // 18
        channel.freqcorrection, // 19
        channel.visible // 20
      );
      // clang-format on
    }
  }
}

void CConPoolPVR::SetLastScanTime(sqlite3* instance, time_t time, unsigned int channelsfound)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  execute_non_query(instance,
                    "INSERT INTO channelscan (c02_time, c03_channelsfound) VALUES (?1, ?2)", time,
                    channelsfound);
}

time_t CConPoolPVR::GetLastScanTime(sqlite3* instance)
{
  if (instance == nullptr)
    throw std::invalid_argument("instance");

  return static_cast<time_t>(
      execute_scalar_int(instance, "SELECT MAX(c02_time) FROM channelscan;"));
}

} // namespace DB
} // namespace SETTINGS
} // namespace RTLRADIO
