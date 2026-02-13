#include "vr_ui_framework.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_UI_PANELS 32

struct VRUIFramework {
    UIPanel panels[MAX_UI_PANELS];
    uint32_t panelCount;
    uint32_t nextPanelId;
    
    XrVector3f gazeOrigin;
    XrVector3f gazeDirection;
    
    XrVector3f teleportTarget;
    bool teleportActive;
    
    LocomotionMode locomotionMode;
    
    bool initialized;
};

VRUIFramework* vr_ui_framework_create(void) {
    VRUIFramework *framework = (VRUIFramework*)calloc(1, sizeof(VRUIFramework));
    if (!framework) {
        fprintf(stderr, "Failed to allocate VRUIFramework\n");
        return NULL;
    }
    
    framework->initialized = false;
    framework->panelCount = 0;
    framework->nextPanelId = 1;
    framework->teleportActive = false;
    framework->locomotionMode = LOCOMOTION_TELEPORT;
    
    return framework;
}

int vr_ui_framework_init(VRUIFramework *framework) {
    if (!framework) {
        return -1;
    }
    
    memset(framework->panels, 0, sizeof(framework->panels));
    
    framework->initialized = true;
    
    printf("VR UI framework initialized\n");
    
    return 0;
}

uint32_t vr_ui_create_panel(VRUIFramework *framework, const XrVector3f *position,
                            float width, float height, UIMode mode) {
    if (!framework || !framework->initialized || framework->panelCount >= MAX_UI_PANELS) {
        return 0;
    }
    
    uint32_t panelId = framework->nextPanelId++;
    
    for (uint32_t i = 0; i < MAX_UI_PANELS; i++) {
        if (framework->panels[i].panelId == 0) {
            framework->panels[i].panelId = panelId;
            framework->panels[i].position = *position;
            framework->panels[i].rotation.w = 1.0f;
            framework->panels[i].rotation.x = 0.0f;
            framework->panels[i].rotation.y = 0.0f;
            framework->panels[i].rotation.z = 0.0f;
            framework->panels[i].width = width;
            framework->panels[i].height = height;
            framework->panels[i].interactionMode = mode;
            framework->panels[i].pinned = false;
            framework->panels[i].visible = true;
            framework->panelCount++;
            
            return panelId;
        }
    }
    
    return 0;
}

int vr_ui_pin_panel_to_head(VRUIFramework *framework, uint32_t panelId, float distance) {
    if (!framework || !framework->initialized) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_UI_PANELS; i++) {
        if (framework->panels[i].panelId == panelId) {
            framework->panels[i].pinned = true;
            framework->panels[i].position.z = -distance;
            return 0;
        }
    }
    
    return -1;
}

int vr_ui_set_panel_world_position(VRUIFramework *framework, uint32_t panelId, 
                                   const XrVector3f *position) {
    if (!framework || !framework->initialized || !position) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_UI_PANELS; i++) {
        if (framework->panels[i].panelId == panelId) {
            framework->panels[i].position = *position;
            framework->panels[i].pinned = false;
            return 0;
        }
    }
    
    return -1;
}

int vr_ui_show_panel(VRUIFramework *framework, uint32_t panelId, bool visible) {
    if (!framework || !framework->initialized) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_UI_PANELS; i++) {
        if (framework->panels[i].panelId == panelId) {
            framework->panels[i].visible = visible;
            return 0;
        }
    }
    
    return -1;
}

// Helper function to check ray-plane intersection
static bool ray_plane_intersect(const XrVector3f *rayOrigin, const XrVector3f *rayDir,
                                const XrVector3f *planePos, const XrVector3f *planeNormal,
                                float planeWidth, float planeHeight, XrVector3f *hitPoint) {
    // Calculate ray-plane intersection
    float denom = planeNormal->x * rayDir->x + 
                  planeNormal->y * rayDir->y + 
                  planeNormal->z * rayDir->z;
    
    if (fabsf(denom) < 0.0001f) {
        return false;  // Ray parallel to plane
    }
    
    XrVector3f diff = {
        planePos->x - rayOrigin->x,
        planePos->y - rayOrigin->y,
        planePos->z - rayOrigin->z
    };
    
    float t = (diff.x * planeNormal->x + 
               diff.y * planeNormal->y + 
               diff.z * planeNormal->z) / denom;
    
    if (t < 0.0f) {
        return false;  // Intersection behind ray origin
    }
    
    // Calculate intersection point
    hitPoint->x = rayOrigin->x + rayDir->x * t;
    hitPoint->y = rayOrigin->y + rayDir->y * t;
    hitPoint->z = rayOrigin->z + rayDir->z * t;
    
    // Check if hit point is within panel bounds
    XrVector3f localHit = {
        hitPoint->x - planePos->x,
        hitPoint->y - planePos->y,
        hitPoint->z - planePos->z
    };
    
    return (fabsf(localHit.x) <= planeWidth / 2.0f && 
            fabsf(localHit.y) <= planeHeight / 2.0f);
}

