#include "hand_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct HandTracker {
    HandState leftHand;
    HandState rightHand;
    bool initialized;
};

HandTracker* hand_tracker_create(void) {
    HandTracker *tracker = (HandTracker*)calloc(1, sizeof(HandTracker));
    if (!tracker) {
        fprintf(stderr, "Failed to allocate HandTracker\n");
        return NULL;
    }
    
    tracker->initialized = false;
    
    return tracker;
}

int hand_tracker_init(HandTracker *tracker) {
    if (!tracker) {
        return -1;
    }
    
    memset(&tracker->leftHand, 0, sizeof(HandState));
    memset(&tracker->rightHand, 0, sizeof(HandState));
    
    tracker->leftHand.palmOrientation.w = 1.0f;
    tracker->rightHand.palmOrientation.w = 1.0f;
    
    tracker->initialized = true;
    
    printf("Hand tracker initialized\n");
    
    return 0;
}

int hand_tracker_update(HandTracker *tracker, Hand hand, const XrPosef *palmPose) {
    if (!tracker || !tracker->initialized || !palmPose) {
        return -1;
    }
    
    HandState *state = (hand == HAND_LEFT) ? &tracker->leftHand : &tracker->rightHand;
    
    state->palmPosition = palmPose->position;
    state->palmOrientation = palmPose->orientation;
    state->isTracked = true;
    
    // In a real implementation, would update all finger joints from OpenXR
    
    return 0;
}

HandState hand_tracker_get_state(HandTracker *tracker, Hand hand) {
    if (!tracker || !tracker->initialized) {
        HandState empty = {0};
        return empty;
    }
    
    return (hand == HAND_LEFT) ? tracker->leftHand : tracker->rightHand;
}

Gesture hand_tracker_detect_gesture(HandTracker *tracker, Hand hand) {
    if (!tracker || !tracker->initialized) {
        return GESTURE_NONE;
    }
    
    HandState *state = (hand == HAND_LEFT) ? &tracker->leftHand : &tracker->rightHand;
    
    // Simplified gesture detection
    // In a real implementation, would analyze finger joint positions
    
    return state->detectedGesture;
}

XrVector3f hand_tracker_get_finger_tip(HandTracker *tracker, Hand hand, uint32_t fingerIndex) {
    XrVector3f zero = {0};
    
    if (!tracker || !tracker->initialized || fingerIndex >= 5) {
        return zero;
    }
    
    HandState *state = (hand == HAND_LEFT) ? &tracker->leftHand : &tracker->rightHand;
    
    // Return the tip joint (joint 4 of each finger)
    return state->fingerPositions[fingerIndex * 5 + 4];
}

bool hand_tracker_is_tracked(HandTracker *tracker, Hand hand) {
    if (!tracker || !tracker->initialized) {
        return false;
    }
    
    HandState *state = (hand == HAND_LEFT) ? &tracker->leftHand : &tracker->rightHand;
    
    return state->isTracked;
}

int hand_tracker_get_ray(HandTracker *tracker, Hand hand, XrVector3f *origin, XrVector3f *direction) {
    if (!tracker || !tracker->initialized || !origin || !direction) {
        return -1;
    }
    
    HandState *state = (hand == HAND_LEFT) ? &tracker->leftHand : &tracker->rightHand;
    
    *origin = state->palmPosition;
    
    // Ray direction from palm orientation (forward is -Z rotated by orientation)
    direction->x = 0.0f;
    direction->y = 0.0f;
    direction->z = -1.0f;
    
    // In a real implementation, would rotate by palm orientation
    
    return 0;
}

void hand_tracker_cleanup(HandTracker *tracker) {
    if (!tracker) {
        return;
    }
    
    tracker->initialized = false;
    
    printf("Hand tracker cleaned up\n");
}

void hand_tracker_destroy(HandTracker *tracker) {
    if (!tracker) {
        return;
    }
    
    hand_tracker_cleanup(tracker);
    free(tracker);
}
