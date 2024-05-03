/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <cstring>
#include <exception>
#include <sstream>
#include <string>

#ifdef WIN32
#include "win32_exception.h"
#endif

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// Class CSocketException
//
// std::exception used to describe a socket exception

class CSocketException : public std::exception
{
public:
  // Instance Constructor
  //
  template<typename... Args>
  CSocketException(const nostd::source_location location,
                   const std::string_view& format,
                   Args&&... args)
  {
    std::ostringstream stream;

    stream << fmt::format("file: {}({}:{}) `{}`: {}", location.file_name(), location.line(),
                          location.column(), location.function_name(),
                          fmt::format(format, std::forward<Args>(args)...));

#ifdef _WINDOWS
    stream << " (ERROR: " << win32_exception(static_cast<DWORD>(WSAGetLastError())) << ")";
#elif __clang__
    int code = errno;
    char buf[512] = {};
    if (strerror_r(code, buf, std::extent<decltype(buf)>::value) == 0)
      stream << " (ERROR: " << buf << ")";
    else
      stream << " (ERROR: CSocketException code " << code << ")";
#else
    int code = errno;
    char buf[512] = {};
    stream << " (ERROR: " << strerror_r(code, buf, std::extent<decltype(buf)>::value) << ")";
#endif

    m_what = stream.str();
  }

  // Copy Constructor
  //
  CSocketException(CSocketException const& rhs) : m_what(rhs.m_what) {}

  // Move Constructor
  //
  CSocketException(CSocketException&& rhs) : m_what(std::move(rhs.m_what)) {}

  // char const* conversion operator
  //
  operator char const *() const { return m_what.c_str(); }

  //-------------------------------------------------------------------------
  // Member Functions

  // what (std::exception)
  //
  // Gets a pointer to the exception message text
  virtual char const* what(void) const noexcept override { return m_what.c_str(); }

private:
  //-------------------------------------------------------------------------
  // Member Variables

  std::string m_what; // Exception message
};

//-----------------------------------------------------------------------------

#pragma warning(pop)
