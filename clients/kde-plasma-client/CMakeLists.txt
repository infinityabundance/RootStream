# RootStream KDE Plasma Client
cmake_minimum_required(VERSION 3.16)
project(rootstream-kde-client VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(ENABLE_AI_LOGGING "Enable AI logging support" ON)
option(ENABLE_RENDERER_OPENGL "Enable OpenGL renderer backend" ON)
option(ENABLE_RENDERER_VULKAN "Enable Vulkan renderer backend" ON)
option(ENABLE_RENDERER_PROTON "Enable Proton compatibility layer" ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets OpenGL)

# Find OpenGL
if(ENABLE_RENDERER_OPENGL)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
endif()

# Find Vulkan
if(ENABLE_RENDERER_VULKAN)
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message(WARNING "Vulkan not found, disabling Vulkan renderer")
        set(ENABLE_RENDERER_VULKAN OFF)
    endif()
endif()

# Find KDE Frameworks  
find_package(ECM QUIET NO_MODULE)
if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    find_package(KF6 COMPONENTS Config CoreAddons)
endif()

# Find other dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)
pkg_check_modules(SAMPLERATE REQUIRED samplerate)
find_package(ALSA REQUIRED)
pkg_check_modules(VAAPI libva libva-drm)
pkg_check_modules(PULSEAUDIO libpulse-simple libpulse)
pkg_check_modules(PIPEWIRE libpipewire-0.3)

# Wayland (for Vulkan renderer)
if(ENABLE_RENDERER_VULKAN)
    pkg_check_modules(WAYLAND wayland-client)
    if(NOT WAYLAND_FOUND)
        message(STATUS "Wayland not found, Vulkan Wayland backend will be limited")
    endif()
endif()

# Include RootStream library
include_directories(${CMAKE_SOURCE_DIR}/../../include)
include_directories(${CMAKE_SOURCE_DIR}/../../src)

# Sources
set(SOURCES
    src/main.cpp
    src/rootstreamclient.cpp
    src/peermanager.cpp
    src/videorenderer.cpp
    src/audioplayer.cpp
    src/inputmanager.cpp
    src/settingsmanager.cpp
    src/logmanager.cpp
    src/connectiondialog.cpp
    src/mainwindow.cpp
    # Audio components
    src/audio/audio_player.cpp
    src/audio/opus_decoder.cpp
    src/audio/audio_ring_buffer.cpp
    src/audio/audio_resampler.cpp
    src/audio/audio_sync.cpp
    src/audio/audio_backend_selector.cpp
    src/audio/playback_pulseaudio.cpp
    src/audio/playback_pipewire.cpp
    src/audio/playback_alsa.cpp
)

# Renderer sources (C)
if(ENABLE_RENDERER_OPENGL)
    list(APPEND SOURCES
        src/renderer/renderer.c
        src/renderer/opengl_renderer.c
        src/renderer/opengl_utils.c
        src/renderer/color_space.c
        src/renderer/frame_buffer.c
    )
endif()

# Vulkan renderer sources
if(ENABLE_RENDERER_VULKAN)
    list(APPEND SOURCES
        src/renderer/renderer.c
        src/renderer/vulkan_renderer.c
        src/renderer/vulkan_wayland.c
        src/renderer/vulkan_x11.c
        src/renderer/vulkan_headless.c
        src/renderer/color_space.c
        src/renderer/frame_buffer.c
    )
endif()

# Proton renderer sources
if(ENABLE_RENDERER_PROTON)
    list(APPEND SOURCES
        src/renderer/proton_detector.c
        src/renderer/proton_renderer.c
        src/renderer/dxvk_interop.c
        src/renderer/vkd3d_interop.c
        src/renderer/proton_game_db.c
        src/renderer/proton_settings.c
    )
endif()

# Headers (for MOC)
set(HEADERS
    src/rootstreamclient.h
    src/peermanager.h
    src/videorenderer.h
    src/audioplayer.h
    src/inputmanager.h
    src/settingsmanager.h
    src/logmanager.h
    src/connectiondialog.h
    src/mainwindow.h
)

# QML resources
qt6_add_resources(QML_RESOURCES qml/qml.qrc)

# Executable
add_executable(rootstream-kde-client
    ${SOURCES}
    ${QML_RESOURCES}
)

target_link_libraries(rootstream-kde-client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
    Qt6::OpenGL
    ${OPUS_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
    ${ALSA_LIBRARIES}
)

# Link OpenGL if renderer is enabled
if(ENABLE_RENDERER_OPENGL)
    target_link_libraries(rootstream-kde-client PRIVATE
        OpenGL::GL
        ${X11_LIBRARIES}
    )
    target_include_directories(rootstream-kde-client PRIVATE
        ${X11_INCLUDE_DIR}
    )
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_OPENGL_RENDERER)
endif()

# Link Vulkan if renderer is enabled
if(ENABLE_RENDERER_VULKAN)
    target_link_libraries(rootstream-kde-client PRIVATE
        Vulkan::Vulkan
        ${X11_LIBRARIES}
    )
    target_include_directories(rootstream-kde-client PRIVATE
        ${X11_INCLUDE_DIR}
        ${Vulkan_INCLUDE_DIRS}
    )
    if(WAYLAND_FOUND)
        target_link_libraries(rootstream-kde-client PRIVATE ${WAYLAND_LIBRARIES})
        target_include_directories(rootstream-kde-client PRIVATE ${WAYLAND_INCLUDE_DIRS})
    endif()
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_VULKAN_RENDERER)
endif()

