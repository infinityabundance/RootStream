#ifndef VR_UI_FRAMEWORK_H
#define VR_UI_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "openxr_manager.h"

// UI interaction mode
typedef enum {
    UI_MODE_GAZE,        // Gaze-based interaction
    UI_MODE_CONTROLLER,  // Controller ray-casting
    UI_MODE_HAND,        // Hand tracking
    UI_MODE_HYBRID       // Multiple modes
} UIMode;

// Locomotion mode
typedef enum {
    LOCOMOTION_TELEPORT,  // Instant teleportation
    LOCOMOTION_SMOOTH,    // Smooth movement
    LOCOMOTION_DASH       // Fast dash movement
} LocomotionMode;

// UI panel structure
typedef struct {
    uint32_t panelId;
    XrVector3f position;
    XrQuaternionf rotation;
    float width;
    float height;
    UIMode interactionMode;
    bool pinned;  // Pinned to head or world
    bool visible;
} UIPanel;

// VR UI Framework structure
typedef struct VRUIFramework VRUIFramework;

// Creation and initialization
VRUIFramework* vr_ui_framework_create(void);
int vr_ui_framework_init(VRUIFramework *framework);
void vr_ui_framework_cleanup(VRUIFramework *framework);
void vr_ui_framework_destroy(VRUIFramework *framework);

// UI panel management
uint32_t vr_ui_create_panel(VRUIFramework *framework, const XrVector3f *position,
                            float width, float height, UIMode mode);
int vr_ui_pin_panel_to_head(VRUIFramework *framework, uint32_t panelId, float distance);
int vr_ui_set_panel_world_position(VRUIFramework *framework, uint32_t panelId, 
                                   const XrVector3f *position);
int vr_ui_show_panel(VRUIFramework *framework, uint32_t panelId, bool visible);

// Raycasting for UI interaction
bool vr_ui_raycast(VRUIFramework *framework, const XrVector3f *rayOrigin,
                  const XrVector3f *rayDirection, uint32_t *hitPanelId,
                  XrVector3f *hitPoint);

// Gaze interaction
int vr_ui_update_gaze(VRUIFramework *framework, const XrVector3f *gazeOrigin,
                     const XrVector3f *gazeDirection);

// Teleportation
int vr_ui_init_teleportation(VRUIFramework *framework);
int vr_ui_update_teleport_target(VRUIFramework *framework, const XrVector3f *target);
int vr_ui_execute_teleport(VRUIFramework *framework, XrVector3f *newPosition);

// Locomotion mode
int vr_ui_set_locomotion_mode(VRUIFramework *framework, LocomotionMode mode);
LocomotionMode vr_ui_get_locomotion_mode(VRUIFramework *framework);

#ifdef __cplusplus
}
#endif

#endif // VR_UI_FRAMEWORK_H
