#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/vr/openxr_manager.h"
#include "../src/vr/stereoscopic_renderer.h"
#include "../src/vr/head_tracker.h"
#include "../src/vr/hand_tracker.h"
#include "../src/vr/vr_input_system.h"
#include "../src/vr/spatial_audio.h"
#include "../src/vr/vr_ui_framework.h"
#include "../src/vr/vr_profiler.h"
#include "../src/vr/vr_manager.h"
#include "../src/vr/platforms/vr_platform_base.h"
#include "../src/vr/platforms/meta_quest.h"
#include "../src/vr/platforms/steamvr.h"
#include "../src/vr/platforms/apple_vision.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", message); \
            return 1; \
        } \
    } while(0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", message); \
    } while(0)

// Test OpenXR Manager
static int test_openxr_manager(void) {
    printf("\n=== Testing OpenXR Manager ===\n");
    
    OpenXRManager *manager = openxr_manager_create();
    TEST_ASSERT(manager != NULL, "OpenXR manager creation");
    
    int result = openxr_manager_init(manager);
    TEST_ASSERT(result == 0, "OpenXR manager initialization");
    
    result = openxr_manager_create_session(manager);
    TEST_ASSERT(result == 0, "OpenXR session creation");
    
    bool isActive = openxr_manager_is_tracking_active(manager);
    TEST_ASSERT(isActive == true, "OpenXR tracking active");
    
    XRState state = openxr_manager_get_tracking_data(manager);
    TEST_ASSERT(state.headOrientation.w == 1.0f, "Default head orientation");
    
    uint32_t width, height;
    result = openxr_manager_get_recommended_resolution(manager, &width, &height);
    TEST_ASSERT(result == 0 && width > 0 && height > 0, "Get recommended resolution");
    
    openxr_manager_cleanup(manager);
    openxr_manager_destroy(manager);
    
    TEST_PASS("OpenXR Manager tests");
    return 0;
}

// Test Stereoscopic Renderer
static int test_stereoscopic_renderer(void) {
    printf("\n=== Testing Stereoscopic Renderer ===\n");
    
    StereoscopicRenderer *renderer = stereoscopic_renderer_create();
    TEST_ASSERT(renderer != NULL, "Stereoscopic renderer creation");
    
    int result = stereoscopic_renderer_init(renderer, 2048, 2048);
    TEST_ASSERT(result == 0, "Stereoscopic renderer initialization");
    
    result = stereoscopic_renderer_resize(renderer, 1024, 1024);
    TEST_ASSERT(result == 0, "Stereoscopic renderer resize");
    
    stereoscopic_renderer_cleanup(renderer);
    stereoscopic_renderer_destroy(renderer);
    
    TEST_PASS("Stereoscopic Renderer tests");
    return 0;
}

// Test Head Tracker
static int test_head_tracker(void) {
    printf("\n=== Testing Head Tracker ===\n");
    
    HeadTracker *tracker = head_tracker_create();
    TEST_ASSERT(tracker != NULL, "Head tracker creation");
    
    int result = head_tracker_init(tracker);
    TEST_ASSERT(result == 0, "Head tracker initialization");
    
    bool isActive = head_tracker_is_active(tracker);
    TEST_ASSERT(isActive == true, "Head tracker active");
    
    XrPosef testPose = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 2.0f}};
    result = head_tracker_update_pose(tracker, &testPose);
    TEST_ASSERT(result == 0, "Head tracker pose update");
    
    HeadTrackingData data = head_tracker_get_pose(tracker, 0);
    TEST_ASSERT(data.position.y == 1.0f, "Head tracker position");
    
    HeadTrackingData predicted = head_tracker_predict_pose(tracker, 16);
    TEST_ASSERT(predicted.timestamp_us > 0, "Head tracker prediction");
    
    float confidence = head_tracker_get_confidence(tracker);
    TEST_ASSERT(confidence >= 0.0f && confidence <= 1.0f, "Head tracker confidence");
    
    XrVector3f forward = head_tracker_get_forward(tracker);
    TEST_ASSERT(forward.z == -1.0f, "Head tracker forward vector");
    
    head_tracker_cleanup(tracker);
    head_tracker_destroy(tracker);
    
    TEST_PASS("Head Tracker tests");
    return 0;
}

// Test Hand Tracker
static int test_hand_tracker(void) {
    printf("\n=== Testing Hand Tracker ===\n");
    
    HandTracker *tracker = hand_tracker_create();
    TEST_ASSERT(tracker != NULL, "Hand tracker creation");
    
    int result = hand_tracker_init(tracker);
    TEST_ASSERT(result == 0, "Hand tracker initialization");
    
    XrPosef leftHandPose = {{1.0f, 0.0f, 0.0f, 0.0f}, {-0.5f, 1.0f, 0.0f}};
    result = hand_tracker_update(tracker, HAND_LEFT, &leftHandPose);
    TEST_ASSERT(result == 0, "Hand tracker update");
    
    bool isTracked = hand_tracker_is_tracked(tracker, HAND_LEFT);
    TEST_ASSERT(isTracked == true, "Hand is tracked");
    
    HandState state = hand_tracker_get_state(tracker, HAND_LEFT);
    TEST_ASSERT(state.palmPosition.x == -0.5f, "Hand palm position");
    
    hand_tracker_cleanup(tracker);
    hand_tracker_destroy(tracker);
    
    TEST_PASS("Hand Tracker tests");
    return 0;
}

