set(SOURCE_NAME code_src_dsp_dab)

set(SOURCES dummy.cpp)

set(HEADERS app_ofdm_blocks.h
            block_frequencies.h)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})
target_include_directories(${SOURCE_NAME} PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})

unset(SOURCE_NAME)
