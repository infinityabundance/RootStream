#include "head_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_HISTORY_SIZE 120  // 2 seconds at 60 FPS

struct HeadTracker {
    HeadTrackingData history[MAX_HISTORY_SIZE];
    uint32_t historySize;
    uint32_t historyIndex;
    
    HeadTrackingData currentPose;
    
    float smoothingFactor;
    bool predictionEnabled;
    bool active;
    bool initialized;
};

// Helper functions for vector and quaternion math
static void vec3_normalize(XrVector3f *v) {
    float len = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (len > 0.0001f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

static void quat_normalize(XrQuaternionf *q) {
    float len = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
    if (len > 0.0001f) {
        q->x /= len;
        q->y /= len;
        q->z /= len;
        q->w /= len;
    }
}

static void quat_to_matrix(const XrQuaternionf *q, float matrix[16]) {
    float xx = q->x * q->x;
    float yy = q->y * q->y;
    float zz = q->z * q->z;
    float xy = q->x * q->y;
    float xz = q->x * q->z;
    float yz = q->y * q->z;
    float wx = q->w * q->x;
    float wy = q->w * q->y;
    float wz = q->w * q->z;
    
    matrix[0] = 1.0f - 2.0f * (yy + zz);
    matrix[1] = 2.0f * (xy + wz);
    matrix[2] = 2.0f * (xz - wy);
    matrix[3] = 0.0f;
    
    matrix[4] = 2.0f * (xy - wz);
    matrix[5] = 1.0f - 2.0f * (xx + zz);
    matrix[6] = 2.0f * (yz + wx);
    matrix[7] = 0.0f;
    
    matrix[8] = 2.0f * (xz + wy);
    matrix[9] = 2.0f * (yz - wx);
    matrix[10] = 1.0f - 2.0f * (xx + yy);
    matrix[11] = 0.0f;
    
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

static XrVector3f quat_rotate_vector(const XrQuaternionf *q, const XrVector3f *v) {
    // Apply quaternion rotation to vector
    XrVector3f u = {q->x, q->y, q->z};
    float s = q->w;
    
    // v' = 2.0 * dot(u, v) * u + (s*s - dot(u, u)) * v + 2.0 * s * cross(u, v)
    float dot_uv = u.x * v->x + u.y * v->y + u.z * v->z;
    float dot_uu = u.x * u.x + u.y * u.y + u.z * u.z;
    
    XrVector3f cross_uv = {
        u.y * v->z - u.z * v->y,
        u.z * v->x - u.x * v->z,
        u.x * v->y - u.y * v->x
    };
    
    XrVector3f result = {
        2.0f * dot_uv * u.x + (s*s - dot_uu) * v->x + 2.0f * s * cross_uv.x,
        2.0f * dot_uv * u.y + (s*s - dot_uu) * v->y + 2.0f * s * cross_uv.y,
        2.0f * dot_uv * u.z + (s*s - dot_uu) * v->z + 2.0f * s * cross_uv.z
    };
    
    return result;
}

HeadTracker* head_tracker_create(void) {
    HeadTracker *tracker = (HeadTracker*)calloc(1, sizeof(HeadTracker));
    if (!tracker) {
        fprintf(stderr, "Failed to allocate HeadTracker\n");
        return NULL;
    }
    
    tracker->historySize = 0;
    tracker->historyIndex = 0;
    tracker->smoothingFactor = 0.3f;  // Default smoothing
    tracker->predictionEnabled = true;
    tracker->active = false;
    tracker->initialized = false;
    
    return tracker;
}

int head_tracker_init(HeadTracker *tracker) {
    if (!tracker) {
        return -1;
    }
    
    // Initialize with identity orientation
    tracker->currentPose.orientation.w = 1.0f;
    tracker->currentPose.orientation.x = 0.0f;
    tracker->currentPose.orientation.y = 0.0f;
    tracker->currentPose.orientation.z = 0.0f;
    
    tracker->currentPose.position.x = 0.0f;
    tracker->currentPose.position.y = 0.0f;
    tracker->currentPose.position.z = 0.0f;
    
    tracker->currentPose.linearVelocity.x = 0.0f;
    tracker->currentPose.linearVelocity.y = 0.0f;
    tracker->currentPose.linearVelocity.z = 0.0f;
    
    tracker->currentPose.angularVelocity.x = 0.0f;
    tracker->currentPose.angularVelocity.y = 0.0f;
    tracker->currentPose.angularVelocity.z = 0.0f;
    
    tracker->currentPose.confidence = 1.0f;
    tracker->currentPose.timestamp_us = 0;
    
    tracker->active = true;
    tracker->initialized = true;
    
    printf("Head tracker initialized\n");
    
    return 0;
}

int head_tracker_update_pose(HeadTracker *tracker, const XrPosef *xrPose) {
    if (!tracker || !tracker->initialized || !xrPose) {
        return -1;
    }
    
    // Update current pose
    tracker->currentPose.orientation = xrPose->orientation;
    tracker->currentPose.position = xrPose->position;
    
    // Normalize quaternion
    quat_normalize(&tracker->currentPose.orientation);
    
    // Update timestamp
    tracker->currentPose.timestamp_us++;  // Would use actual timestamp
    
    // Add to history
    tracker->history[tracker->historyIndex] = tracker->currentPose;
    tracker->historyIndex = (tracker->historyIndex + 1) % MAX_HISTORY_SIZE;
    if (tracker->historySize < MAX_HISTORY_SIZE) {
        tracker->historySize++;
    }
    
    // Calculate velocities if we have history
    if (tracker->historySize >= 2) {
        uint32_t prevIndex = (tracker->historyIndex + MAX_HISTORY_SIZE - 2) % MAX_HISTORY_SIZE;
        HeadTrackingData *prev = &tracker->history[prevIndex];
        
        uint64_t dt_us = tracker->currentPose.timestamp_us - prev->timestamp_us;
        if (dt_us > 0) {
            float dt = (float)dt_us / 1000000.0f;  // Convert to seconds
            
            // Linear velocity
            tracker->currentPose.linearVelocity.x = 
                (tracker->currentPose.position.x - prev->position.x) / dt;
            tracker->currentPose.linearVelocity.y = 
                (tracker->currentPose.position.y - prev->position.y) / dt;
            tracker->currentPose.linearVelocity.z = 
                (tracker->currentPose.position.z - prev->position.z) / dt;
        }
    }
    
    tracker->active = true;
    
    return 0;
}

HeadTrackingData head_tracker_get_pose(HeadTracker *tracker, uint64_t timestamp_us) {
    if (!tracker || !tracker->initialized) {
        HeadTrackingData empty = {0};
        return empty;
    }
    
    // For now, just return current pose
    // In a real implementation, would interpolate based on timestamp
    (void)timestamp_us;
    
    return tracker->currentPose;
}

HeadTrackingData head_tracker_predict_pose(HeadTracker *tracker, uint32_t prediction_ms) {
    if (!tracker || !tracker->initialized || !tracker->predictionEnabled) {
        return tracker->currentPose;
    }
    
    HeadTrackingData predicted = tracker->currentPose;
    
    float dt = (float)prediction_ms / 1000.0f;  // Convert to seconds
    
    // Predict position based on linear velocity
    predicted.position.x += tracker->currentPose.linearVelocity.x * dt;
    predicted.position.y += tracker->currentPose.linearVelocity.y * dt;
    predicted.position.z += tracker->currentPose.linearVelocity.z * dt;
    
    // Predict orientation based on angular velocity
    // This is simplified - real implementation would use quaternion integration
    float angularSpeed = sqrtf(
        tracker->currentPose.angularVelocity.x * tracker->currentPose.angularVelocity.x +
        tracker->currentPose.angularVelocity.y * tracker->currentPose.angularVelocity.y +
        tracker->currentPose.angularVelocity.z * tracker->currentPose.angularVelocity.z
    );
    
    if (angularSpeed > 0.001f) {
        float angle = angularSpeed * dt;
        
        // Create rotation quaternion
        float halfAngle = angle * 0.5f;
        float s = sinf(halfAngle) / angularSpeed;
        
        XrQuaternionf deltaQuat;
        deltaQuat.w = cosf(halfAngle);
        deltaQuat.x = tracker->currentPose.angularVelocity.x * s;
        deltaQuat.y = tracker->currentPose.angularVelocity.y * s;
        deltaQuat.z = tracker->currentPose.angularVelocity.z * s;
        
        // Multiply quaternions: predicted = delta * current
        predicted.orientation.w = deltaQuat.w * tracker->currentPose.orientation.w - 
                                 deltaQuat.x * tracker->currentPose.orientation.x -
                                 deltaQuat.y * tracker->currentPose.orientation.y -
                                 deltaQuat.z * tracker->currentPose.orientation.z;
        predicted.orientation.x = deltaQuat.w * tracker->currentPose.orientation.x +
                                 deltaQuat.x * tracker->currentPose.orientation.w +
                                 deltaQuat.y * tracker->currentPose.orientation.z -
                                 deltaQuat.z * tracker->currentPose.orientation.y;
        predicted.orientation.y = deltaQuat.w * tracker->currentPose.orientation.y -
                                 deltaQuat.x * tracker->currentPose.orientation.z +
                                 deltaQuat.y * tracker->currentPose.orientation.w +
                                 deltaQuat.z * tracker->currentPose.orientation.x;
        predicted.orientation.z = deltaQuat.w * tracker->currentPose.orientation.z +
                                 deltaQuat.x * tracker->currentPose.orientation.y -
                                 deltaQuat.y * tracker->currentPose.orientation.x +
                                 deltaQuat.z * tracker->currentPose.orientation.w;
        
        quat_normalize(&predicted.orientation);
    }
    
    predicted.timestamp_us = tracker->currentPose.timestamp_us + (uint64_t)prediction_ms * 1000;
    
    return predicted;
}

int head_tracker_get_rotation_matrix(HeadTracker *tracker, float matrix[16]) {
    if (!tracker || !tracker->initialized || !matrix) {
        return -1;
    }
    
    quat_to_matrix(&tracker->currentPose.orientation, matrix);
    
    return 0;
}

XrVector3f head_tracker_get_forward(HeadTracker *tracker) {
    XrVector3f forward = {0.0f, 0.0f, -1.0f};  // Default forward is -Z
    
    if (!tracker || !tracker->initialized) {
        return forward;
    }
    
    return quat_rotate_vector(&tracker->currentPose.orientation, &forward);
}

XrVector3f head_tracker_get_right(HeadTracker *tracker) {
    XrVector3f right = {1.0f, 0.0f, 0.0f};  // Default right is +X
    
    if (!tracker || !tracker->initialized) {
        return right;
    }
    
    return quat_rotate_vector(&tracker->currentPose.orientation, &right);
}

XrVector3f head_tracker_get_up(HeadTracker *tracker) {
    XrVector3f up = {0.0f, 1.0f, 0.0f};  // Default up is +Y
    
    if (!tracker || !tracker->initialized) {
        return up;
    }
    
    return quat_rotate_vector(&tracker->currentPose.orientation, &up);
}

float head_tracker_get_confidence(HeadTracker *tracker) {
    if (!tracker || !tracker->initialized) {
        return 0.0f;
    }
    
    return tracker->currentPose.confidence;
}

bool head_tracker_is_active(HeadTracker *tracker) {
    if (!tracker) {
        return false;
    }
    
    return tracker->active;
}

int head_tracker_set_smoothing(HeadTracker *tracker, float smoothing_factor) {
    if (!tracker || smoothing_factor < 0.0f || smoothing_factor > 1.0f) {
        return -1;
    }
    
    tracker->smoothingFactor = smoothing_factor;
    
    return 0;
}

int head_tracker_enable_prediction(HeadTracker *tracker, bool enable) {
    if (!tracker) {
        return -1;
    }
    
    tracker->predictionEnabled = enable;
    
    return 0;
}

void head_tracker_cleanup(HeadTracker *tracker) {
    if (!tracker) {
        return;
    }
    
    tracker->initialized = false;
    tracker->active = false;
    tracker->historySize = 0;
    
    printf("Head tracker cleaned up\n");
}

void head_tracker_destroy(HeadTracker *tracker) {
    if (!tracker) {
        return;
    }
    
    head_tracker_cleanup(tracker);
    free(tracker);
}