// Test VR Input System
static int test_vr_input_system(void) {
    printf("\n=== Testing VR Input System ===\n");
    
    VRInputSystem *system = vr_input_system_create();
    TEST_ASSERT(system != NULL, "VR input system creation");
    
    int result = vr_input_system_init(system);
    TEST_ASSERT(result == 0, "VR input system initialization");
    
    XRInputState xrInput = {0};
    xrInput.leftTrigger = 0.8f;
    xrInput.rightTrigger = 0.3f;
    
    result = vr_input_system_update(system, &xrInput);
    TEST_ASSERT(result == 0, "VR input system update");
    
    ControllerInput leftCtrl = vr_input_system_get_controller(system, HAND_LEFT);
    TEST_ASSERT(leftCtrl.triggerValue == 0.8f, "Left trigger value");
    TEST_ASSERT(leftCtrl.triggerPressed == true, "Left trigger pressed");
    
    result = vr_input_system_vibrate(system, HAND_RIGHT, 0.5f, 100.0f);
    TEST_ASSERT(result == 0, "Controller vibration");
    
    vr_input_system_cleanup(system);
    vr_input_system_destroy(system);
    
    TEST_PASS("VR Input System tests");
    return 0;
}

// Test Spatial Audio
static int test_spatial_audio(void) {
    printf("\n=== Testing Spatial Audio ===\n");
    
    SpatialAudioEngine *engine = spatial_audio_engine_create();
    TEST_ASSERT(engine != NULL, "Spatial audio engine creation");
    
    int result = spatial_audio_engine_init(engine);
    TEST_ASSERT(result == 0, "Spatial audio engine initialization");
    
    XrVector3f sourcePos = {1.0f, 0.0f, 0.0f};
    uint32_t sourceId = spatial_audio_create_source(engine, &sourcePos, 10.0f);
    TEST_ASSERT(sourceId > 0, "Audio source creation");
    
    XrVector3f newPos = {2.0f, 1.0f, 0.0f};
    result = spatial_audio_update_source_position(engine, sourceId, &newPos);
    TEST_ASSERT(result == 0, "Audio source position update");
    
    result = spatial_audio_set_source_volume(engine, sourceId, 0.7f);
    TEST_ASSERT(result == 0, "Audio source volume update");
    
    XrVector3f listenerPos = {0.0f, 0.0f, 0.0f};
    XrQuaternionf listenerOri = {1.0f, 0.0f, 0.0f, 0.0f};
    result = spatial_audio_update_listener(engine, &listenerPos, &listenerOri);
    TEST_ASSERT(result == 0, "Listener update");
    
    result = spatial_audio_destroy_source(engine, sourceId);
    TEST_ASSERT(result == 0, "Audio source destruction");
    
    spatial_audio_engine_cleanup(engine);
    spatial_audio_engine_destroy(engine);
    
    TEST_PASS("Spatial Audio tests");
    return 0;
}

// Test VR UI Framework
static int test_vr_ui_framework(void) {
    printf("\n=== Testing VR UI Framework ===\n");
    
    VRUIFramework *framework = vr_ui_framework_create();
    TEST_ASSERT(framework != NULL, "VR UI framework creation");
    
    int result = vr_ui_framework_init(framework);
    TEST_ASSERT(result == 0, "VR UI framework initialization");
    
    XrVector3f panelPos = {0.0f, 1.5f, -2.0f};
    uint32_t panelId = vr_ui_create_panel(framework, &panelPos, 1.0f, 0.8f, UI_MODE_CONTROLLER);
    TEST_ASSERT(panelId > 0, "UI panel creation");
    
    result = vr_ui_show_panel(framework, panelId, true);
    TEST_ASSERT(result == 0, "UI panel visibility");
    
    result = vr_ui_set_locomotion_mode(framework, LOCOMOTION_TELEPORT);
    TEST_ASSERT(result == 0, "Locomotion mode setting");
    
    LocomotionMode mode = vr_ui_get_locomotion_mode(framework);
    TEST_ASSERT(mode == LOCOMOTION_TELEPORT, "Locomotion mode retrieval");
    
    vr_ui_framework_cleanup(framework);
    vr_ui_framework_destroy(framework);
    
    TEST_PASS("VR UI Framework tests");
    return 0;
}

