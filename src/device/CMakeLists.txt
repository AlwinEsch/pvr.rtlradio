set(SOURCE_NAME code_src_device)

set(SOURCES device.cpp
            device_file.cpp
            device_list.cpp
            device_tcp.cpp
            $<$<BOOL:${USB_DEVICE_SUPPORT}>:device_usb.cpp>)

set(HEADERS device.h
            device_file.h
            device_list.h
            device_tcp.h
            $<$<BOOL:${USB_DEVICE_SUPPORT}>:device_usb.h>)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})
target_include_directories(${SOURCE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                          PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE $<$<BOOL:${USB_DEVICE_SUPPORT}>:${RTLSDR_INCLUDE_DIRS}>)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})
target_compile_definitions(${SOURCE_NAME} PRIVATE rtlsdr_EXPORTS=1
                                                  $<$<BOOL:${USB_DEVICE_SUPPORT}>:USB_DEVICE_SUPPORT=1>)
unset(SOURCE_NAME)
