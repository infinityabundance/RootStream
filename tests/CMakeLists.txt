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
    
    message(STATUS "Unit tests enabled")
endif()