// Test VR Profiler
static int test_vr_profiler(void) {
    printf("\n=== Testing VR Profiler ===\n");
    
    VRProfiler *profiler = vr_profiler_create();
    TEST_ASSERT(profiler != NULL, "VR profiler creation");
    
    int result = vr_profiler_init(profiler);
    TEST_ASSERT(result == 0, "VR profiler initialization");
    
    VRFrameMetrics metrics = {
        .frametime_ms = 11.1f,
        .rendertime_ms = 8.5f,
        .fps = 90.0f,
        .latency_ms = 15.0f,
        .gpu_utilization = 75.0f,
        .cpu_utilization = 50.0f,
        .memory_usage_mb = 2048.0f
    };
    
    result = vr_profiler_record_frame(profiler, &metrics);
    TEST_ASSERT(result == 0, "Frame metrics recording");
    
    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 1);
    TEST_ASSERT(avg.fps == 90.0f, "Average FPS");
    
    bool shouldEnableFoveated = vr_profiler_should_enable_foveated_rendering(profiler);
    TEST_ASSERT(shouldEnableFoveated == false, "Foveated rendering recommendation");
    
    char report[1024];
    result = vr_profiler_generate_report(profiler, report, sizeof(report));
    TEST_ASSERT(result == 0, "Performance report generation");
    
    vr_profiler_cleanup(profiler);
    vr_profiler_destroy(profiler);
    
    TEST_PASS("VR Profiler tests");
    return 0;
}

// Test VR Manager
static int test_vr_manager(void) {
    printf("\n=== Testing VR Manager ===\n");
    
    VRManager *manager = vr_manager_create();
    TEST_ASSERT(manager != NULL, "VR manager creation");
    
    VRConfig config = {
        .platform = VR_PLATFORM_OPENXR,
        .renderWidth = 2048,
        .renderHeight = 2048,
        .renderScale = 1.0f,
        .targetFPS = 90.0f,
        .enableFoveatedRendering = false,
        .enableReprojection = true
    };
    
    int result = vr_manager_init(manager, &config);
    TEST_ASSERT(result == 0, "VR manager initialization");
    
    bool isInitialized = vr_manager_is_initialized(manager);
    TEST_ASSERT(isInitialized == true, "VR manager initialized");
    
    bool isSessionActive = vr_manager_is_session_active(manager);
    TEST_ASSERT(isSessionActive == true, "VR session active");
    
    const char *platformName = vr_manager_get_platform_name(manager);
    TEST_ASSERT(strcmp(platformName, "OpenXR") == 0, "Platform name");
    
    vr_manager_cleanup(manager);
    vr_manager_destroy(manager);
    
    TEST_PASS("VR Manager tests");
    return 0;
}

// Test VR Platforms
static int test_vr_platforms(void) {
    printf("\n=== Testing VR Platforms ===\n");
    
    // Test Meta Quest
    MetaQuestPlatform *quest = meta_quest_platform_create();
    TEST_ASSERT(quest != NULL, "Meta Quest platform creation");
    
    VRPlatformBase *questBase = meta_quest_get_base(quest);
    TEST_ASSERT(questBase != NULL, "Meta Quest base platform");
    
    int result = vr_platform_init(questBase);
    TEST_ASSERT(result == 0, "Meta Quest platform init");
    
    const char *name = vr_platform_get_name(questBase);
    TEST_ASSERT(strcmp(name, "Meta Quest") == 0, "Meta Quest platform name");
    
    VRPlatformCapabilities caps = vr_platform_get_capabilities(questBase);
    TEST_ASSERT(caps.supportsHandTracking == true, "Meta Quest hand tracking");
    TEST_ASSERT(caps.supportsPassthrough == true, "Meta Quest passthrough");
    
    vr_platform_base_destroy(questBase);
    
    // Test SteamVR
    SteamVRPlatform *steamvr = steamvr_platform_create();
    TEST_ASSERT(steamvr != NULL, "SteamVR platform creation");
    
    VRPlatformBase *steamvrBase = steamvr_get_base(steamvr);
    TEST_ASSERT(steamvrBase != NULL, "SteamVR base platform");
    
    name = vr_platform_get_name(steamvrBase);
    TEST_ASSERT(strcmp(name, "SteamVR") == 0, "SteamVR platform name");
    
    vr_platform_base_destroy(steamvrBase);
    
    // Test Apple Vision
    AppleVisionPlatform *vision = apple_vision_platform_create();
    TEST_ASSERT(vision != NULL, "Apple Vision platform creation");
    
    VRPlatformBase *visionBase = apple_vision_get_base(vision);
    TEST_ASSERT(visionBase != NULL, "Apple Vision base platform");
    
    name = vr_platform_get_name(visionBase);
    TEST_ASSERT(strcmp(name, "Apple Vision Pro") == 0, "Apple Vision platform name");
    
    caps = vr_platform_get_capabilities(visionBase);
    TEST_ASSERT(caps.supportsEyeTracking == true, "Apple Vision eye tracking");
    
    vr_platform_base_destroy(visionBase);
    
    TEST_PASS("VR Platforms tests");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("Starting VR System Tests...\n");
    
    failures += test_openxr_manager();
    failures += test_stereoscopic_renderer();
    failures += test_head_tracker();
    failures += test_hand_tracker();
    failures += test_vr_input_system();
    failures += test_spatial_audio();
    failures += test_vr_ui_framework();
    failures += test_vr_profiler();
    failures += test_vr_manager();
    failures += test_vr_platforms();
    
    printf("\n===================\n");
    if (failures == 0) {
        printf("All VR tests passed!\n");
    } else {
        printf("Some VR tests failed: %d\n", failures);
    }
    
    return failures;
}
