#ifndef APPLE_VISION_H
#define APPLE_VISION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vr_platform_base.h"
#include "../openxr_manager.h"

// Apple Vision Pro platform structure
typedef struct AppleVisionPlatform AppleVisionPlatform;

// Creation
AppleVisionPlatform* apple_vision_platform_create(void);

// Vision Pro-specific features
int apple_vision_enable_passthrough(AppleVisionPlatform *platform, bool enable);
int apple_vision_setup_spatial_computing(AppleVisionPlatform *platform);

// Get base platform interface
VRPlatformBase* apple_vision_get_base(AppleVisionPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif // APPLE_VISION_H
