/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <exception>
#include <string>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// Class libusb_exception
//
// std::exception used to wrap libusb error conditions

class libusb_exception : public std::exception
{
public:
  // Instance Constructors
  //
  libusb_exception(int code);

  // Copy Constructor
  //
  libusb_exception(libusb_exception const& rhs);

  // Move Constructor
  //
  libusb_exception(libusb_exception&& rhs);

  // char const* conversion operator
  //
  operator char const *() const;

  //-------------------------------------------------------------------------
  // Member Functions

  // what (std::exception)
  //
  // Gets a pointer to the exception message text
  virtual char const* what(void) const noexcept override;

private:
  //-------------------------------------------------------------------------
  // Member Variables

  std::string m_what; // libusb error message
};

//-----------------------------------------------------------------------------

#pragma warning(pop)
