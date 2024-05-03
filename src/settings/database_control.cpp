/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "database.h"
#include "props.h"
#include "utils/log.h"

#include <cassert>

namespace RTLRADIO
{
namespace SETTINGS
{
namespace DB
{

//------------------------------------------------------------------------------

CConPool::CConPool(const std::string& dbPath, size_t poolsize, int flags)
  : m_dbPath(dbPath), m_poolsize(poolsize), m_flags(flags)
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "constructed");

  if (m_dbPath.empty())
    throw std::invalid_argument("dbPath");
}

CConPool::~CConPool()
{
  UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__, "destructed");

  // Close all of the connections that were created in the pool
  for (auto const& iterator : m_connections)
    CloseDatabase(iterator);
}

void CConPool::InitDatabase()
{
  m_dbFile = fmt::format("file:///{}/{}", m_dbPath, GetBaseDBName());

  // Create and pool an initial connection to initialize the database
  sqlite3* handle = OpenDatabase(true);
  m_connections.push_back(handle);
  m_queue.push(handle);

  // Create and pool the requested number of additional connections
  try
  {
    for (size_t index = 1; index < m_poolsize; index++)
    {
      handle = OpenDatabase(false);
      m_connections.push_back(handle);
      m_queue.push(handle);
    }
  }
  catch (...)
  {
    // Clear the connection cache and destroy all created connections
    while (!m_queue.empty())
      m_queue.pop();
    for (auto const& iterator : m_connections)
      CloseDatabase(iterator);

    throw;
  }
}

sqlite3* CConPool::OpenDatabase(bool initialize)
{
  sqlite3* instance = nullptr; // SQLite database instance

  // Create the database using the provided connection string
  int result = sqlite3_open_v2(m_dbFile.c_str(), &instance, m_flags, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result);

  // set the connection to report extended error codes
  sqlite3_extended_result_codes(instance, -1);

  // set a busy_timeout handler for this connection
  sqlite3_busy_timeout(instance, 5000);

  try
  {
    // switch the database to write-ahead logging
    execute_non_query(instance, "PRAGMA journal_mode=wal");

    // Only execute schema creation steps if the database is being initialized; the caller needs
    // to ensure that this is set for only one connection otherwise locking issues can occur
    if (initialize)
      InitDatabase(instance);
  }

  // Close the database instance on any thrown exceptions
  catch (...)
  {
    sqlite3_close(instance);
    throw;
  }

  return instance;
}

void CConPool::CloseDatabase(sqlite3* instance)
{
  if (instance)
    sqlite3_close(instance);
}

sqlite3* CConPool::Acquire(void)
{
  sqlite3* handle = nullptr; // Handle to return to the caller

  std::unique_lock<std::mutex> lock(m_lock);

  if (m_queue.empty())
  {

    // No connections are available, open a new one using the same flags
    handle = OpenDatabase(false);
    m_connections.push_back(handle);
  }

  // At least one connection is available for reuse
  else
  {
    handle = m_queue.front();
    m_queue.pop();
  }

  return handle;
}

void CConPool::Release(sqlite3* handle)
{
  std::unique_lock<std::mutex> lock(m_lock);

  if (handle == nullptr)
    throw std::invalid_argument("handle");

  m_queue.push(handle);
}

//------------------------------------------------------------------------------

void bind_parameter(sqlite3_stmt* statement, int& paramindex, char const* value)
{
  int result; // Result from binding operation

  // If a null string pointer was provided, bind it as NULL instead of TEXT
  if (value == nullptr)
    result = sqlite3_bind_null(statement, paramindex++);
  else
    result = sqlite3_bind_text(statement, paramindex++, value, -1, SQLITE_STATIC);

  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result);
}

void bind_parameter(sqlite3_stmt* statement, int& paramindex, int value)
{
  int result = sqlite3_bind_int(statement, paramindex++, value);
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result);
}

void bind_parameter(sqlite3_stmt* statement, int& paramindex, unsigned int value)
{
  int result = sqlite3_bind_int64(statement, paramindex++, static_cast<int64_t>(value));
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result);
}

void bind_parameter(sqlite3_stmt* statement, int& paramindex, time_t value)
{
  int result = sqlite3_bind_int64(statement, paramindex++, static_cast<int64_t>(value));
  if (result != SQLITE_OK)
    throw sqlite_exception(__src_loc_cur__, result);
}

} // namespace DB
} // namespace SETTINGS
} // namespace RTLRADIO