# Link Proton renderer dependencies
if(ENABLE_RENDERER_PROTON)
    # Proton renderer requires Vulkan as backend
    if(NOT ENABLE_RENDERER_VULKAN)
        message(WARNING "Proton renderer requires Vulkan backend, enabling ENABLE_RENDERER_VULKAN")
        set(ENABLE_RENDERER_VULKAN ON)
    endif()
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_PROTON_RENDERER)
endif()

# Link KDE Frameworks if available
if(KF6_FOUND)
    target_link_libraries(rootstream-kde-client PRIVATE
        KF6::ConfigCore
        KF6::CoreAddons
    )
endif()

# Add include directories for audio libraries
target_include_directories(rootstream-kde-client PRIVATE
    ${OPUS_INCLUDE_DIRS}
    ${SAMPLERATE_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
)

# Link PulseAudio if available
if(PULSEAUDIO_FOUND)
    target_link_libraries(rootstream-kde-client PRIVATE ${PULSEAUDIO_LIBRARIES})
    target_include_directories(rootstream-kde-client PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_PULSEAUDIO)
endif()

# Link PipeWire if available
if(PIPEWIRE_FOUND)
    target_link_libraries(rootstream-kde-client PRIVATE ${PIPEWIRE_LIBRARIES})
    target_include_directories(rootstream-kde-client PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_PIPEWIRE)
endif()

# Link VA-API if available
if(VAAPI_FOUND)
    target_link_libraries(rootstream-kde-client PRIVATE ${VAAPI_LIBRARIES})
    target_include_directories(rootstream-kde-client PRIVATE ${VAAPI_INCLUDE_DIRS})
    target_compile_definitions(rootstream-kde-client PRIVATE HAVE_VAAPI)
endif()

# Link RootStream library (assume it's built separately)
target_link_libraries(rootstream-kde-client PRIVATE
    ${CMAKE_SOURCE_DIR}/../../build/librootstream.a
    sodium
    m
    pthread
)

# AI Logging support
if(ENABLE_AI_LOGGING)
    target_compile_definitions(rootstream-kde-client PRIVATE ENABLE_AI_LOGGING)
endif()

# Installation
install(TARGETS rootstream-kde-client RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/rootstream-kde-client.desktop
        DESTINATION share/applications)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME rootstream-kde-client.svg)

# Tests
enable_testing()
add_subdirectory(tests)

# Summary
message(STATUS "")
message(STATUS "RootStream KDE Plasma Client Configuration:")
message(STATUS "  Qt Version:     ${Qt6_VERSION}")
message(STATUS "  KDE Frameworks: ${KF6_FOUND}")
message(STATUS "  VA-API:         ${VAAPI_FOUND}")
message(STATUS "  PulseAudio:     ${PULSEAUDIO_FOUND}")
message(STATUS "  PipeWire:       ${PIPEWIRE_FOUND}")
message(STATUS "  AI Logging:     ${ENABLE_AI_LOGGING}")
message(STATUS "  OpenGL Renderer: ${ENABLE_RENDERER_OPENGL}")
message(STATUS "  Vulkan Renderer: ${ENABLE_RENDERER_VULKAN}")
if(ENABLE_RENDERER_VULKAN)
    message(STATUS "    - Wayland:    ${WAYLAND_FOUND}")
    message(STATUS "    - X11:        ${X11_FOUND}")
endif()
message(STATUS "  Proton Renderer: ${ENABLE_RENDERER_PROTON}")
message(STATUS "")
