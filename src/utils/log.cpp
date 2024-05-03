/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "log.h"

#include <kodi/AddonBase.h>
#include <kodi/tools/StringUtils.h>

namespace RTLRADIO
{
namespace UTILS
{

void log_internal(LOGLevel loglevel, const std::string_view& text)
{
  ADDON_LOG kodiLogLevel;
  std::string typeStr;
  switch (loglevel)
  {
    case LOGLevel::DEBUG:
      typeStr = "DEBUG:   ";
      kodiLogLevel = ADDON_LOG_DEBUG;
      break;
    case LOGLevel::WARNING:
      typeStr = "WARNING: ";
      kodiLogLevel = ADDON_LOG_WARNING;
      break;
    case LOGLevel::ERROR:
      typeStr = "ERROR:   ";
      kodiLogLevel = ADDON_LOG_ERROR;
      break;
    case LOGLevel::FATAL:
      typeStr = "FATAL:   ";
      kodiLogLevel = ADDON_LOG_FATAL;
      break;
    case LOGLevel::INFO:
    default:
      typeStr = "INFO:    ";
      kodiLogLevel = ADDON_LOG_INFO;
      break;
  }

  kodi::Log(kodiLogLevel, text.data());

  fprintf(stderr, "%s%s\n", typeStr.c_str(), text.data());
}

void DEBUG_PRINT(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  const std::string str = kodi::tools::StringUtils::FormatV(format, args);
  va_end(args);

  fprintf(stderr, "%s", str.c_str());
}

} // namespace UTILS
} // namespace RTLRADIO