bool vr_ui_raycast(VRUIFramework *framework, const XrVector3f *rayOrigin,
                  const XrVector3f *rayDirection, uint32_t *hitPanelId,
                  XrVector3f *hitPoint) {
    if (!framework || !framework->initialized || !rayOrigin || !rayDirection) {
        return false;
    }
    
    float closestDist = 1000000.0f;
    bool hit = false;
    
    for (uint32_t i = 0; i < MAX_UI_PANELS; i++) {
        if (framework->panels[i].panelId == 0 || !framework->panels[i].visible) {
            continue;
        }
        
        // Panel normal (facing -Z by default)
        XrVector3f normal = {0.0f, 0.0f, 1.0f};
        XrVector3f tempHit;
        
        if (ray_plane_intersect(rayOrigin, rayDirection, &framework->panels[i].position,
                               &normal, framework->panels[i].width, 
                               framework->panels[i].height, &tempHit)) {
            float dist = sqrtf(
                (tempHit.x - rayOrigin->x) * (tempHit.x - rayOrigin->x) +
                (tempHit.y - rayOrigin->y) * (tempHit.y - rayOrigin->y) +
                (tempHit.z - rayOrigin->z) * (tempHit.z - rayOrigin->z)
            );
            
            if (dist < closestDist) {
                closestDist = dist;
                hit = true;
                if (hitPanelId) {
                    *hitPanelId = framework->panels[i].panelId;
                }
                if (hitPoint) {
                    *hitPoint = tempHit;
                }
            }
        }
    }
    
    return hit;
}

int vr_ui_update_gaze(VRUIFramework *framework, const XrVector3f *gazeOrigin,
                     const XrVector3f *gazeDirection) {
    if (!framework || !framework->initialized || !gazeOrigin || !gazeDirection) {
        return -1;
    }
    
    framework->gazeOrigin = *gazeOrigin;
    framework->gazeDirection = *gazeDirection;
    
    return 0;
}

int vr_ui_init_teleportation(VRUIFramework *framework) {
    if (!framework || !framework->initialized) {
        return -1;
    }
    
    framework->teleportActive = false;
    
    return 0;
}

int vr_ui_update_teleport_target(VRUIFramework *framework, const XrVector3f *target) {
    if (!framework || !framework->initialized || !target) {
        return -1;
    }
    
    framework->teleportTarget = *target;
    framework->teleportActive = true;
    
    return 0;
}

int vr_ui_execute_teleport(VRUIFramework *framework, XrVector3f *newPosition) {
    if (!framework || !framework->initialized || !framework->teleportActive || !newPosition) {
        return -1;
    }
    
    *newPosition = framework->teleportTarget;
    framework->teleportActive = false;
    
    return 0;
}

int vr_ui_set_locomotion_mode(VRUIFramework *framework, LocomotionMode mode) {
    if (!framework || !framework->initialized) {
        return -1;
    }
    
    framework->locomotionMode = mode;
    
    return 0;
}

LocomotionMode vr_ui_get_locomotion_mode(VRUIFramework *framework) {
    if (!framework || !framework->initialized) {
        return LOCOMOTION_TELEPORT;
    }
    
    return framework->locomotionMode;
}

void vr_ui_framework_cleanup(VRUIFramework *framework) {
    if (!framework) {
        return;
    }
    
    framework->initialized = false;
    framework->panelCount = 0;
    
    printf("VR UI framework cleaned up\n");
}

void vr_ui_framework_destroy(VRUIFramework *framework) {
    if (!framework) {
        return;
    }
    
    vr_ui_framework_cleanup(framework);
    free(framework);
}
