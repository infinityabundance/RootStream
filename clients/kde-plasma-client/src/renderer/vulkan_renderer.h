/**
 * @file vulkan_renderer.h
 * @brief Vulkan rendering backend for RootStream client
 * 
 * Implements video rendering using Vulkan with support for:
 * - Wayland (primary backend)
 * - X11 (fallback backend)
 * - Headless (final fallback for CI/testing)
 * 
 * Maintains API compatibility with OpenGL renderer.
 */

#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include "renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Vulkan display backend types
 */
typedef enum {
    VULKAN_BACKEND_WAYLAND,   /**< Wayland compositor (primary) */
    VULKAN_BACKEND_X11,       /**< X11 display server (fallback) */
    VULKAN_BACKEND_HEADLESS,  /**< Offscreen rendering (final fallback) */
} vulkan_backend_t;

/**
 * Opaque Vulkan context handle
 */
typedef struct vulkan_context_s vulkan_context_t;

/**
 * Detect available Vulkan backend
 * 
 * Checks in priority order: Wayland → X11 → Headless
 * 
 * @return Best available backend type
 */
vulkan_backend_t vulkan_detect_backend(void);

/**
 * Initialize Vulkan renderer
 * 
 * Creates Vulkan instance, device, and backend-specific surface.
 * 
 * @param native_window Native window handle (or NULL for headless)
 * @return Vulkan context, or NULL on failure
 */
vulkan_context_t* vulkan_init(void *native_window);

/**
 * Upload frame data to GPU
 * 
 * Uploads NV12 frame data to Vulkan image memory.
 * 
 * @param ctx Vulkan context
 * @param frame Frame to upload
 * @return 0 on success, -1 on failure
 */
int vulkan_upload_frame(vulkan_context_t *ctx, const frame_t *frame);

/**
 * Render current frame
 * 
 * Applies NV12→RGB conversion and renders to swapchain/offscreen buffer.
 * 
 * @param ctx Vulkan context
 * @return 0 on success, -1 on failure
 */
int vulkan_render(vulkan_context_t *ctx);

/**
 * Present rendered frame
 * 
 * Presents to display or writes to memory (headless mode).
 * 
 * @param ctx Vulkan context
 * @return 0 on success, -1 on failure
 */
int vulkan_present(vulkan_context_t *ctx);

/**
 * Enable or disable vsync
 * 
 * @param ctx Vulkan context
 * @param enabled True to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int vulkan_set_vsync(vulkan_context_t *ctx, bool enabled);

/**
 * Resize rendering surface
 * 
 * @param ctx Vulkan context
 * @param width New width
 * @param height New height
 * @return 0 on success, -1 on failure
 */
int vulkan_resize(vulkan_context_t *ctx, int width, int height);

/**
 * Get active backend name
 * 
 * @param ctx Vulkan context
 * @return Backend name ("wayland", "x11", or "headless")
 */
const char* vulkan_get_backend_name(vulkan_context_t *ctx);

/**
 * Clean up Vulkan resources
 * 
 * Destroys device, instance, and backend-specific resources.
 * 
 * @param ctx Vulkan context
 */
void vulkan_cleanup(vulkan_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VULKAN_RENDERER_H */
