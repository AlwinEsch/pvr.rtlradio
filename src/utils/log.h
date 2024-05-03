/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/source_location.hpp"

#include <fmt/format.h>

//! @brief Macro to allow OS independet the use of __PRETTY_FUNCTION__.
//!
//! This only be used for Debugs during development and on released versions is unusued.
#if defined(_MSC_VER) && !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

//! Fix stupid macros on Windows where destroy always own codes :(.
#ifdef DEBUG
#undef DEBUG
#endif // DEBUG
#ifdef INFO
#undef INFO
#endif // INFO
#ifdef WARNING
#undef WARNING
#endif // WARNING
#ifdef ERROR
#undef ERROR
#endif // ERROR
#ifdef FATAL
#undef FATAL
#endif // FATAL

//! @brief Macro to get shorter version of call.
#ifndef __src_loc_cur__
#define __src_loc_cur__ nostd::source_location::current()
#endif // ! __src_loc_cur__

namespace RTLRADIO
{
namespace UTILS
{

enum LOGLevel
{
  DEBUG = 0,
  INFO = 1,
  WARNING = 2,
  ERROR = 3,
  FATAL = 4
};

void log_internal(LOGLevel loglevel, const std::string_view& text);

template<typename... Args>
inline void LOG(LOGLevel loglevel, const std::string_view& format, Args&&... args)
{
  auto message = fmt::format(format, std::forward<Args>(args)...);
  log_internal(loglevel, message);
}

template<typename... Args>
inline void log(LOGLevel loglevel,
                const nostd::source_location location,
                const std::string_view& format,
                Args&&... args)
{
  auto message = fmt::format("file: {}({}:{}) `{}`: {}", location.file_name(), location.line(),
                             location.column(), location.function_name(),
                             fmt::format(format, std::forward<Args>(args)...));
  log_internal(loglevel, message);
}

void DEBUG_PRINT(const char* format, ...);

#if PRINT_DEBUG == 1
#define DBGLOG(f, ...) \
  { \
    utils::DEBUG_PRINT(f, ##__VA_ARGS__); \
  }
#else
#define DBGLOG
#endif

} // namespace UTILS
} // namespace RTLRADIO
