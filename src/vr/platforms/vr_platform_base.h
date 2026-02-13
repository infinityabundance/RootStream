#ifndef VR_PLATFORM_BASE_H
#define VR_PLATFORM_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// VR Platform base interface
typedef struct VRPlatformBase VRPlatformBase;

// Platform capabilities
typedef struct {
    bool supportsHandTracking;
    bool supportsEyeTracking;
    bool supportsPassthrough;
    bool supportsFoveatedRendering;
    bool supportsGuardianSystem;
    uint32_t maxRefreshRate;
    uint32_t recommendedEyeWidth;
    uint32_t recommendedEyeHeight;
} VRPlatformCapabilities;

// Platform interface (virtual methods)
typedef struct {
    int (*init)(VRPlatformBase *platform);
    int (*shutdown)(VRPlatformBase *platform);
    int (*poll_events)(VRPlatformBase *platform);
    VRPlatformCapabilities (*get_capabilities)(VRPlatformBase *platform);
    const char* (*get_platform_name)(VRPlatformBase *platform);
} VRPlatformVTable;

// Base platform structure
struct VRPlatformBase {
    VRPlatformVTable *vtable;
    void *platformData;  // Platform-specific data
    bool initialized;
};

// Base platform functions
VRPlatformBase* vr_platform_base_create(void);
void vr_platform_base_destroy(VRPlatformBase *platform);

// Virtual method wrappers
int vr_platform_init(VRPlatformBase *platform);
int vr_platform_shutdown(VRPlatformBase *platform);
int vr_platform_poll_events(VRPlatformBase *platform);
VRPlatformCapabilities vr_platform_get_capabilities(VRPlatformBase *platform);
const char* vr_platform_get_name(VRPlatformBase *platform);

#ifdef __cplusplus
}
#endif

#endif // VR_PLATFORM_BASE_H
