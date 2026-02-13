/**
 * @file vulkan_wayland.h
 * @brief Vulkan Wayland backend for primary display integration
 */

#ifndef VULKAN_WAYLAND_H
#define VULKAN_WAYLAND_H

#include "vulkan_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wayland-specific context
 */
typedef struct vulkan_wayland_context_s vulkan_wayland_context_t;

/**
 * Initialize Wayland backend
 * 
 * @param ctx Vulkan context
 * @param native_window Native window handle
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_init(void *ctx, void *native_window);

/**
 * Create Wayland surface
 * 
 * @param ctx Vulkan context
 * @param instance Vulkan instance
 * @param surface Output surface handle
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_create_surface(void *ctx, void *instance, void *surface);

/**
 * Cleanup Wayland backend
 * 
 * @param ctx Wayland context
 */
void vulkan_wayland_cleanup(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VULKAN_WAYLAND_H */
