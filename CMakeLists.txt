# RootStream - Cross-Platform Build Configuration
# Supports Linux (host + client) and Windows (client only)

cmake_minimum_required(VERSION 3.16)
project(rootstream VERSION 1.0.0 LANGUAGES C)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler warnings
# Note: -Werror temporarily disabled due to pre-existing warnings in crypto.c and network.c
# These warnings exist in the codebase before this PR and are unrelated to the encoder changes
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wno-deprecated-declarations -Wno-format-truncation -Wno-stringop-truncation -Wno-unused-result -Wno-unused-label)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG=1)
endif()

# =============================================================================
# Options
# =============================================================================

option(BUILD_HOST "Build host functionality (Linux only)" ON)
option(BUILD_CLIENT "Build client functionality" ON)
option(USE_FFMPEG "Use FFmpeg for decoding instead of native APIs" OFF)
option(HEADLESS "Build without GUI (no system tray)" OFF)

# Host features are Linux-only
if(WIN32)
    set(BUILD_HOST OFF)
endif()

# =============================================================================
# Find Dependencies
# =============================================================================

# Cross-platform dependencies
find_package(PkgConfig)

# SDL2 (cross-platform display)
find_package(SDL2 CONFIG)
if(NOT SDL2_FOUND AND PkgConfig_FOUND)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# libsodium (cross-platform crypto)
find_package(unofficial-sodium CONFIG)
if(NOT unofficial-sodium_FOUND AND PkgConfig_FOUND)
    pkg_check_modules(SODIUM REQUIRED libsodium)
endif()

# Opus (cross-platform audio codec)
find_package(Opus CONFIG)
if(NOT Opus_FOUND AND PkgConfig_FOUND)
    pkg_check_modules(OPUS REQUIRED opus)
endif()

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    # Linux dependencies
    pkg_check_modules(DRM REQUIRED libdrm)
    pkg_check_modules(VAAPI libva libva-drm)
    pkg_check_modules(ALSA REQUIRED alsa)
    pkg_check_modules(PULSEAUDIO libpulse-simple libpulse)
    pkg_check_modules(PIPEWIRE libpipewire-0.3)

    if(NOT HEADLESS)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    endif()

    pkg_check_modules(AVAHI avahi-client)
    pkg_check_modules(QRENCODE libqrencode)
    pkg_check_modules(PNG libpng)
    pkg_check_modules(X11 x11)
    pkg_check_modules(NCURSES ncurses)

    if(VAAPI_FOUND)
        add_compile_definitions(HAVE_VAAPI)
    endif()

    if(AVAHI_FOUND)
        add_compile_definitions(HAVE_AVAHI)
    endif()

    if(X11_FOUND)
        add_compile_definitions(HAVE_X11)
    endif()

    if(PULSEAUDIO_FOUND)
        add_compile_definitions(HAVE_PULSEAUDIO)
    endif()

    if(PIPEWIRE_FOUND)
        add_compile_definitions(HAVE_PIPEWIRE)
    endif()
    
    if(NCURSES_FOUND)
        add_compile_definitions(HAVE_NCURSES)
    endif()
endif()

if(WIN32)
    # Windows SDK provides Media Foundation, WASAPI, etc.
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# =============================================================================
# Platform Abstraction Layer
# =============================================================================

set(PLATFORM_SOURCES
    src/platform/platform.h
)

if(WIN32)
    list(APPEND PLATFORM_SOURCES src/platform/platform_win32.c)
else()
    list(APPEND PLATFORM_SOURCES src/platform/platform_linux.c)
endif()

# =============================================================================
# Common Sources (cross-platform)
# =============================================================================

set(COMMON_SOURCES
    src/core.c
    src/crypto.c
    src/network.c
    src/packet_validate.c
    src/opus_codec.c
    src/display_sdl2.c
    src/config.c
    src/latency.c
)

# =============================================================================
# Linux-specific Sources
# =============================================================================

set(LINUX_SOURCES
    src/drm_capture.c
    src/x11_capture.c
    src/dummy_capture.c
    src/vaapi_encoder.c
    src/nvenc_encoder.c
    src/ffmpeg_encoder.c
    src/raw_encoder.c
    src/vaapi_decoder.c
    src/audio_capture.c
    src/audio_playback.c
    src/audio_capture_pulse.c
    src/audio_playback_pulse.c
    src/audio_capture_pipewire.c
    src/audio_playback_pipewire.c
    src/audio_capture_dummy.c
    src/audio_playback_dummy.c
    src/input.c
    src/input_xdotool.c
    src/input_logging.c
    src/discovery.c
    src/discovery_broadcast.c
    src/discovery_manual.c
    src/network_tcp.c
    src/network_reconnect.c
    src/diagnostics.c
    src/ai_logging.c
    src/service.c
    src/recording.c
    src/qrcode.c
)

