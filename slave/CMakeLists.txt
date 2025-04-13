# CMake configuration for slave component

# Add this line to your top-level CMakeLists.txt
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define source files
set(SLAVE_SOURCES
    Slave.cpp
    VideoSender.cpp
    AudioSender.cpp
    ControlBoard.cpp
    main.cpp
    Hardware.cpp
    Camera.cpp
)

set(SLAVE_HEADERS
    Slave.h
    VideoSender.h
    AudioSender.h
    ControlBoard.h
    Hardware.h
    Camera.h
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

# Create executable
add_executable(slave ${SLAVE_SOURCES} ${SLAVE_HEADERS})

# Add include directories
target_include_directories(slave PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${GST_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(slave PRIVATE
    common
    ${GST_LIBRARIES}
)

# Add compile definitions and options
target_compile_options(slave PRIVATE
    ${GST_CFLAGS_OTHER}
)

# For installation
install(TARGETS slave
    RUNTIME DESTINATION bin
)