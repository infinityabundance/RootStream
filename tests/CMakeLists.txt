# Tests for RootStream - PHASE 8

enable_testing()

# Common test utilities
add_library(test_harness STATIC
    common/test_harness.c
)
target_include_directories(test_harness PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Integration tests
if(ENABLE_INTEGRATION_TESTS)
    add_executable(test_capture_fallback integration/test_capture_fallback.c)
    target_link_libraries(test_capture_fallback test_harness)
    add_test(NAME CaptureFollback COMMAND test_capture_fallback)
    set_tests_properties(CaptureFollback PROPERTIES LABELS "integration")
    
    add_executable(test_encode_fallback integration/test_encode_fallback.c)
    target_link_libraries(test_encode_fallback test_harness)
    add_test(NAME EncodeFollback COMMAND test_encode_fallback)
    set_tests_properties(EncodeFollback PROPERTIES LABELS "integration")
    
    add_executable(test_audio_fallback integration/test_audio_fallback.c)
    target_link_libraries(test_audio_fallback test_harness)
    add_test(NAME AudioFollback COMMAND test_audio_fallback)
    set_tests_properties(AudioFollback PROPERTIES LABELS "integration")
    
    add_executable(test_network_fallback integration/test_network_fallback.c)
    target_link_libraries(test_network_fallback test_harness)
    add_test(NAME NetworkFollback COMMAND test_network_fallback)
    set_tests_properties(NetworkFollback PROPERTIES LABELS "integration")
    
    add_executable(test_discovery_fallback integration/test_discovery_fallback.c)
    target_link_libraries(test_discovery_fallback test_harness)
    add_test(NAME DiscoveryFollback COMMAND test_discovery_fallback)
    set_tests_properties(DiscoveryFollback PROPERTIES LABELS "integration")
    
    message(STATUS "Integration tests enabled")
endif()

# Unit tests
if(ENABLE_UNIT_TESTS)
    add_executable(test_backends_capture unit/test_backends_capture.c)
    target_link_libraries(test_backends_capture test_harness)
    add_test(NAME BackendsCaptureUnit COMMAND test_backends_capture)
    set_tests_properties(BackendsCaptureUnit PROPERTIES LABELS "unit")
    
    add_executable(test_backends_encode unit/test_backends_encode.c)
    target_link_libraries(test_backends_encode test_harness)
    add_test(NAME BackendsEncodeUnit COMMAND test_backends_encode)
    set_tests_properties(BackendsEncodeUnit PROPERTIES LABELS "unit")
    
    add_executable(test_backends_audio unit/test_backends_audio.c)
    target_link_libraries(test_backends_audio test_harness)
    add_test(NAME BackendsAudioUnit COMMAND test_backends_audio)
    set_tests_properties(BackendsAudioUnit PROPERTIES LABELS "unit")
    
    add_executable(test_diagnostics unit/test_diagnostics.c)
    target_link_libraries(test_diagnostics test_harness)
    add_test(NAME DiagnosticsUnit COMMAND test_diagnostics)
    set_tests_properties(DiagnosticsUnit PROPERTIES LABELS "unit")
    
    add_executable(test_feature_detection unit/test_feature_detection.c)
    target_link_libraries(test_feature_detection test_harness)
    add_test(NAME FeatureDetectionUnit COMMAND test_feature_detection)
    set_tests_properties(FeatureDetectionUnit PROPERTIES LABELS "unit")
    
    # PHASE 15: Input Manager tests
    add_executable(test_input_manager unit/test_input_manager.c
        unit/test_util_stubs.c
        ${CMAKE_SOURCE_DIR}/src/input/input_manager.c
        ${CMAKE_SOURCE_DIR}/src/input_logging.c
        ${CMAKE_SOURCE_DIR}/src/input_xdotool.c
    )
    target_include_directories(test_input_manager PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_input_manager test_harness pthread m)
    if(X11_FOUND)
        target_link_libraries(test_input_manager ${X11_LIBRARIES})
    endif()
    add_test(NAME InputManagerUnit COMMAND test_input_manager)
    set_tests_properties(InputManagerUnit PROPERTIES LABELS "unit")
    
    # PHASE 17: Discovery Cache tests
    add_executable(test_discovery_cache unit/test_discovery_cache.c
        ${CMAKE_SOURCE_DIR}/src/discovery.c
    )
    target_include_directories(test_discovery_cache PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test_discovery_cache test_harness pthread m)
    if(AVAHI_FOUND)
        target_link_libraries(test_discovery_cache ${AVAHI_LIBRARIES})
    endif()
    add_test(NAME DiscoveryCacheUnit COMMAND test_discovery_cache)
    set_tests_properties(DiscoveryCacheUnit PROPERTIES LABELS "unit")
    
    # PHASE 27: Recording system tests (requires FFmpeg)
    if(FFMPEG_FOUND)
        # Container formats test
        add_executable(test_container_formats unit/test_container_formats.cpp)
        target_include_directories(test_container_formats PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/recording)
        target_link_libraries(test_container_formats 
            ${FFMPEG_LIBRARIES}
        )
        add_test(NAME ContainerFormatsUnit COMMAND test_container_formats)
        set_tests_properties(ContainerFormatsUnit PROPERTIES LABELS "unit")
        
        # Replay buffer test
        add_executable(test_replay_buffer unit/test_replay_buffer.cpp
            ${CMAKE_SOURCE_DIR}/src/recording/replay_buffer.cpp
        )
        target_include_directories(test_replay_buffer PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/recording)
        target_link_libraries(test_replay_buffer 
            ${FFMPEG_LIBRARIES}
            pthread
        )
        add_test(NAME ReplayBufferUnit COMMAND test_replay_buffer)
        set_tests_properties(ReplayBufferUnit PROPERTIES LABELS "unit")
        
        # RecordingManager integration test
        add_executable(test_recording_manager_integration unit/test_recording_manager_integration.cpp
            ${CMAKE_SOURCE_DIR}/src/recording/recording_manager.cpp
            ${CMAKE_SOURCE_DIR}/src/recording/disk_manager.cpp
            ${CMAKE_SOURCE_DIR}/src/recording/replay_buffer.cpp
            ${CMAKE_SOURCE_DIR}/src/recording/recording_metadata.cpp
        )
        target_include_directories(test_recording_manager_integration PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/recording)
        target_link_libraries(test_recording_manager_integration 
            ${FFMPEG_LIBRARIES}
            pthread
        )
        add_test(NAME RecordingManagerIntegrationUnit COMMAND test_recording_manager_integration)
        set_tests_properties(RecordingManagerIntegrationUnit PROPERTIES LABELS "unit")
        
        message(STATUS "Recording system tests enabled (FFmpeg found)")
    else()
        message(STATUS "Recording system tests skipped (FFmpeg not found)")
    endif()
    
    message(STATUS "Unit tests enabled")
endif()