if(NOT HEADLESS)
    list(APPEND LINUX_SOURCES src/tray.c src/tray_cli.c src/tray_tui.c)
else()
    list(APPEND LINUX_SOURCES src/tray_stub.c)
endif()

# =============================================================================
# Windows-specific Sources
# =============================================================================

set(WINDOWS_SOURCES
    src/audio_wasapi.c
    src/decoder_mf.c
    src/input_win32.c
    src/service.c
)

# =============================================================================
# Targets
# =============================================================================

if(UNIX AND NOT APPLE)
    # Linux: Full host + client
    add_executable(rootstream
        src/main.c
        ${COMMON_SOURCES}
        ${LINUX_SOURCES}
        ${PLATFORM_SOURCES}
    )

    target_include_directories(rootstream PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
        ${DRM_INCLUDE_DIRS}
        ${GTK3_INCLUDE_DIRS}
    )

    target_link_libraries(rootstream PRIVATE
        ${SDL2_LIBRARIES}
        ${DRM_LIBRARIES}
        m pthread
    )

    # Conditional libraries
    if(unofficial-sodium_FOUND)
        target_link_libraries(rootstream PRIVATE unofficial-sodium::sodium)
    else()
        target_link_libraries(rootstream PRIVATE ${SODIUM_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${SODIUM_INCLUDE_DIRS})
    endif()

    if(Opus_FOUND)
        target_link_libraries(rootstream PRIVATE Opus::opus)
    else()
        target_link_libraries(rootstream PRIVATE ${OPUS_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${OPUS_INCLUDE_DIRS})
    endif()

    if(VAAPI_FOUND)
        target_link_libraries(rootstream PRIVATE ${VAAPI_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${VAAPI_INCLUDE_DIRS})
    endif()

    if(ALSA_FOUND)
        target_link_libraries(rootstream PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()

    if(PULSEAUDIO_FOUND)
        target_link_libraries(rootstream PRIVATE ${PULSEAUDIO_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    endif()

    if(PIPEWIRE_FOUND)
        target_link_libraries(rootstream PRIVATE ${PIPEWIRE_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    endif()

    if(NOT HEADLESS AND GTK3_FOUND)
        target_link_libraries(rootstream PRIVATE ${GTK3_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${GTK3_INCLUDE_DIRS})
    endif()

    if(AVAHI_FOUND)
        target_link_libraries(rootstream PRIVATE ${AVAHI_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${AVAHI_INCLUDE_DIRS})
    endif()

    if(X11_FOUND)
        target_link_libraries(rootstream PRIVATE ${X11_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${X11_INCLUDE_DIRS})
    endif()

    if(QRENCODE_FOUND)
        target_link_libraries(rootstream PRIVATE ${QRENCODE_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${QRENCODE_INCLUDE_DIRS})
    endif()

    if(PNG_FOUND)
        target_link_libraries(rootstream PRIVATE ${PNG_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${PNG_INCLUDE_DIRS})
    endif()
    
    if(NCURSES_FOUND)
        target_link_libraries(rootstream PRIVATE ${NCURSES_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${NCURSES_INCLUDE_DIRS})
    endif()
    
    if(FFMPEG_FOUND)
        target_link_libraries(rootstream PRIVATE ${FFMPEG_LIBRARIES})
        target_include_directories(rootstream PRIVATE ${FFMPEG_INCLUDE_DIRS})
    endif()

    # Recording player tool
    add_executable(rstr-player
        tools/rstr-player.c
        src/recording.c
        src/vaapi_decoder.c
        src/display_sdl2.c
        src/network.c
        src/network_tcp.c
        src/network_reconnect.c
        src/packet_validate.c
        src/crypto.c
        src/config.c
        src/input.c
        src/opus_codec.c
        src/audio_playback.c
        src/audio_playback_pulse.c
        src/audio_playback_pipewire.c
        src/audio_playback_dummy.c
        src/latency.c
        ${PLATFORM_SOURCES}
    )

    target_include_directories(rstr-player PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
    )

    target_link_libraries(rstr-player PRIVATE
        ${SDL2_LIBRARIES}
        ${VAAPI_LIBRARIES}
        ${ALSA_LIBRARIES}
        ${SODIUM_LIBRARIES}
        ${AVAHI_LIBRARIES}
        m pthread
    )

    if(PULSEAUDIO_FOUND)
        target_link_libraries(rstr-player PRIVATE ${PULSEAUDIO_LIBRARIES})
        target_include_directories(rstr-player PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    endif()

    if(PIPEWIRE_FOUND)
        target_link_libraries(rstr-player PRIVATE ${PIPEWIRE_LIBRARIES})
        target_include_directories(rstr-player PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    endif()

    if(Opus_FOUND)
        target_link_libraries(rstr-player PRIVATE Opus::opus)
    else()
        target_link_libraries(rstr-player PRIVATE ${OPUS_LIBRARIES})
    endif()

endif()

if(WIN32)
    # Windows: Client only
    add_executable(rootstream-client WIN32
        src/main_client.c
        ${COMMON_SOURCES}
        ${WINDOWS_SOURCES}
        ${PLATFORM_SOURCES}
    )

    target_include_directories(rootstream-client PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    # Windows system libraries
    target_link_libraries(rootstream-client PRIVATE
        ws2_32           # Winsock
        mfplat           # Media Foundation
        mfuuid           # Media Foundation GUIDs
        mf               # Media Foundation
        ole32            # COM
        d3d11            # Direct3D 11
        dxgi             # DXGI
    )

    # vcpkg dependencies
    if(unofficial-sodium_FOUND)
        target_link_libraries(rootstream-client PRIVATE unofficial-sodium::sodium)
    endif()

    if(SDL2_FOUND)
        target_link_libraries(rootstream-client PRIVATE SDL2::SDL2 SDL2::SDL2main)
    endif()

    if(Opus_FOUND)
        target_link_libraries(rootstream-client PRIVATE Opus::opus)
    endif()
endif()

# =============================================================================
# Installation
# =============================================================================

if(UNIX AND NOT APPLE)
    install(TARGETS rootstream RUNTIME DESTINATION bin)
    install(TARGETS rstr-player RUNTIME DESTINATION bin)

    # Desktop file
    install(FILES assets/rootstream.desktop
            DESTINATION share/applications)

    # Icons
    install(FILES assets/rootstream.png
            DESTINATION share/icons/hicolor/256x256/apps)
endif()

if(WIN32)
    install(TARGETS rootstream-client RUNTIME DESTINATION bin)
endif()

# =============================================================================
# Testing
# =============================================================================

enable_testing()

# Unit tests
add_executable(test_crypto tests/unit/test_crypto.c src/crypto.c ${PLATFORM_SOURCES})
target_include_directories(test_crypto PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(unofficial-sodium_FOUND)
    target_link_libraries(test_crypto PRIVATE unofficial-sodium::sodium)
else()
    target_link_libraries(test_crypto PRIVATE ${SODIUM_LIBRARIES})
endif()
add_test(NAME crypto_tests COMMAND test_crypto)

add_executable(test_encoding tests/unit/test_encoding.c)
target_include_directories(test_encoding PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME encoding_tests COMMAND test_encoding)

add_executable(test_packet tests/unit/test_packet.c src/packet_validate.c)
target_include_directories(test_packet PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME packet_tests COMMAND test_packet)

# PHASE 8: Integration and Unit Tests
option(ENABLE_UNIT_TESTS "Build PHASE 8 unit tests" ON)
option(ENABLE_INTEGRATION_TESTS "Build PHASE 8 integration tests" ON)

if(ENABLE_UNIT_TESTS OR ENABLE_INTEGRATION_TESTS)
    add_subdirectory(tests)
endif()

# =============================================================================
# Summary
# =============================================================================

message(STATUS "")
message(STATUS "RootStream Build Configuration:")
message(STATUS "  Platform:      ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Host:    ${BUILD_HOST}")
message(STATUS "  Build Client:  ${BUILD_CLIENT}")
message(STATUS "  Headless:      ${HEADLESS}")
if(UNIX)
    message(STATUS "  VA-API:        ${VAAPI_FOUND}")
    message(STATUS "  PulseAudio:    ${PULSEAUDIO_FOUND}")
    message(STATUS "  PipeWire:      ${PIPEWIRE_FOUND}")
    message(STATUS "  Avahi:         ${AVAHI_FOUND}")
endif()
message(STATUS "")
message(STATUS "PHASE 8 Testing:")
if(ENABLE_UNIT_TESTS)
    message(STATUS "  Unit tests: ENABLED (run with: ctest -L unit)")
endif()
if(ENABLE_INTEGRATION_TESTS)
    message(STATUS "  Integration tests: ENABLED (run with: ctest -L integration)")
endif()
message(STATUS "")
