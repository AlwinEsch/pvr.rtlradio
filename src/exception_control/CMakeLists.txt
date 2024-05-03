set(SOURCE_NAME code_src_exception_control)

set(SOURCES sqlite_exception.cpp)

set(HEADERS socket_exception.h
            sqlite_exception.h
            string_exception.h)

if(USB_DEVICE_SUPPORT)
  list(APPEND SOURCES libusb_exception.cpp)
  list(APPEND HEADERS libusb_exception.h)
endif()

if(WIN32)
  list(APPEND SOURCES win32_exception.cpp)
  list(APPEND HEADERS win32_exception.h)
endif()

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})
target_include_directories(${SOURCE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                          PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE $<$<BOOL:${USB_DEVICE_SUPPORT}>:${RTLSDR_INCLUDE_DIRS}>)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})
target_compile_definitions(${SOURCE_NAME} PRIVATE rtlsdr_EXPORTS=1
                                                  $<$<BOOL:${USB_DEVICE_SUPPORT}>:USB_DEVICE_SUPPORT=1>)
unset(SOURCE_NAME)
