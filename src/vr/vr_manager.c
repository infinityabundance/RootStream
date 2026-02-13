#include "vr_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

struct VRManager {
    VRConfig config;
    
    OpenXRManager *openxr;
    StereoscopicRenderer *stereoRenderer;
    HeadTracker *headTracker;
    
    bool initialized;
    bool sessionActive;
    
    VRPerformanceMetrics metrics;
    uint64_t frameStartTime;
    uint64_t frameCount;
};

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

VRManager* vr_manager_create(void) {
    VRManager *manager = (VRManager*)calloc(1, sizeof(VRManager));
    if (!manager) {
        fprintf(stderr, "Failed to allocate VRManager\n");
        return NULL;
    }
    
    manager->initialized = false;
    manager->sessionActive = false;
    manager->frameCount = 0;
    
    return manager;
}

int vr_manager_init(VRManager *manager, const VRConfig *config) {
    if (!manager || !config) {
        return -1;
    }
    
    // Store configuration
    manager->config = *config;
    
    // Set defaults if not specified
    if (manager->config.renderWidth == 0) {
        manager->config.renderWidth = 2048;
    }
    if (manager->config.renderHeight == 0) {
        manager->config.renderHeight = 2048;
    }
    if (manager->config.renderScale == 0.0f) {
        manager->config.renderScale = 1.0f;
    }
    if (manager->config.targetFPS == 0.0f) {
        manager->config.targetFPS = 90.0f;
    }
    
    // Initialize OpenXR
    manager->openxr = openxr_manager_create();
    if (!manager->openxr) {
        fprintf(stderr, "Failed to create OpenXR manager\n");
        goto error;
    }
    
    if (openxr_manager_init(manager->openxr) < 0) {
        fprintf(stderr, "Failed to initialize OpenXR\n");
        goto error;
    }
    
    // Get recommended resolution from OpenXR
    uint32_t recommendedWidth, recommendedHeight;
    if (openxr_manager_get_recommended_resolution(manager->openxr, 
                                                  &recommendedWidth, 
                                                  &recommendedHeight) == 0) {
        manager->config.renderWidth = (uint32_t)(recommendedWidth * manager->config.renderScale);
        manager->config.renderHeight = (uint32_t)(recommendedHeight * manager->config.renderScale);
    }
    
    // Create session
    if (openxr_manager_create_session(manager->openxr) < 0) {
        fprintf(stderr, "Failed to create OpenXR session\n");
        goto error;
    }
    
    // Initialize stereoscopic renderer
    manager->stereoRenderer = stereoscopic_renderer_create();
    if (!manager->stereoRenderer) {
        fprintf(stderr, "Failed to create stereoscopic renderer\n");
        goto error;
    }
    
    if (stereoscopic_renderer_init(manager->stereoRenderer, 
                                  manager->config.renderWidth,
                                  manager->config.renderHeight) < 0) {
        fprintf(stderr, "Failed to initialize stereoscopic renderer\n");
        goto error;
    }
    
    // Initialize head tracker
    manager->headTracker = head_tracker_create();
    if (!manager->headTracker) {
        fprintf(stderr, "Failed to create head tracker\n");
        goto error;
    }
    
    if (head_tracker_init(manager->headTracker) < 0) {
        fprintf(stderr, "Failed to initialize head tracker\n");
        goto error;
    }
    
    manager->initialized = true;
    manager->sessionActive = true;
    
    printf("VR Manager initialized successfully\n");
    printf("  Platform: %s\n", vr_manager_get_platform_name(manager));
    printf("  Render resolution: %dx%d per eye\n", 
           manager->config.renderWidth, manager->config.renderHeight);
    printf("  Target FPS: %.0f\n", manager->config.targetFPS);
    
    return 0;
    
error:
    if (manager->headTracker) {
        head_tracker_destroy(manager->headTracker);
        manager->headTracker = NULL;
    }
    if (manager->stereoRenderer) {
        stereoscopic_renderer_destroy(manager->stereoRenderer);
        manager->stereoRenderer = NULL;
    }
    if (manager->openxr) {
        openxr_manager_destroy(manager->openxr);
        manager->openxr = NULL;
    }
    
    return -1;
}

int vr_manager_begin_frame(VRManager *manager) {
    if (!manager || !manager->initialized || !manager->sessionActive) {
        return -1;
    }
    
    manager->frameStartTime = get_time_us();
    
    // Begin OpenXR frame
    if (openxr_manager_begin_frame(manager->openxr) < 0) {
        return -1;
    }
    
    // Update head tracking
    XRState xrState = openxr_manager_get_tracking_data(manager->openxr);
    head_tracker_update_pose(manager->headTracker, &xrState.headPose);
    
    return 0;
}

int vr_manager_render_frame(VRManager *manager, const VideoFrame *frame) {
    if (!manager || !manager->initialized || !manager->sessionActive || !frame) {
        return -1;
    }
    
    uint64_t renderStart = get_time_us();
    
    // Get view and projection matrices
    float leftView[16], rightView[16];
    float leftProj[16], rightProj[16];
    
    openxr_manager_get_eye_view(manager->openxr, XR_EYE_LEFT, leftView);
    openxr_manager_get_eye_view(manager->openxr, XR_EYE_RIGHT, rightView);
    openxr_manager_get_eye_projection(manager->openxr, XR_EYE_LEFT, leftProj);
    openxr_manager_get_eye_projection(manager->openxr, XR_EYE_RIGHT, rightProj);
    
    // Render left eye
    if (stereoscopic_renderer_render_left_eye(manager->stereoRenderer, 
                                             frame, leftProj, leftView) < 0) {
        fprintf(stderr, "Failed to render left eye\n");
        return -1;
    }
    
    // Render right eye
    if (stereoscopic_renderer_render_right_eye(manager->stereoRenderer,
                                              frame, rightProj, rightView) < 0) {
        fprintf(stderr, "Failed to render right eye\n");
        return -1;
    }
    
    uint64_t renderEnd = get_time_us();
    manager->metrics.rendertime_ms = (float)(renderEnd - renderStart) / 1000.0f;
    
    return 0;
}

