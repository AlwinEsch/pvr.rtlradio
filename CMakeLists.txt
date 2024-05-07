cmake_minimum_required(VERSION 3.12)
project(pvr.rtlradio)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include(cmake/USBDeviceSupportCheck.cmake)
include(cmake/PlatformCompileOptimization.cmake)

message("Build configuration:")
message("  Target platform: ${CORE_SYSTEM_NAME}")
message("  Build type: ${CMAKE_BUILD_TYPE}")
message("  Target CPU: ${TARGET_CPU}")
message("  Option \"USB_DEVICE_SUPPORT\": ${USB_DEVICE_SUPPORT}")
message("  Option \"USE_WARNINGS_AS_ERROR\": ${USE_WARNINGS_AS_ERROR}")
message("  Option \"MATH_OPTIMIZATION\": ${MATH_OPTIMIZATION}")
message("  C flags: ${CMAKE_C_FLAGS}")
message("  C++ flags: ${CMAKE_CXX_FLAGS}")

find_package(Kodi REQUIRED)
find_package(FAAD2 REQUIRED)
find_package(FFTW REQUIRED)
find_package(Fmt REQUIRED)
find_package(glm REQUIRED)
find_package(SQLite3 REQUIRED)
if(USB_DEVICE_SUPPORT)
  find_package(rtlsdr REQUIRED)
endif()

if(NOT WIN32 AND (APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM))
  find_package(OpenGl REQUIRED)
  set(DEPLIBS ${OPENGL_LIBRARIES})
  set(includes ${OPENGL_INCLUDE_DIR})
  add_definitions(${OPENGL_DEFINITIONS})
else()
  find_package(OpenGLES REQUIRED)
  set(DEPLIBS ${OPENGLES_LIBRARIES})
  set(includes ${OPENGLES_INCLUDE_DIR})
  add_definitions(${OPENGLES_DEFINITIONS})
endif()

# Set new KODI_INCLUDE_DIRS by hack way with "/..", need bigger Kodi cmake rework to match right include ways
if(NOT KODI_INCLUDE_DIRS)
  set(KODI_INCLUDE_DIRS ${KODI_INCLUDE_DIR}/..)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src # To get headers without "../"
                    ${PROJECT_SOURCE_DIR}/src/dsp_dab/process_lib # To let DAB-Radio inside find his headers
                    ${PROJECT_SOURCE_DIR}/src/utils # To allow DAB-Radio lib the find of "viterbi"
                    ${FAAD2_INCLUDE_DIR}
                    ${GLM_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIRS}
                    ${LIBFMT_INCLUDE_DIRS}
                    ${SQLITE3_INCLUDE_DIR}
                    $<$<BOOL:${USB_DEVICE_SUPPORT}>:${RTLSDR_INCLUDE_DIRS}>)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/audio)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/device)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/dsp_dab)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/dsp_dab/process_lib)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/exception_control)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gui)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/instance_inputstream)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/instance_pvr)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/settings)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/utils)

if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

add_definitions(-DTCB_SPAN_THROW_ON_CONTRACT_VIOLATION)
add_definitions(${ARCH_DEFINES})

list(APPEND DEPLIBS code_src_audio
                    code_src_device
                    code_src_dsp_dab
                    code_src_exception_control
                    code_src_gui
                    code_src_instance_inputstream
                    code_src_instance_pvr
                    code_src_settings
                    code_src_utils
                    ofdm_core
                    dab_core
                    basic_radio
                    basic_scraper
                    ${FAAD2_LIBRARY}
                    ${FFTW_FLOAT_LIB}
                    ${LIBFMT_LIBRARIES}
                    ${SQLITE3_LIBRARY}
                    $<$<BOOL:${USB_DEVICE_SUPPORT}>:${RTLSDR_LIBRARIES}>
                    $<$<BOOL:${WIN32}>:ws2_32>
                    $<$<BOOL:${WIN32}>:Shlwapi>)

set(PVRRTLRADIO_SOURCES src/addon.cpp)

set(PVRRTLRADIO_HEADERS src/addon.h
                        src/api_defines.h
                        src/props.h)

build_addon(pvr.rtlradio PVRRTLRADIO DEPLIBS)

target_compile_definitions(pvr.rtlradio PRIVATE rtlsdr_EXPORTS=1
                                                $<$<BOOL:${USB_DEVICE_SUPPORT}>:USB_DEVICE_SUPPORT=1>)

include(CPack)
