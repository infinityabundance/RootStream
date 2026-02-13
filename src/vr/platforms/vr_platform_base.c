#include "vr_platform_base.h"
#include <stdlib.h>
#include <stdio.h>

VRPlatformBase* vr_platform_base_create(void) {
    VRPlatformBase *platform = (VRPlatformBase*)calloc(1, sizeof(VRPlatformBase));
    if (!platform) {
        fprintf(stderr, "Failed to allocate VRPlatformBase\n");
        return NULL;
    }
    
    platform->initialized = false;
    platform->vtable = NULL;
    platform->platformData = NULL;
    
    return platform;
}

void vr_platform_base_destroy(VRPlatformBase *platform) {
    if (!platform) {
        return;
    }
    
    if (platform->initialized && platform->vtable && platform->vtable->shutdown) {
        platform->vtable->shutdown(platform);
    }
    
    // Note: platformData points to the platform struct itself for derived types,
    // so we don't free it separately - we just free the whole struct
    free(platform);
}

int vr_platform_init(VRPlatformBase *platform) {
    if (!platform || !platform->vtable || !platform->vtable->init) {
        return -1;
    }
    
    return platform->vtable->init(platform);
}

int vr_platform_shutdown(VRPlatformBase *platform) {
    if (!platform || !platform->vtable || !platform->vtable->shutdown) {
        return -1;
    }
    
    return platform->vtable->shutdown(platform);
}

int vr_platform_poll_events(VRPlatformBase *platform) {
    if (!platform || !platform->vtable || !platform->vtable->poll_events) {
        return -1;
    }
    
    return platform->vtable->poll_events(platform);
}

VRPlatformCapabilities vr_platform_get_capabilities(VRPlatformBase *platform) {
    VRPlatformCapabilities empty = {0};
    
    if (!platform || !platform->vtable || !platform->vtable->get_capabilities) {
        return empty;
    }
    
    return platform->vtable->get_capabilities(platform);
}

const char* vr_platform_get_name(VRPlatformBase *platform) {
    if (!platform || !platform->vtable || !platform->vtable->get_platform_name) {
        return "Unknown";
    }
    
    return platform->vtable->get_platform_name(platform);
}
