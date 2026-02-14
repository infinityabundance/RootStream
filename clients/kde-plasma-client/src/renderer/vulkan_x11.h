/**
 * @file vulkan_x11.h
 * @brief Vulkan X11 backend for fallback display integration - Full featured
 */

#ifndef VULKAN_X11_H
#define VULKAN_X11_H

#include "vulkan_renderer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * X11-specific context
 */
typedef struct vulkan_x11_context_s vulkan_x11_context_t;

/**
 * X11 event types
 */
typedef enum {
    VULKAN_X11_EVENT_NONE = 0,
    VULKAN_X11_EVENT_RESIZE,
    VULKAN_X11_EVENT_CLOSE,
    VULKAN_X11_EVENT_FOCUS_GAINED,
    VULKAN_X11_EVENT_FOCUS_LOST,
    VULKAN_X11_EVENT_KEY_PRESS,
    VULKAN_X11_EVENT_KEY_RELEASE,
    VULKAN_X11_EVENT_BUTTON_PRESS,
    VULKAN_X11_EVENT_BUTTON_RELEASE,
    VULKAN_X11_EVENT_MOTION,
    VULKAN_X11_EVENT_EXPOSE
} vulkan_x11_event_type_t;

/**
 * X11 event structure
 */
typedef struct {
    vulkan_x11_event_type_t type;
    union {
        struct {
            int width;
            int height;
        } resize;
        struct {
            unsigned int keycode;
            unsigned long keysym;
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
} vulkan_x11_event_t;

/**
 * Monitor information
 */
typedef struct {
    char name[64];
    int x, y;
    int width, height;
    bool is_primary;
} vulkan_x11_monitor_t;

/**
 * Event callback function
 */
typedef void (*vulkan_x11_event_callback_t)(const vulkan_x11_event_t *event, void *user_data);

/**
 * Initialize X11 backend
 * 
 * @param ctx Vulkan context
 * @param native_window Native window handle (or NULL to create window)
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
 * Set fullscreen mode
 * 
 * @param ctx X11 context
 * @param fullscreen True for fullscreen, false for windowed
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_set_fullscreen(void *ctx, bool fullscreen);

/**
 * Set cursor visibility
 * 
 * @param ctx X11 context
 * @param visible True to show cursor, false to hide
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_set_cursor_visible(void *ctx, bool visible);

/**
 * Confine cursor to window
 * 
 * @param ctx X11 context
 * @param confine True to confine, false to release
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_confine_cursor(void *ctx, bool confine);

/**
 * Set window title
 * 
 * @param ctx X11 context
 * @param title Window title string
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_set_window_title(void *ctx, const char *title);

/**
 * Get current window size
 * 
 * @param ctx X11 context
 * @param width Output width
 * @param height Output height
 * @return 0 on success, -1 on failure
 */
int vulkan_x11_get_window_size(void *ctx, int *width, int *height);

/**
 * Process pending X11 events
 * 
 * @param ctx X11 context
 * @param callback Event callback function (can be NULL)
 * @param user_data User data passed to callback
 * @return Number of events processed, -1 on failure
 */
int vulkan_x11_process_events(void *ctx, vulkan_x11_event_callback_t callback, void *user_data);

/**
 * Get information about connected monitors
 * 
 * @param ctx X11 context
 * @param monitors Array to fill with monitor information
 * @param max_monitors Maximum number of monitors to return
 * @return Number of monitors found, -1 on failure
 */
int vulkan_x11_get_monitors(void *ctx, vulkan_x11_monitor_t *monitors, int max_monitors);

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
