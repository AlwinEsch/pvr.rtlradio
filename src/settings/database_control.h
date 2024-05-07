/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "exception_control/sqlite_exception.h"

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <sqlite3.h>

namespace RTLRADIO
{

struct ChannelProps;

namespace SETTINGS
{
namespace DB
{

static size_t const CONNECTIONPOOL_SIZE = 3;

class CConPool
{
public:
  CConPool(const std::string& dbPath, size_t poolsize, int flags);
  ~CConPool();

  virtual std::string GetBaseDBName() const = 0;

  void InitDatabase();

  sqlite3* OpenDatabase(bool initialize);
  void CloseDatabase(sqlite3* instance);

  sqlite3* Acquire(void);
  void Release(sqlite3* handle);

  class handle
  {
  public:
    handle(std::shared_ptr<CConPool> const& pool) : m_pool(pool), m_handle(pool->Acquire()) {}
    ~handle() { m_pool->Release(m_handle); }

    operator sqlite3*(void) const { return m_handle; }

  private:
    handle(handle const&) = delete;
    handle& operator=(handle const&) = delete;

    std::shared_ptr<CConPool> const m_pool;
    sqlite3* m_handle;
  };

protected:
  virtual void InitDatabase(sqlite3* instance) = 0;

private:
  CConPool(CConPool const&) = delete;
  CConPool& operator=(CConPool const&) = delete;

  const std::string m_dbPath;
  const size_t m_poolsize;
  const int m_flags;
  std::string m_dbFile;
  std::vector<sqlite3*> m_connections;
  std::queue<sqlite3*> m_queue;
  mutable std::mutex m_lock;
};

void bind_parameter(sqlite3_stmt* statement, int& paramindex, char const* value);
void bind_parameter(sqlite3_stmt* statement, int& paramindex, int value);
void bind_parameter(sqlite3_stmt* statement, int& paramindex, unsigned int value);
void bind_parameter(sqlite3_stmt* statement, int& paramindex, time_t value);
bool try_execute_non_query(sqlite3* instance, char const* sql);

template<typename... _parameters>
int execute_non_query(const nostd::source_location location,
                      sqlite3* instance,
                      char const* sql,
                      _parameters&&... parameters)
{
  sqlite3_stmt* statement; // SQL statement to execute
  int paramindex = 1; // Bound parameter index value

  if (instance == nullptr)
    throw std::invalid_argument("instance");
  if (sql == nullptr)
    throw std::invalid_argument("sql");

  // Suppress unreferenced local variable warning when there are no parameters to bind
  (void)paramindex;

  // Prepare the statement
  int result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(location, result, sqlite3_errmsg(instance));

  try
  {

    // Bind the provided query parameter(s) by unpacking the parameter pack
    int unpack[] = {0,
                    (static_cast<void>(bind_parameter(statement, paramindex, parameters)), 0)...};
    (void)unpack;

    // Execute the query; ignore any rows that are returned
    do
      result = sqlite3_step(statement);
    while (result == SQLITE_ROW);

    // The final result from sqlite3_step should be SQLITE_DONE
    if (result != SQLITE_DONE)
      throw sqlite_exception(location, result, sqlite3_errmsg(instance));

    // Finalize the statement
    sqlite3_finalize(statement);

    // Return the number of changes made by the statement
    return sqlite3_changes(instance);
  }

  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

template<typename... _parameters>
int execute_scalar_int(const nostd::source_location location,
                       sqlite3* instance,
                       char const* sql,
                       _parameters&&... parameters)
{
  sqlite3_stmt* statement; // SQL statement to execute
  int paramindex = 1; // Bound parameter index value
  int value = 0; // Result from the scalar function

  if (instance == nullptr)
    throw std::invalid_argument("instance");
  if (sql == nullptr)
    throw std::invalid_argument("sql");

  // Suppress unreferenced local variable warning when there are no parameters to bind
  (void)paramindex;

  // Prepare the statement
  int result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(location, result, sqlite3_errmsg(instance));

  try
  {

    // Bind the provided query parameter(s) by unpacking the parameter pack
    int unpack[] = {0,
                    (static_cast<void>(bind_parameter(statement, paramindex, parameters)), 0)...};
    (void)unpack;

    // Execute the query; only the first row returned will be used
    result = sqlite3_step(statement);

    if (result == SQLITE_ROW)
      value = sqlite3_column_int(statement, 0);
    else if (result != SQLITE_DONE)
      throw sqlite_exception(location, result, sqlite3_errmsg(instance));

    // Finalize the statement
    sqlite3_finalize(statement);

    // Return the resultant value from the scalar query
    return value;
  }

  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

template<typename... _parameters>
std::string execute_scalar_string(const nostd::source_location location,
                                  sqlite3* instance,
                                  char const* sql,
                                  _parameters&&... parameters)
{
  sqlite3_stmt* statement; // SQL statement to execute
  int paramindex = 1; // Bound parameter index value
  std::string value; // Result from the scalar function

  if (instance == nullptr)
    throw std::invalid_argument("instance");
  if (sql == nullptr)
    throw std::invalid_argument("sql");

  // Suppress unreferenced local variable warning when there are no parameters to bind
  (void)paramindex;

  // Prepare the statement
  int result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
  if (result != SQLITE_OK)
    throw sqlite_exception(location, result, sqlite3_errmsg(instance));

  try
  {

    // Bind the provided query parameter(s) by unpacking the parameter pack
    int unpack[] = {0,
                    (static_cast<void>(bind_parameter(statement, paramindex, parameters)), 0)...};
    (void)unpack;

    // Execute the query; only the first row returned will be used
    result = sqlite3_step(statement);

    if (result == SQLITE_ROW)
    {
      char const* ptr = reinterpret_cast<char const*>(sqlite3_column_text(statement, 0));
      if (ptr != nullptr)
        value.assign(ptr);
    }
    else if (result != SQLITE_DONE)
      throw sqlite_exception(location, result, sqlite3_errmsg(instance));

    // Finalize the statement
    sqlite3_finalize(statement);

    // Return the resultant value from the scalar query
    return value;
  }

  catch (...)
  {
    sqlite3_finalize(statement);
    throw;
  }
}

} // namespace DB
} // namespace SETTINGS
} // namespace RTLRADIO
