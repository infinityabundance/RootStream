#ifndef VR_MANAGER_H
#define VR_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "openxr_manager.h"
#include "stereoscopic_renderer.h"
#include "head_tracker.h"

// VR Platform enumeration
typedef enum {
    VR_PLATFORM_OPENXR = 0,
    VR_PLATFORM_META_QUEST = 1,
    VR_PLATFORM_STEAMVR = 2,
    VR_PLATFORM_APPLE_VISION = 3
} VRPlatform;

// VR configuration
typedef struct {
    VRPlatform platform;
    uint32_t renderWidth;
    uint32_t renderHeight;
    float renderScale;
    bool enableFoveatedRendering;
    bool enableReprojection;
    float targetFPS;
} VRConfig;

// VR Manager structure
typedef struct VRManager VRManager;

// Creation and initialization
VRManager* vr_manager_create(void);
int vr_manager_init(VRManager *manager, const VRConfig *config);
void vr_manager_cleanup(VRManager *manager);
void vr_manager_destroy(VRManager *manager);

// Frame rendering
int vr_manager_begin_frame(VRManager *manager);
int vr_manager_render_frame(VRManager *manager, const VideoFrame *frame);
int vr_manager_end_frame(VRManager *manager);

// Input handling
int vr_manager_update_input(VRManager *manager);
XRInputState vr_manager_get_input_state(VRManager *manager);

// Head tracking
HeadTrackingData vr_manager_get_head_pose(VRManager *manager);
int vr_manager_get_view_matrices(VRManager *manager, float leftView[16], float rightView[16]);
int vr_manager_get_projection_matrices(VRManager *manager, float leftProj[16], float rightProj[16]);

// Performance
typedef struct {
    float frametime_ms;
    float rendertime_ms;
    float fps;
    float latency_ms;
    bool droppedFrame;
} VRPerformanceMetrics;

VRPerformanceMetrics vr_manager_get_performance_metrics(VRManager *manager);

// Status
bool vr_manager_is_initialized(VRManager *manager);
bool vr_manager_is_session_active(VRManager *manager);
const char* vr_manager_get_platform_name(VRManager *manager);

// Configuration
int vr_manager_set_render_scale(VRManager *manager, float scale);
int vr_manager_enable_foveated_rendering(VRManager *manager, bool enable);

#ifdef __cplusplus
}
#endif

#endif // VR_MANAGER_H
