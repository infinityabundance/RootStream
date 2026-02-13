#ifndef VR_INPUT_SYSTEM_H
#define VR_INPUT_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "openxr_manager.h"
#include "hand_tracker.h"

// Controller input structure
typedef struct {
    // Buttons
    bool buttonA, buttonB, buttonX, buttonY;
    bool buttonGrip, buttonMenu;
    bool triggerPressed, gripPressed;
    
    // Analog inputs
    float triggerValue;      // 0.0 - 1.0
    float gripValue;         // 0.0 - 1.0
    XrVector2f thumbstick;   // -1.0 - 1.0
    XrVector2f touchpad;     // 0.0 - 1.0
    
    // Pose
    XrVector3f position;
    XrQuaternionf orientation;
} ControllerInput;

// VR Input System structure
typedef struct VRInputSystem VRInputSystem;

// Creation and initialization
VRInputSystem* vr_input_system_create(void);
int vr_input_system_init(VRInputSystem *system);
void vr_input_system_cleanup(VRInputSystem *system);
void vr_input_system_destroy(VRInputSystem *system);

// Update input state
int vr_input_system_update(VRInputSystem *system, const XRInputState *xrInput);

// Get controller input
ControllerInput vr_input_system_get_controller(VRInputSystem *system, Hand hand);

// Haptic feedback
int vr_input_system_vibrate(VRInputSystem *system, Hand hand, 
                            float intensity, float duration_ms);

// Haptic pulse
int vr_input_system_pulse(VRInputSystem *system, Hand hand, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // VR_INPUT_SYSTEM_H
