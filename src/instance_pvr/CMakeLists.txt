set(SOURCE_NAME code_src_instance_pvr)

set(SOURCES
    pvr.cpp
    pvr_type_dab.cpp
    pvr_type_fm.cpp
    pvr_type_hd.cpp
    pvr_type_mw.cpp
    pvr_type_wx.cpp
    signalmeter.cpp
    fastfir.cpp
    fft.cpp
)

set(HEADERS
    pvr.h
    pvr_radio_switcher_i.h
    pvr_type_dab.h
    pvr_type_fm.h
    pvr_type_hd.h
    pvr_type_mw.h
    pvr_type_wx.h
    pvr_type_i.h
    signalmeter.h
    fastfir.h
    fft.h
)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})

set_target_properties(${SOURCE_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${SOURCE_NAME} PRIVATE "VERSION_PRODUCTNAME_ANSI=\"${CMAKE_PROJECT_NAME}\""
                                          PRIVATE "VERSION_VERSION3_ANSI=\"${PVRRTLRADIO_VERSION}\"")
target_include_directories(${SOURCE_NAME} PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})

unset(SOURCE_NAME)
