/**
 * @file vulkan_wayland.h
 * @brief Vulkan Wayland backend for primary display integration - Full featured
 */

#ifndef VULKAN_WAYLAND_H
#define VULKAN_WAYLAND_H

#include "vulkan_renderer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wayland-specific context
 */
typedef struct vulkan_wayland_context_s vulkan_wayland_context_t;

/**
 * Wayland event types
 */
typedef enum {
    VULKAN_WAYLAND_EVENT_NONE = 0,
    VULKAN_WAYLAND_EVENT_RESIZE,
    VULKAN_WAYLAND_EVENT_CLOSE,
    VULKAN_WAYLAND_EVENT_FOCUS_GAINED,
    VULKAN_WAYLAND_EVENT_FOCUS_LOST,
    VULKAN_WAYLAND_EVENT_KEY_PRESS,
    VULKAN_WAYLAND_EVENT_KEY_RELEASE,
    VULKAN_WAYLAND_EVENT_BUTTON_PRESS,
    VULKAN_WAYLAND_EVENT_BUTTON_RELEASE,
    VULKAN_WAYLAND_EVENT_MOTION,
    VULKAN_WAYLAND_EVENT_EXPOSE
} vulkan_wayland_event_type_t;

/**
 * Wayland event structure
 */
typedef struct {
    vulkan_wayland_event_type_t type;
    union {
        struct {
            int width;
            int height;
        } resize;
        struct {
            unsigned int keycode;
            unsigned int keysym;
        } key;
        struct {
            unsigned int button;
            int x;
            int y;
        } button;
        struct {
            int x;
            int y;
        } motion;
    };
} vulkan_wayland_event_t;

/**
 * Monitor (output) information
 */
typedef struct {
    char name[64];
    int x, y;
    int width, height;
    bool is_primary;
} vulkan_wayland_monitor_t;

/**
 * Event callback function
 */
typedef void (*vulkan_wayland_event_callback_t)(const vulkan_wayland_event_t *event, void *user_data);

/**
 * Initialize Wayland backend
 * 
 * @param ctx Pointer to receive Wayland context
 * @param native_window Native window handle (or NULL to create window)
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_init(void **ctx, void *native_window);

/**
 * Create Wayland surface for Vulkan
 * 
 * @param ctx Wayland context
 * @param instance Vulkan instance
 * @param surface Output surface handle
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_create_surface(void *ctx, void *instance, void *surface);

/**
 * Set fullscreen mode
 * 
 * @param ctx Wayland context
 * @param fullscreen True for fullscreen, false for windowed
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_set_fullscreen(void *ctx, bool fullscreen);

/**
 * Set cursor visibility
 * 
 * @param ctx Wayland context
 * @param visible True to show cursor, false to hide
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_set_cursor_visible(void *ctx, bool visible);

/**
 * Confine cursor to window
 * 
 * @param ctx Wayland context
 * @param confine True to confine, false to release
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_confine_cursor(void *ctx, bool confine);

/**
 * Set window title
 * 
 * @param ctx Wayland context
 * @param title Window title string
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_set_window_title(void *ctx, const char *title);

/**
 * Get current window size
 * 
 * @param ctx Wayland context
 * @param width Output width
 * @param height Output height
 * @return 0 on success, -1 on failure
 */
int vulkan_wayland_get_window_size(void *ctx, int *width, int *height);

/**
 * Process pending Wayland events
 * 
 * @param ctx Wayland context
 * @param callback Event callback function (can be NULL)
 * @param user_data User data passed to callback
 * @return Number of events processed, -1 on failure
 */
int vulkan_wayland_process_events(void *ctx, vulkan_wayland_event_callback_t callback, void *user_data);

/**
 * Get information about connected monitors (outputs)
 * 
 * @param ctx Wayland context
 * @param monitors Array to fill with monitor information
 * @param max_monitors Maximum number of monitors to return
 * @return Number of monitors found, -1 on failure
 */
int vulkan_wayland_get_monitors(void *ctx, vulkan_wayland_monitor_t *monitors, int max_monitors);

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
