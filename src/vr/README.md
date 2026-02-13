# RootStream VR/XR Support - PHASE 23

## Overview

RootStream now includes comprehensive VR/XR support for immersive gaming experiences across multiple VR headset platforms. The VR subsystem provides stereoscopic 3D rendering, head and hand tracking, spatial audio, VR-specific UI, and platform-specific optimizations.

## Features

### Core VR Infrastructure
- **OpenXR Abstraction Layer** - Cross-platform VR API support
- **Stereoscopic Rendering** - Dual viewport rendering with distortion correction
- **Head Tracking** - 6-DOF tracking with prediction and smoothing
- **Hand Tracking** - Gesture recognition and finger tracking
- **Spatial Audio** - 3D positional audio with HRTF processing

### Platform Support
- **Meta Quest** (Quest 2, Quest 3, Quest Pro)
  - Hand tracking
  - Passthrough camera support
  - Guardian system integration
  - Foveated rendering
- **SteamVR** (Valve Index, HTC Vive, etc.)
  - Chaperone system
  - Full controller support
  - High refresh rate (up to 144Hz)
- **Apple Vision Pro**
  - Eye tracking
  - Spatial computing integration
  - Passthrough by default

### Advanced Features
- **VR Input System** - Controller mapping and haptic feedback
- **VR UI Framework** - World-space UI with multiple interaction modes
- **Performance Profiler** - Real-time performance monitoring and adaptive quality
- **Locomotion Systems** - Teleportation, smooth movement, and dash modes

## Building with VR Support

### Enable VR Support in CMake

```bash
mkdir build && cd build
cmake -DBUILD_VR_SUPPORT=ON ..
make
```

### Run VR Tests

```bash
cd build
ctest -R vr_tests --verbose
```

## Architecture

```
┌────────────────────────────────────────────────────────────┐
│                  RootStream VR Stack                       │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │         VR Platform Abstraction Layer               │  │
│  │  - Meta Quest (OpenXR)                              │  │
│  │  - SteamVR (OpenVR/OpenXR)                          │  │
│  │  - Apple Vision Pro (ARKit/RealityKit)              │  │
│  └────────────────────┬────────────────────────────────┘  │
│                       │                                     │
│  ┌────────────────────▼────────────────────────────────┐  │
│  │      Stereoscopic Rendering Engine                  │  │
│  │  - Dual viewport rendering                          │  │
│  │  - Distortion correction                            │  │
│  │  - Chromatic aberration correction                  │  │
│  └────────────────────┬────────────────────────────────┘  │
│                       │                                     │
│  ┌────────────────────▼────────────────────────────────┐  │
│  │        Head & Hand Tracking                         │  │
│  │  - 6-DOF head pose tracking                         │  │
│  │  - Hand tracking and gesture recognition            │  │
│  │  - Predictive tracking for latency compensation     │  │
│  └────────────────────┬────────────────────────────────┘  │
│                       │                                     │
│  ┌────────────────────▼────────────────────────────────┐  │
│  │       Spatial Audio & VR Input                      │  │
│  │  - 3D spatial sound positioning                     │  │
│  │  - Controller input mapping                         │  │
│  │  - Haptic feedback                                  │  │
│  └────────────────────┬────────────────────────────────┘  │
│                       │                                     │
│  ┌────────────────────▼────────────────────────────────┐  │
│  │      VR UI & Performance                            │  │
│  │  - World-space UI canvases                          │  │
│  │  - Performance profiling                            │  │
│  │  - Adaptive quality settings                        │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

## API Usage

### Initialize VR Manager

```c
#include "vr/vr_manager.h"

// Create and configure VR manager
VRManager *vrManager = vr_manager_create();

VRConfig config = {
    .platform = VR_PLATFORM_OPENXR,
    .renderWidth = 2048,
    .renderHeight = 2048,
    .renderScale = 1.0f,
    .targetFPS = 90.0f,
    .enableFoveatedRendering = false,
    .enableReprojection = true
};

if (vr_manager_init(vrManager, &config) != 0) {
    fprintf(stderr, "Failed to initialize VR manager\n");
    return -1;
}
```

### Render VR Frame

```c
// Begin VR frame
vr_manager_begin_frame(vrManager);

// Prepare video frame
VideoFrame frame = {
    .data = videoData,
    .width = 1920,
    .height = 1080,
    .format = VIDEO_FORMAT_NV12,
    .timestamp = getCurrentTimestamp()
};

// Render stereoscopic frame
vr_manager_render_frame(vrManager, &frame);

// End and submit frame
vr_manager_end_frame(vrManager);
```

### Get Head Tracking Data

```c
HeadTrackingData headPose = vr_manager_get_head_pose(vrManager);

printf("Head position: (%.2f, %.2f, %.2f)\n",
       headPose.position.x,
       headPose.position.y,
       headPose.position.z);

// Predict future pose for latency compensation
HeadTrackingData predicted = head_tracker_predict_pose(headTracker, 16);
```

### Handle VR Input

```c
// Update input state
vr_manager_update_input(vrManager);

// Get controller input
XRInputState input = vr_manager_get_input_state(vrManager);

if (input.rightTrigger > 0.5f) {
    // Handle trigger press
    printf("Right trigger pressed: %.2f\n", input.rightTrigger);
}

// Provide haptic feedback
if (input.buttonA) {
    vr_input_system_vibrate(inputSystem, HAND_RIGHT, 0.7f, 50.0f);
}
```

### Create Spatial Audio

```c
#include "vr/spatial_audio.h"

SpatialAudioEngine *audioEngine = spatial_audio_engine_create();
spatial_audio_engine_init(audioEngine);

