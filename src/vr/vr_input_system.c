#include "vr_input_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct VRInputSystem {
    ControllerInput leftController;
    ControllerInput rightController;
    bool initialized;
};

VRInputSystem* vr_input_system_create(void) {
    VRInputSystem *system = (VRInputSystem*)calloc(1, sizeof(VRInputSystem));
    if (!system) {
        fprintf(stderr, "Failed to allocate VRInputSystem\n");
        return NULL;
    }
    
    system->initialized = false;
    
    return system;
}

int vr_input_system_init(VRInputSystem *system) {
    if (!system) {
        return -1;
    }
    
    memset(&system->leftController, 0, sizeof(ControllerInput));
    memset(&system->rightController, 0, sizeof(ControllerInput));
    
    system->leftController.orientation.w = 1.0f;
    system->rightController.orientation.w = 1.0f;
    
    system->initialized = true;
    
    printf("VR input system initialized\n");
    
    return 0;
}

int vr_input_system_update(VRInputSystem *system, const XRInputState *xrInput) {
    if (!system || !system->initialized || !xrInput) {
        return -1;
    }
    
    // Update left controller
    system->leftController.triggerValue = xrInput->leftTrigger;
    system->leftController.triggerPressed = (xrInput->leftTrigger > 0.5f);
    system->leftController.thumbstick = xrInput->leftThumbstick;
    system->leftController.position = xrInput->leftControllerPose.position;
    system->leftController.orientation = xrInput->leftControllerPose.orientation;
    
    // Update right controller
    system->rightController.triggerValue = xrInput->rightTrigger;
    system->rightController.triggerPressed = (xrInput->rightTrigger > 0.5f);
    system->rightController.thumbstick = xrInput->rightThumbstick;
    system->rightController.position = xrInput->rightControllerPose.position;
    system->rightController.orientation = xrInput->rightControllerPose.orientation;
    
    // Update buttons
    system->leftController.buttonX = xrInput->buttonX;
    system->leftController.buttonY = xrInput->buttonY;
    system->rightController.buttonA = xrInput->buttonA;
    system->rightController.buttonB = xrInput->buttonB;
    system->leftController.buttonMenu = xrInput->buttonMenu;
    system->rightController.buttonMenu = xrInput->buttonMenu;
    
    return 0;
}

ControllerInput vr_input_system_get_controller(VRInputSystem *system, Hand hand) {
    if (!system || !system->initialized) {
        ControllerInput empty = {0};
        return empty;
    }
    
    return (hand == HAND_LEFT) ? system->leftController : system->rightController;
}

int vr_input_system_vibrate(VRInputSystem *system, Hand hand, 
                            float intensity, float duration_ms) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    // In a real implementation, would send haptic command to OpenXR
    printf("Vibrate %s controller: intensity=%.2f, duration=%.1fms\n",
           (hand == HAND_LEFT) ? "left" : "right", intensity, duration_ms);
    
    return 0;
}

int vr_input_system_pulse(VRInputSystem *system, Hand hand, uint32_t duration_ms) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    // Send a simple pulse at medium intensity
    return vr_input_system_vibrate(system, hand, 0.5f, (float)duration_ms);
}

void vr_input_system_cleanup(VRInputSystem *system) {
    if (!system) {
        return;
    }
    
    system->initialized = false;
    
    printf("VR input system cleaned up\n");
}

void vr_input_system_destroy(VRInputSystem *system) {
    if (!system) {
        return;
    }
    
    vr_input_system_cleanup(system);
    free(system);
}
