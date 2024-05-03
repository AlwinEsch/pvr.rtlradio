find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_RTLSDR rtlsdr QUIET)
endif()

find_path(RTLSDR_INCLUDE_DIRS rtl-sdr.h
                              PATHS ${PC_RTLSDR_INCLUDEDIR})
find_library(RTLSDR_LIBRARIES rtlsdr
                              PATHS ${PC_RTLSDR_LIBDIR})

# Append needed libusb to rtlsdr if not set before
if (NOT RTLSDR_LIBRARIES MATCHES usb-1.0)
  find_package(libusb-1.0 REQUIRED)
  list(APPEND RTLSDR_INCLUDE_DIRS ${LIBUSB_1_INCLUDE_DIRS})
  list(APPEND RTLSDR_LIBRARIES ${LIBUSB_1_LIBRARIES})
  unset(LIBUSB_1_INCLUDE_DIRS)
  unset(LIBUSB_1_LIBRARIES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rtlsdr REQUIRED_VARS RTLSDR_LIBRARIES RTLSDR_INCLUDE_DIRS)

mark_as_advanced(RTLSDR_INCLUDE_DIRS RTLSDR_LIBRARIES)
