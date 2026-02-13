/**
 * @file vulkan_x11.h
 * @brief Vulkan X11 backend for fallback display integration
 */

#ifndef VULKAN_X11_H
#define VULKAN_X11_H

#include "vulkan_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * X11-specific context
 */
typedef struct vulkan_x11_context_s vulkan_x11_context_t;

/**
 * Initialize X11 backend
 * 
 * @param ctx Vulkan context
 * @param native_window Native window handle
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_init(void *ctx, void *native_window);

/**
 * Create X11 surface
 * 
 * @param ctx Vulkan context
 * @param instance Vulkan instance
 * @param surface Output surface handle
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_create_surface(void *ctx, void *instance, void *surface);

/**
 * Cleanup X11 backend
 * 
 * @param ctx X11 context
 */
void vulkan_x11_cleanup(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VULKAN_X11_H */
