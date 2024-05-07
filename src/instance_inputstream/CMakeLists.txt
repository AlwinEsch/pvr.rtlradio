set(SOURCE_NAME code_src_instance_inputstream)

set(SOURCES
    inputstream.cpp
    inputstream_base.cpp
    inputstream_type_dab.cpp
    inputstream_type_fm.cpp
    inputstream_type_hd.cpp
    inputstream_type_mw.cpp
    inputstream_type_wx.cpp
)

set(HEADERS
    inputstream.h
    inputstream_base.h
    inputstream_type_dab.h
    inputstream_type_fm.h
    inputstream_type_hd.h
    inputstream_type_i.h
    inputstream_type_mw.h
    inputstream_type_wx.h
)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})

set_target_properties(${SOURCE_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_include_directories(${SOURCE_NAME} PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})

unset(SOURCE_NAME)