// Create audio source at position
XrVector3f sourcePos = {2.0f, 1.0f, 0.0f};
uint32_t sourceId = spatial_audio_create_source(audioEngine, &sourcePos, 10.0f);

// Update listener (head) position
XrVector3f listenerPos = {0.0f, 1.7f, 0.0f};
XrQuaternionf listenerOri = headPose.orientation;
spatial_audio_update_listener(audioEngine, &listenerPos, &listenerOri);

// Process audio with HRTF
spatial_audio_apply_hrtf(audioEngine, sourceId, audioData, dataSize, processedData);
```

### Create VR UI Panel

```c
#include "vr/vr_ui_framework.h"

VRUIFramework *uiFramework = vr_ui_framework_create();
vr_ui_framework_init(uiFramework);

// Create world-space UI panel
XrVector3f panelPos = {0.0f, 1.5f, -2.0f};
uint32_t panelId = vr_ui_create_panel(uiFramework, &panelPos, 
                                      1.2f, 0.8f, UI_MODE_CONTROLLER);

// Or pin panel to head (HUD-style)
vr_ui_pin_panel_to_head(uiFramework, panelId, 2.0f);

// Raycast for UI interaction
XrVector3f rayOrigin, rayDirection;
hand_tracker_get_ray(handTracker, HAND_RIGHT, &rayOrigin, &rayDirection);

uint32_t hitPanelId;
XrVector3f hitPoint;
if (vr_ui_raycast(uiFramework, &rayOrigin, &rayDirection, 
                  &hitPanelId, &hitPoint)) {
    printf("Hit UI panel %u at (%.2f, %.2f, %.2f)\n",
           hitPanelId, hitPoint.x, hitPoint.y, hitPoint.z);
}
```

### Monitor Performance

```c
#include "vr/vr_profiler.h"

VRProfiler *profiler = vr_profiler_create();
vr_profiler_init(profiler);

// Record frame metrics
VRFrameMetrics metrics = vr_manager_get_performance_metrics(vrManager);
vr_profiler_record_frame(profiler, &metrics);

// Get average metrics
VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 60);
printf("Average FPS: %.1f\n", avg.fps);
printf("Average latency: %.1f ms\n", avg.latency_ms);

// Check if we should enable optimizations
if (vr_profiler_should_enable_foveated_rendering(profiler)) {
    vr_manager_enable_foveated_rendering(vrManager, true);
}

// Adaptive quality adjustment
float recommendedScale;
vr_profiler_adjust_quality(profiler, 90.0f, &recommendedScale);
if (recommendedScale != 1.0f) {
    vr_manager_set_render_scale(vrManager, recommendedScale);
}
```

## File Structure

```
src/vr/
├── openxr_manager.h/c           # OpenXR abstraction
├── stereoscopic_renderer.h/c    # Stereo rendering
├── head_tracker.h/c             # Head tracking
├── hand_tracker.h/c             # Hand tracking
├── vr_input_system.h/c          # Input handling
├── spatial_audio.h/c            # Spatial audio
├── vr_ui_framework.h/c          # VR UI
├── vr_profiler.h/c              # Performance profiling
├── platforms/
│   ├── vr_platform_base.h/c     # Base platform class
│   ├── meta_quest.h/c           # Meta Quest support
│   ├── steamvr.h/c              # SteamVR support
│   └── apple_vision.h/c         # Apple Vision Pro support
└── vr_manager.h/c               # Main coordinator

tests/unit/
└── test_vr.c                    # Unit tests for VR components
```

## Dependencies

The VR subsystem is designed with minimal external dependencies:

- **OpenXR SDK** (optional) - For full OpenXR support
- **Math library** (libm) - For vector/quaternion operations
- **Standard C library** - No additional dependencies required

The current implementation provides stub/mock functionality for testing and can be extended with actual OpenXR integration when needed.

## Performance Targets

- **Target FPS**: 90 Hz (11.1ms per frame)
- **Maximum Latency**: <20ms (motion-to-photon)
- **Tracking Prediction**: 16-20ms ahead
- **Recommended Resolution**: 2048x2048 per eye (adjustable)

## Platform-Specific Features

### Meta Quest
- **Resolution**: 1832x1920 per eye (Quest 3)
- **Refresh Rate**: Up to 120Hz
- **Features**: Hand tracking, passthrough, Guardian system
- **Optimizations**: Foveated rendering, dynamic resolution

### SteamVR (Valve Index)
- **Resolution**: 2016x2240 per eye
- **Refresh Rate**: Up to 144Hz
- **Features**: Finger tracking, eye tracking (optional)
- **Optimizations**: Motion smoothing, advanced reprojection

### Apple Vision Pro
- **Resolution**: 3680x3140 per eye
- **Refresh Rate**: 90Hz
- **Features**: Eye tracking, hand tracking, spatial computing
- **Optimizations**: Foveated rendering, spatial audio

## Testing

Run VR unit tests:

```bash
cd build
./test_vr
```

All VR components have comprehensive unit tests covering:
- OpenXR manager initialization and session management
- Stereoscopic rendering and distortion correction
- Head tracking with prediction and smoothing
- Hand tracking and gesture recognition
- VR input system and haptic feedback
- Spatial audio engine
- VR UI framework
- Performance profiling
- Platform-specific implementations

## Future Enhancements

- Full OpenXR SDK integration (currently stub implementation)
- Eye tracking support for foveated rendering
- Advanced gesture recognition
- Multiplayer VR synchronization
- VR recording and replay
- Controller vibration patterns
- Advanced spatial audio (Ambisonics, room acoustics)

## License

Same as RootStream main project - see LICENSE file.