int vr_manager_end_frame(VRManager *manager) {
    if (!manager || !manager->initialized || !manager->sessionActive) {
        return -1;
    }
    
    // End OpenXR frame
    if (openxr_manager_end_frame(manager->openxr) < 0) {
        return -1;
    }
    
    // Update performance metrics
    uint64_t frameEnd = get_time_us();
    manager->metrics.frametime_ms = (float)(frameEnd - manager->frameStartTime) / 1000.0f;
    manager->metrics.fps = 1000.0f / manager->metrics.frametime_ms;
    manager->metrics.droppedFrame = (manager->metrics.fps < manager->config.targetFPS * 0.9f);
    
    manager->frameCount++;
    
    return 0;
}

int vr_manager_update_input(VRManager *manager) {
    if (!manager || !manager->initialized) {
        return -1;
    }
    
    // Input is handled automatically by OpenXR manager
    // This is a placeholder for any additional processing
    
    return 0;
}

XRInputState vr_manager_get_input_state(VRManager *manager) {
    if (!manager || !manager->initialized) {
        XRInputState empty = {0};
        return empty;
    }
    
    return openxr_manager_get_input(manager->openxr);
}

HeadTrackingData vr_manager_get_head_pose(VRManager *manager) {
    if (!manager || !manager->initialized) {
        HeadTrackingData empty = {0};
        return empty;
    }
    
    return head_tracker_get_pose(manager->headTracker, 0);
}

int vr_manager_get_view_matrices(VRManager *manager, float leftView[16], float rightView[16]) {
    if (!manager || !manager->initialized || !leftView || !rightView) {
        return -1;
    }
    
    openxr_manager_get_eye_view(manager->openxr, XR_EYE_LEFT, leftView);
    openxr_manager_get_eye_view(manager->openxr, XR_EYE_RIGHT, rightView);
    
    return 0;
}

int vr_manager_get_projection_matrices(VRManager *manager, float leftProj[16], float rightProj[16]) {
    if (!manager || !manager->initialized || !leftProj || !rightProj) {
        return -1;
    }
    
    openxr_manager_get_eye_projection(manager->openxr, XR_EYE_LEFT, leftProj);
    openxr_manager_get_eye_projection(manager->openxr, XR_EYE_RIGHT, rightProj);
    
    return 0;
}

VRPerformanceMetrics vr_manager_get_performance_metrics(VRManager *manager) {
    if (!manager || !manager->initialized) {
        VRPerformanceMetrics empty = {0};
        return empty;
    }
    
    return manager->metrics;
}

bool vr_manager_is_initialized(VRManager *manager) {
    if (!manager) {
        return false;
    }
    
    return manager->initialized;
}

bool vr_manager_is_session_active(VRManager *manager) {
    if (!manager) {
        return false;
    }
    
    return manager->sessionActive;
}

const char* vr_manager_get_platform_name(VRManager *manager) {
    if (!manager) {
        return "Unknown";
    }
    
    switch (manager->config.platform) {
        case VR_PLATFORM_OPENXR:
            return "OpenXR";
        case VR_PLATFORM_META_QUEST:
            return "Meta Quest";
        case VR_PLATFORM_STEAMVR:
            return "SteamVR";
        case VR_PLATFORM_APPLE_VISION:
            return "Apple Vision Pro";
        default:
            return "Unknown";
    }
}

int vr_manager_set_render_scale(VRManager *manager, float scale) {
    if (!manager || !manager->initialized || scale <= 0.0f || scale > 2.0f) {
        return -1;
    }
    
    manager->config.renderScale = scale;
    
    uint32_t newWidth = (uint32_t)(2048 * scale);
    uint32_t newHeight = (uint32_t)(2048 * scale);
    
    return stereoscopic_renderer_resize(manager->stereoRenderer, newWidth, newHeight);
}

int vr_manager_enable_foveated_rendering(VRManager *manager, bool enable) {
    if (!manager || !manager->initialized) {
        return -1;
    }
    
    manager->config.enableFoveatedRendering = enable;
    
    printf("Foveated rendering %s\n", enable ? "enabled" : "disabled");
    
    return 0;
}

void vr_manager_cleanup(VRManager *manager) {
    if (!manager) {
        return;
    }
    
    if (manager->headTracker) {
        head_tracker_destroy(manager->headTracker);
        manager->headTracker = NULL;
    }
    
    if (manager->stereoRenderer) {
        stereoscopic_renderer_destroy(manager->stereoRenderer);
        manager->stereoRenderer = NULL;
    }
    
    if (manager->openxr) {
        openxr_manager_destroy(manager->openxr);
        manager->openxr = NULL;
    }
    
    manager->initialized = false;
    manager->sessionActive = false;
    
    printf("VR Manager cleaned up\n");
}

void vr_manager_destroy(VRManager *manager) {
    if (!manager) {
        return;
    }
    
    vr_manager_cleanup(manager);
    free(manager);
}
