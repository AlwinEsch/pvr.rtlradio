/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "libusb_exception.h"

#pragma warning(disable : 4200)
#include <libusb.h>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// libusb_exception Constructor
//
// Arguments:
//
//	code		- SQLite error code
//	message		- Message to associate with the exception

libusb_exception::libusb_exception(int code)
{
  char what[512] = {'\0'}; // Formatted exception string

  snprintf(what, std::extent<decltype(what)>::value, "%s (%d) : %s", libusb_error_name(code), code,
           libusb_strerror(static_cast<libusb_error>(code)));

  m_what.assign(what);
}

//-----------------------------------------------------------------------------
// libusb_exception Copy Constructor

libusb_exception::libusb_exception(libusb_exception const& rhs) : m_what(rhs.m_what)
{
}

//-----------------------------------------------------------------------------
// libusb_exception Move Constructor

libusb_exception::libusb_exception(libusb_exception&& rhs) : m_what(std::move(rhs.m_what))
{
}

//-----------------------------------------------------------------------------
// libusb_exception char const* conversion operator

libusb_exception::operator char const *() const
{
  return m_what.c_str();
}

//-----------------------------------------------------------------------------
// libusb_exception::what
//
// Gets a pointer to the exception message text
//
// Arguments:
//
//	NONE

char const* libusb_exception::what(void) const noexcept
{
  return m_what.c_str();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
