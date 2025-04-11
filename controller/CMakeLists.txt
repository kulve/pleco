# CMake configuration for controller component

# Define source files
set(CONTROLLER_SOURCES
    main.cpp
    Controller.cpp
    Controller-sdl.cpp
    VideoReceiver.cpp
    AudioReceiver.cpp
    Joystick.cpp
)

set(CONTROLLER_HEADERS
    Controller.h
    Controller-sdl.h
    VideoReceiver.h
    AudioReceiver.h
    Joystick.h
)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find D-Bus
pkg_check_modules(DBUS REQUIRED dbus-1)

pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

# Find SDL2 packages
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Find libdrm
pkg_check_modules(LIBDRM REQUIRED libdrm)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system)

# Create executable
add_executable(controller
    ${CONTROLLER_SOURCES}
    ${CONTROLLER_HEADERS}
    ${IMGUI_SOURCES}
)

# Add include directories
target_include_directories(controller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${GST_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Link libraries
target_link_libraries(controller PRIVATE
    common
    ${GST_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
)

# Add compile definitions and options
target_compile_options(controller PRIVATE
    -funsigned-char
    -Werror
    ${GST_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
)

# Install target
install(TARGETS controller
    RUNTIME DESTINATION bin
)