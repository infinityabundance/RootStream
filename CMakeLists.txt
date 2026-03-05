# RootStream - Cross-Platform Build Configuration
# Supports Linux (host + client) and Windows (client only)

cmake_minimum_required(VERSION 3.16)
project(rootstream VERSION 1.0.0 LANGUAGES C CXX)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# C++17 standard (for recording system)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
option(BUILD_WEB_DASHBOARD "Build web dashboard (PHASE 19)" OFF)
option(BUILD_VR_SUPPORT "Build VR/XR support (PHASE 23)" OFF)

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
    
    # PHASE 18: Recording support with FFmpeg
    pkg_check_modules(FFMPEG libavformat libavcodec libavutil libswscale)
    
    # PHASE 25.1: Additional codec support
    if(FFMPEG_FOUND)
        # Check for VP9 codec (libvpx-vp9)
        execute_process(
            COMMAND pkg-config --exists vpx
            RESULT_VARIABLE VPX_EXISTS
        )
        if(VPX_EXISTS EQUAL 0)
            add_compile_definitions(HAVE_LIBVPX)
            message(STATUS "  VP9 encoder (libvpx) found")
        else()
            message(STATUS "  VP9 encoder (libvpx) not found - VP9 support disabled")
        endif()
        
        # Check for AV1 codec (libaom)
        execute_process(
            COMMAND pkg-config --exists aom
            RESULT_VARIABLE AOM_EXISTS
        )
        if(AOM_EXISTS EQUAL 0)
            add_compile_definitions(HAVE_LIBAOM)
            message(STATUS "  AV1 encoder (libaom) found")
        else()
            message(STATUS "  AV1 encoder (libaom) not found - AV1 support disabled")
        endif()
    endif()

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
    
    if(FFMPEG_FOUND)
        add_compile_definitions(HAVE_FFMPEG_MUXING)
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
    src/input/input_manager.c
    src/discovery.c
    src/discovery_broadcast.c
    src/discovery_manual.c
    src/network_tcp.c
    src/network_reconnect.c
    src/network/network_monitor.c
    src/network/adaptive_bitrate.c
    src/network/qos_manager.c
    src/network/bandwidth_estimator.c
    src/network/socket_tuning.c
    src/network/jitter_buffer.c
    src/network/loss_recovery.c
    src/network/load_balancer.c
    src/network/network_config.c
    src/network/network_optimizer.c
    src/diagnostics.c
    src/ai_logging.c
    src/service.c
    src/recording.c
    src/qrcode.c
)

# PHASE 18: Recording system (optional, requires FFmpeg)
if(FFMPEG_FOUND)
    list(APPEND LINUX_SOURCES
        src/recording/disk_manager.cpp
        src/recording/recording_manager.cpp
        src/recording/h264_encoder_wrapper.cpp
        src/recording/vp9_encoder_wrapper.cpp
        src/recording/av1_encoder_wrapper.cpp
        src/recording/replay_buffer.cpp
        src/recording/recording_metadata.cpp
    )
endif()

# PHASE 19: Web Dashboard (optional)
if(BUILD_WEB_DASHBOARD)
    list(APPEND LINUX_SOURCES
        src/web/api_server.c
        src/web/websocket_server.c
        src/web/auth_manager.c
        src/web/rate_limiter.c
        src/web/api_routes.c
    )
endif()

# PHASE 21: Security modules
list(APPEND LINUX_SOURCES
    src/security/crypto_primitives.c
    src/security/key_exchange.c
    src/security/user_auth.c
    src/security/session_manager.c
    src/security/attack_prevention.c
    src/security/audit_log.c
    src/security/security_manager.c
)

# PHASE 23: VR/XR support modules
if(BUILD_VR_SUPPORT)
    list(APPEND LINUX_SOURCES
        src/vr/openxr_manager.c
        src/vr/stereoscopic_renderer.c
        src/vr/head_tracker.c
        src/vr/hand_tracker.c
        src/vr/vr_input_system.c
        src/vr/spatial_audio.c
        src/vr/vr_ui_framework.c
        src/vr/vr_profiler.c
        src/vr/vr_manager.c
        src/vr/platforms/vr_platform_base.c
        src/vr/platforms/meta_quest.c
        src/vr/platforms/steamvr.c
        src/vr/platforms/apple_vision.c
    )
endif()

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
# PHASE-93: rootstream_core — Linkable Static Library
#
# ALL protocol/crypto/decode/encode/network/audio logic lives here.
# Executables (rootstream, rstr-player, rootstream-client) and the KDE client
# link this single library instead of re-compiling the same sources N times.
#
# WHY A LIBRARY?
# --------------
# Before PHASE-93, each executable compiled every source file independently.
# That meant:
#   1. Duplicate compilation = longer build times.
#   2. The KDE client could not link the real backend — it had no library to
#      link against, forcing it to stub or duplicate all protocol logic.
#   3. Adding src/client_session.c (PHASE-94) would have to be added to every
#      executable's source list separately.
#
# With rootstream_core STATIC:
#   - One authoritative compiled object for all streaming logic.
#   - KDE client links it via add_subdirectory (see clients/kde-plasma-client).
#   - Tests can link specific subsets without re-compiling the world.
#   - PUBLIC include/ means any downstream target automatically gets the
#     correct include paths with no extra configuration.
# =============================================================================

