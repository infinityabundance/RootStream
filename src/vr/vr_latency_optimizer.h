#ifndef VR_LATENCY_OPTIMIZER_H
#define VR_LATENCY_OPTIMIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Reprojection mode
typedef enum { VR_REPROJ_NONE = 0, VR_REPROJ_MOTION = 1, VR_REPROJ_DEPTH = 2 } VRReprojectionMode;

// Latency metrics snapshot
typedef struct {
    float frametime_ms;
    float prediction_error_us;
    float reproj_latency_us;
    float total_pipeline_latency_us;
    bool meets_90hz_target;
} VRLatencyMetrics;

// Opaque optimizer handle
typedef struct VRLatencyOptimizer VRLatencyOptimizer;

// Lifecycle
VRLatencyOptimizer *vr_latency_optimizer_create(void);
int vr_latency_optimizer_init(VRLatencyOptimizer *optimizer, VRReprojectionMode mode);
void vr_latency_optimizer_cleanup(VRLatencyOptimizer *optimizer);
void vr_latency_optimizer_destroy(VRLatencyOptimizer *optimizer);

// Frame timing
void vr_latency_optimizer_record_frame_start(VRLatencyOptimizer *optimizer);
void vr_latency_optimizer_record_frame_end(VRLatencyOptimizer *optimizer);

// Metrics
VRLatencyMetrics vr_latency_optimizer_get_metrics(VRLatencyOptimizer *optimizer);

// Target FPS control
void vr_latency_optimizer_set_target_fps(VRLatencyOptimizer *optimizer, float fps);

// Reprojection
bool vr_latency_optimizer_is_reprojection_needed(VRLatencyOptimizer *optimizer);
int vr_latency_optimizer_reproject_frame(VRLatencyOptimizer *optimizer, const void *frame,
                                         void *out_frame, const float *pose_delta_4x4);

#ifdef __cplusplus
}
#endif

#endif  // VR_LATENCY_OPTIMIZER_H
