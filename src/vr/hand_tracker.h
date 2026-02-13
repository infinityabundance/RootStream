#ifndef HAND_TRACKER_H
#define HAND_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "openxr_manager.h"

// Hand enumeration
typedef enum {
    HAND_LEFT = 0,
    HAND_RIGHT = 1
} Hand;

// Gesture enumeration
typedef enum {
    GESTURE_NONE = 0,
    GESTURE_OPEN_PALM,
    GESTURE_CLOSED_FIST,
    GESTURE_POINTING,
    GESTURE_THUMBS_UP,
    GESTURE_PEACE,
    GESTURE_OK,
    GESTURE_PINCH
} Gesture;

// Hand state
typedef struct {
    XrVector3f palmPosition;
    XrQuaternionf palmOrientation;
    XrVector3f fingerPositions[25];  // 25 joints (5 fingers x 5 joints each)
    float fingerConfidence[25];
    Gesture detectedGesture;
    float gestureConfidence;
    bool isTracked;
} HandState;

// Hand tracker structure
typedef struct HandTracker HandTracker;

// Creation and initialization
HandTracker* hand_tracker_create(void);
int hand_tracker_init(HandTracker *tracker);
void hand_tracker_cleanup(HandTracker *tracker);
void hand_tracker_destroy(HandTracker *tracker);

// Update hand tracking
int hand_tracker_update(HandTracker *tracker, Hand hand, const XrPosef *palmPose);

// Get hand state
HandState hand_tracker_get_state(HandTracker *tracker, Hand hand);

// Gesture detection
Gesture hand_tracker_detect_gesture(HandTracker *tracker, Hand hand);

// Finger tracking
XrVector3f hand_tracker_get_finger_tip(HandTracker *tracker, Hand hand, uint32_t fingerIndex);

// Hand presence
bool hand_tracker_is_tracked(HandTracker *tracker, Hand hand);

// Ray casting from hand
int hand_tracker_get_ray(HandTracker *tracker, Hand hand, XrVector3f *origin, XrVector3f *direction);

#ifdef __cplusplus
}
#endif

#endif // HAND_TRACKER_H
