/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "exception_control/sqlite_exception.h"
#include "exception_control/string_exception.h"
#include "utils/log.h"

namespace RTLRADIO
{
namespace EXCEPTION
{

void HandleGeneralException(const nostd::source_location location)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Failed due to an exception");
}

template<typename _result>
_result HandleGeneralException(const nostd::source_location location, _result result)
{
  HandleGeneralException(location);
  return result;
}

void HandleStdException(const nostd::source_location location, std::exception const& ex)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Failed due to an exception: {}", ex.what());
}

template<typename _result>
_result HandleStdException(const nostd::source_location location,
                           std::exception const& ex,
                           _result result)
{
  HandleStdException(location, ex);
  return result;
}

void HandleDBException(const nostd::source_location location, sqlite_exception const& dbex)
{
  UTILS::log(UTILS::LOGLevel::ERROR, location, "Database error: {} - Source: {}({},{})",
             dbex.what(), dbex.location().file_name(), dbex.location().line(),
             dbex.location().column());
}

template<typename _result>
_result HandleDBException(const nostd::source_location location,
                          sqlite_exception const& dbex,
                          _result result)
{
  HandleDBException(location, dbex);
  return result;
}

} // namespace EXCEPTION
} // namespace RTLRADIO