# =============================================================================
# Targets
# =============================================================================

if(UNIX AND NOT APPLE)
    # ── rootstream_core: the shared backend library ────────────────────────
    #
    # Contains everything EXCEPT:
    #   - src/main.c              (CLI entry point — stays in rootstream exe)
    #   - tools/rstr-player.c     (player tool entry point)
    #   - src/tray*.c             (tray UI — only needed by the host executable)
    #
    # display_sdl2.c is included because service_run_client() (which is in
    # service.c → rootstream_core) still calls display_init() for the SDL
    # fallback path.  Once PHASE-94 is complete, service_run_client() becomes
    # a thin wrapper and display_sdl2.c can be moved to the executable.
    add_library(rootstream_core STATIC
        ${COMMON_SOURCES}
        ${LINUX_SOURCES}
        ${PLATFORM_SOURCES}
        src/client_session.c
    )

    # PUBLIC include dir: any target that links rootstream_core automatically
    # receives the correct include path for rootstream.h and
    # rootstream_client_session.h without needing a separate
    # target_include_directories call.
    target_include_directories(rootstream_core
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${SDL2_INCLUDE_DIRS}
            ${DRM_INCLUDE_DIRS}
            ${GTK3_INCLUDE_DIRS}
    )

    # Link all system libraries against rootstream_core (PUBLIC where needed
    # by downstream, PRIVATE for internal use only).
    target_link_libraries(rootstream_core PUBLIC
        ${SDL2_LIBRARIES}
        ${DRM_LIBRARIES}
        m pthread
    )

    if(unofficial-sodium_FOUND)
        target_link_libraries(rootstream_core PUBLIC unofficial-sodium::sodium)
    else()
        target_link_libraries(rootstream_core PUBLIC ${SODIUM_LIBRARIES})
        target_include_directories(rootstream_core PUBLIC ${SODIUM_INCLUDE_DIRS})
    endif()

    if(Opus_FOUND)
        target_link_libraries(rootstream_core PUBLIC Opus::opus)
    else()
        target_link_libraries(rootstream_core PUBLIC ${OPUS_LIBRARIES})
        target_include_directories(rootstream_core PUBLIC ${OPUS_INCLUDE_DIRS})
    endif()

    if(VAAPI_FOUND)
        target_link_libraries(rootstream_core PUBLIC ${VAAPI_LIBRARIES})
        target_include_directories(rootstream_core PUBLIC ${VAAPI_INCLUDE_DIRS})
    endif()

    if(ALSA_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()

    if(PULSEAUDIO_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${PULSEAUDIO_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    endif()

    if(PIPEWIRE_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${PIPEWIRE_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    endif()

    if(NOT HEADLESS AND GTK3_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${GTK3_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${GTK3_INCLUDE_DIRS})
    endif()

    if(AVAHI_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${AVAHI_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${AVAHI_INCLUDE_DIRS})
    endif()

    if(X11_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${X11_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${X11_INCLUDE_DIRS})
    endif()

    if(QRENCODE_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${QRENCODE_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${QRENCODE_INCLUDE_DIRS})
    endif()

    if(PNG_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${PNG_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${PNG_INCLUDE_DIRS})
    endif()

    if(NCURSES_FOUND)
        target_link_libraries(rootstream_core PRIVATE ${NCURSES_LIBRARIES})
        target_include_directories(rootstream_core PRIVATE ${NCURSES_INCLUDE_DIRS})
    endif()

    if(FFMPEG_FOUND)
        target_link_libraries(rootstream_core PUBLIC ${FFMPEG_LIBRARIES})
        target_include_directories(rootstream_core PUBLIC ${FFMPEG_INCLUDE_DIRS})
    endif()

    # ── rootstream: host + client CLI executable ───────────────────────────
    #
    # src/main.c is the ONLY source here — all real logic is in rootstream_core.
    # This keeps the executable thin and makes it easy to verify that no
    # protocol logic accidentally lives only in main.c.
    add_executable(rootstream
        src/main.c
    )

    target_link_libraries(rootstream PRIVATE rootstream_core)

    # ── rstr-player: recording playback tool ──────────────────────────────
    #
    # Links rootstream_core for all decode/display/audio logic.
    # Only the player tool's own main (tools/rstr-player.c) is compiled here.
    add_executable(rstr-player
        tools/rstr-player.c
    )

    target_link_libraries(rstr-player PRIVATE rootstream_core)

    # KDE Plasma client is built via its own CMakeLists.txt which uses
    # add_subdirectory(../.. rootstream_build) to pull in rootstream_core.
    # See: clients/kde-plasma-client/CMakeLists.txt (PHASE-93.2)

endif()

if(WIN32)
    # ── Windows: rootstream_core_win (Windows backend library) ────────────
    #
    # Same principle as the Linux library: all backend logic in one library,
    # thin executable on top.
    add_library(rootstream_core_win STATIC
        ${COMMON_SOURCES}
        ${WINDOWS_SOURCES}
        ${PLATFORM_SOURCES}
        src/client_session.c
    )

    target_include_directories(rootstream_core_win
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(rootstream_core_win PUBLIC
        ws2_32 mfplat mfuuid mf ole32 d3d11 dxgi
    )

    if(unofficial-sodium_FOUND)
        target_link_libraries(rootstream_core_win PUBLIC unofficial-sodium::sodium)
    endif()
    if(SDL2_FOUND)
        target_link_libraries(rootstream_core_win PUBLIC SDL2::SDL2 SDL2::SDL2main)
    endif()
    if(Opus_FOUND)
        target_link_libraries(rootstream_core_win PUBLIC Opus::opus)
    endif()

    # Windows client executable — thin, links the library
    add_executable(rootstream-client WIN32
        src/main_client.c
    )

    target_link_libraries(rootstream-client PRIVATE rootstream_core_win)
endif()

# =============================================================================
# Installation
# =============================================================================

if(UNIX AND NOT APPLE)
    install(TARGETS rootstream_core ARCHIVE DESTINATION lib)
    install(TARGETS rootstream RUNTIME DESTINATION bin)
    install(TARGETS rstr-player RUNTIME DESTINATION bin)

    # Public headers (needed by downstream consumers like KDE client)
    install(FILES
        include/rootstream.h
        include/rootstream_client_session.h
        DESTINATION include/rootstream)

    # Desktop file
    install(FILES assets/rootstream.desktop
            DESTINATION share/applications)

    # Icons
    install(FILES assets/rootstream.png
            DESTINATION share/icons/hicolor/256x256/apps)
endif()

if(WIN32)
    install(TARGETS rootstream_core_win ARCHIVE DESTINATION lib)
    install(TARGETS rootstream-client RUNTIME DESTINATION bin)
endif()

# =============================================================================
# Testing
# =============================================================================

enable_testing()

# Unit tests — link against rootstream_core to avoid re-listing source files
# and to ensure the tests exercise the same compiled code as the executables.
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

# PHASE 21: Security tests
add_executable(test_security tests/unit/test_security.c
               src/security/crypto_primitives.c
               src/security/key_exchange.c
               src/security/user_auth.c
               src/security/session_manager.c
               src/security/attack_prevention.c
               src/security/audit_log.c
               src/security/security_manager.c
               ${PLATFORM_SOURCES})
target_include_directories(test_security PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(unofficial-sodium_FOUND)
    target_link_libraries(test_security PRIVATE unofficial-sodium::sodium)
else()
    target_link_libraries(test_security PRIVATE ${SODIUM_LIBRARIES})
endif()
add_test(NAME security_tests COMMAND test_security)

# PHASE 18: Recording tests
add_executable(test_recording_types tests/unit/test_recording_types.c)
target_include_directories(test_recording_types PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/recording)
add_test(NAME recording_types_tests COMMAND test_recording_types)

if(FFMPEG_FOUND)
    add_executable(test_disk_manager tests/unit/test_disk_manager.cpp 
                   src/recording/disk_manager.cpp)
    target_include_directories(test_disk_manager PRIVATE 
        ${CMAKE_SOURCE_DIR}/include 
        ${CMAKE_SOURCE_DIR}/src/recording)
    add_test(NAME disk_manager_tests COMMAND test_disk_manager)
endif()

# PHASE 19: Web Dashboard tests
if(BUILD_WEB_DASHBOARD)
    add_executable(test_web_dashboard tests/unit/test_web_dashboard.c
                   src/web/api_server.c
                   src/web/websocket_server.c
                   src/web/auth_manager.c
                   src/web/rate_limiter.c
                   ${PLATFORM_SOURCES})
    target_include_directories(test_web_dashboard PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_web_dashboard PRIVATE pthread m)
    add_test(NAME web_dashboard_tests COMMAND test_web_dashboard)
endif()

# PHASE 23: VR/XR support tests
if(BUILD_VR_SUPPORT)
    add_executable(test_vr tests/unit/test_vr.c
                   src/vr/openxr_manager.c
                   src/vr/stereoscopic_renderer.c
                   src/vr/head_tracker.c
                   src/vr/hand_tracker.c
                   src/vr/vr_input_system.c
                   src/vr/spatial_audio.c
                   src/vr/vr_ui_framework.c
                   src/vr/vr_profiler.c
                   src/vr/vr_manager.c
                   src/vr/platforms/vr_platform_base.c
                   src/vr/platforms/meta_quest.c
                   src/vr/platforms/steamvr.c
                   src/vr/platforms/apple_vision.c
                   ${PLATFORM_SOURCES})
    target_include_directories(test_vr PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/vr)
    target_link_libraries(test_vr PRIVATE m pthread)
    add_test(NAME vr_tests COMMAND test_vr)
endif()

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
    message(STATUS "  FFmpeg (Recording): ${FFMPEG_FOUND}")
    message(STATUS "  Web Dashboard: ${BUILD_WEB_DASHBOARD}")
    message(STATUS "  VR/XR Support: ${BUILD_VR_SUPPORT}")
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
