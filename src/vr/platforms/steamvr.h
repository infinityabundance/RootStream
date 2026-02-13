#ifndef STEAMVR_H
#define STEAMVR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vr_platform_base.h"
#include "../openxr_manager.h"

// SteamVR platform structure
typedef struct SteamVRPlatform SteamVRPlatform;

// Creation
SteamVRPlatform* steamvr_platform_create(void);

// SteamVR-specific features
int steamvr_get_chaperone_bounds(SteamVRPlatform *platform, 
                                XrVector3f *bounds, uint32_t maxBounds, 
                                uint32_t *boundCount);
int steamvr_setup_dashboard(SteamVRPlatform *platform);

// Get base platform interface
VRPlatformBase* steamvr_get_base(SteamVRPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif // STEAMVR_H
