set(SOURCE_NAME code_src_audio)

set(SOURCES audio_pipeline.cpp)

set(HEADERS audio_pipeline.h
            frame.h
            ring_buffer.h)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})

target_include_directories(${SOURCE_NAME} PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})

unset(SOURCE_NAME)