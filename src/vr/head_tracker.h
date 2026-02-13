#ifndef HEAD_TRACKER_H
#define HEAD_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "openxr_manager.h"

// Tracking data structure
typedef struct {
    XrQuaternionf orientation;     // Head rotation
    XrVector3f position;           // Head position (6-DOF)
    XrVector3f linearVelocity;     // Head linear motion
    XrVector3f angularVelocity;    // Head rotation velocity
    uint64_t timestamp_us;
    float confidence;              // Tracking quality 0.0-1.0
} HeadTrackingData;

// Head tracker structure
typedef struct HeadTracker HeadTracker;

// Creation and initialization
HeadTracker* head_tracker_create(void);
int head_tracker_init(HeadTracker *tracker);
void head_tracker_cleanup(HeadTracker *tracker);
void head_tracker_destroy(HeadTracker *tracker);

// Update head pose
int head_tracker_update_pose(HeadTracker *tracker, const XrPosef *xrPose);

// Get head pose (interpolated if needed)
HeadTrackingData head_tracker_get_pose(HeadTracker *tracker, uint64_t timestamp_us);

// Predict future head pose for latency compensation
HeadTrackingData head_tracker_predict_pose(HeadTracker *tracker, uint32_t prediction_ms);

// Get rotation matrix (4x4)
int head_tracker_get_rotation_matrix(HeadTracker *tracker, float matrix[16]);

// Get head-relative directions
XrVector3f head_tracker_get_forward(HeadTracker *tracker);
XrVector3f head_tracker_get_right(HeadTracker *tracker);
XrVector3f head_tracker_get_up(HeadTracker *tracker);

// Get tracking quality
float head_tracker_get_confidence(HeadTracker *tracker);
bool head_tracker_is_active(HeadTracker *tracker);

// Configuration
int head_tracker_set_smoothing(HeadTracker *tracker, float smoothing_factor);
int head_tracker_enable_prediction(HeadTracker *tracker, bool enable);

#ifdef __cplusplus
}
#endif

#endif // HEAD_TRACKER_H
