set(SOURCE_NAME code_src_utils)

set(SOURCES frequency_name.cpp
            log.cpp)

set(HEADERS align.h
            app_io_buffers.h
            frequency_name.h
            log.h
            raw_iq.h
            scalar_condition.h
            source_location.hpp
            span.h)

add_library(${SOURCE_NAME} OBJECT ${SOURCES} ${HEADERS})

target_include_directories(${SOURCE_NAME} PRIVATE ${KODI_INCLUDE_DIRS}
                                          PRIVATE ${LIBFMT_INCLUDE_DIRS}
                                          PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${SOURCE_NAME} ${DEPLIBS})

unset(SOURCE_NAME)