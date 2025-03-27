set(COMMON_SOURCES
    Message.cpp
    Message.h
    Transmitter.cpp
    Transmitter.h
    Event.cpp
    Event.h
)

add_library(common STATIC ${COMMON_SOURCES})

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)