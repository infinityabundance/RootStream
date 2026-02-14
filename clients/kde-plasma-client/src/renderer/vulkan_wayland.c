/**
 * @file vulkan_wayland.c
 * @brief Vulkan Wayland backend implementation - Full featured
 */

#include "vulkan_wayland.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#if __has_include(<vulkan/vulkan.h>) && __has_include(<vulkan/vulkan_wayland.h>) && __has_include(<wayland-client.h>)
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#define HAVE_WAYLAND_VULKAN 1

// XDG Shell protocol (generated headers)
#if __has_include(<xdg-shell-client-protocol.h>)
#include <xdg-shell-client-protocol.h>
#define HAVE_XDG_SHELL 1
#endif

// Pointer constraints protocol
#if __has_include(<pointer-constraints-unstable-v1-client-protocol.h>)
#include <pointer-constraints-unstable-v1-client-protocol.h>
#define HAVE_POINTER_CONSTRAINTS 1
#endif

// Relative pointer protocol
#if __has_include(<relative-pointer-unstable-v1-client-protocol.h>)
#include <relative-pointer-unstable-v1-client-protocol.h>
#define HAVE_RELATIVE_POINTER 1
#endif

#endif
#endif

// Forward declarations for when Wayland headers are not available
#ifndef HAVE_WAYLAND_VULKAN
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_surface;
struct wl_seat;
struct wl_keyboard;
struct wl_pointer;
struct wl_output;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct wl_cursor_theme;
struct wl_cursor;
struct wl_shm;
#endif

#define MAX_OUTPUTS 16
#define MAX_EVENTS 128

/**
 * Output (monitor) state
 */
typedef struct {
    struct wl_output *output;
    char name[64];
    int x, y;
    int width, height;
    bool is_primary;
} wayland_output_t;

/**
 * Pending event queue
 */
typedef struct {
    vulkan_wayland_event_t events[MAX_EVENTS];
    int count;
} wayland_event_queue_t;

/**
 * Wayland context structure
 */
struct vulkan_wayland_context_s {
#ifdef HAVE_WAYLAND_VULKAN
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    struct wl_pointer *pointer;
    struct wl_shm *shm;
    
#ifdef HAVE_XDG_SHELL
    struct xdg_wm_base *xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
#endif
    
    struct wl_cursor_theme *cursor_theme;
    struct wl_cursor *default_cursor;
    struct wl_surface *cursor_surface;
    
    wayland_output_t outputs[MAX_OUTPUTS];
    int output_count;
    
    wayland_event_queue_t event_queue;
    
    int width, height;
    bool configured;
    bool fullscreen;
    bool cursor_visible;
    bool cursor_confined;
    bool owns_window;
    char title[256];
#else
    int dummy;  // Placeholder when Wayland not available
#endif
};

// Event queue helpers
static void push_event(vulkan_wayland_context_t *ctx, const vulkan_wayland_event_t *event) {
#ifdef HAVE_WAYLAND_VULKAN
    if (ctx->event_queue.count < MAX_EVENTS) {
        ctx->event_queue.events[ctx->event_queue.count++] = *event;
    }
#endif
}

#ifdef HAVE_WAYLAND_VULKAN

// Registry listener
static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface, uint32_t version) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    if (strcmp(interface, "wl_compositor") == 0) {
        ctx->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        ctx->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
#ifdef HAVE_XDG_SHELL
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        ctx->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    }
#endif
    else if (strcmp(interface, "wl_seat") == 0) {
        ctx->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        if (ctx->output_count < MAX_OUTPUTS) {
            ctx->outputs[ctx->output_count].output = 
                wl_registry_bind(registry, name, &wl_output_interface, 1);
            snprintf(ctx->outputs[ctx->output_count].name, 64, "wayland-%d", ctx->output_count);
            ctx->outputs[ctx->output_count].is_primary = (ctx->output_count == 0);
            ctx->output_count++;
        }
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    // Handle global removal
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove
};

#ifdef HAVE_XDG_SHELL
// XDG surface listener
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    xdg_surface_ack_configure(xdg_surface, serial);
    ctx->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    xdg_surface_configure
};

// XDG toplevel listener
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                                   int32_t width, int32_t height, struct wl_array *states) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    if (width > 0 && height > 0) {
        if (ctx->width != width || ctx->height != height) {
            ctx->width = width;
            ctx->height = height;
            
            vulkan_wayland_event_t event = {0};
            event.type = VULKAN_WAYLAND_EVENT_RESIZE;
            event.resize.width = width;
            event.resize.height = height;
            push_event(ctx, &event);
        }
    }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = VULKAN_WAYLAND_EVENT_CLOSE;
    push_event(ctx, &event);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    xdg_toplevel_configure,
    xdg_toplevel_close
};
#endif

// Keyboard listener
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
                           uint32_t format, int32_t fd, uint32_t size) {
    // Handle keymap
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = VULKAN_WAYLAND_EVENT_FOCUS_GAINED;
    push_event(ctx, &event);
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, struct wl_surface *surface) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = VULKAN_WAYLAND_EVENT_FOCUS_LOST;
    push_event(ctx, &event);
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard,
                        uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? 
                 VULKAN_WAYLAND_EVENT_KEY_PRESS : VULKAN_WAYLAND_EVENT_KEY_RELEASE;
    event.key.keycode = key;
    event.key.keysym = key;  // Simplified, would need xkb for proper conversion
    push_event(ctx, &event);
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                              uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
                              uint32_t mods_locked, uint32_t group) {
    // Handle modifiers
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_keymap,
    keyboard_enter,
    keyboard_leave,
    keyboard_key,
    keyboard_modifiers
};

// Pointer listener
static void pointer_enter(void *data, struct wl_pointer *pointer,
                         uint32_t serial, struct wl_surface *surface,
                         wl_fixed_t surface_x, wl_fixed_t surface_y) {
    // Handle pointer enter
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
                         uint32_t serial, struct wl_surface *surface) {
    // Handle pointer leave
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
                          uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = VULKAN_WAYLAND_EVENT_MOTION;
    event.motion.x = wl_fixed_to_int(surface_x);
    event.motion.y = wl_fixed_to_int(surface_y);
    push_event(ctx, &event);
}

static void pointer_button(void *data, struct wl_pointer *pointer,
                          uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    vulkan_wayland_event_t event = {0};
    event.type = (state == WL_POINTER_BUTTON_STATE_PRESSED) ?
                 VULKAN_WAYLAND_EVENT_BUTTON_PRESS : VULKAN_WAYLAND_EVENT_BUTTON_RELEASE;
    event.button.button = button;
    push_event(ctx, &event);
}

static void pointer_axis(void *data, struct wl_pointer *pointer,
                        uint32_t time, uint32_t axis, wl_fixed_t value) {
    // Handle scroll
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_enter,
    pointer_leave,
    pointer_motion,
    pointer_button,
    pointer_axis
};

// Seat listener
static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)data;
    
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        ctx->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(ctx->keyboard, &keyboard_listener, ctx);
    }
    
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        ctx->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(ctx->pointer, &pointer_listener, ctx);
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    // Handle seat name
}

static const struct wl_seat_listener seat_listener = {
    seat_capabilities,
    seat_name
};

#endif // HAVE_WAYLAND_VULKAN

int vulkan_wayland_init(void **ctx_out, void *native_window) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = calloc(1, sizeof(vulkan_wayland_context_t));
    if (!ctx) {
        return -1;
    }
    
    // Default window size
    ctx->width = 1280;
    ctx->height = 720;
    ctx->cursor_visible = true;
    ctx->owns_window = (native_window == NULL);
    strcpy(ctx->title, "RootStream");
    
    // Connect to Wayland display
    ctx->display = wl_display_connect(NULL);
    if (!ctx->display) {
        free(ctx);
        return -1;
    }
    
    // Get registry and bind globals
    ctx->registry = wl_display_get_registry(ctx->display);
    wl_registry_add_listener(ctx->registry, &registry_listener, ctx);
    
    // First roundtrip to get globals
    wl_display_roundtrip(ctx->display);
    
    if (!ctx->compositor) {
        wl_display_disconnect(ctx->display);
        free(ctx);
        return -1;
    }
    
    // Set up seat if available
    if (ctx->seat) {
        wl_seat_add_listener(ctx->seat, &seat_listener, ctx);
    }
    
    // Create surface
    ctx->surface = wl_compositor_create_surface(ctx->compositor);
    if (!ctx->surface) {
        wl_display_disconnect(ctx->display);
        free(ctx);
        return -1;
    }
    
#ifdef HAVE_XDG_SHELL
    // Create XDG surface if available
    if (ctx->xdg_wm_base && ctx->owns_window) {
        ctx->xdg_surface = xdg_wm_base_get_xdg_surface(ctx->xdg_wm_base, ctx->surface);
        if (ctx->xdg_surface) {
            xdg_surface_add_listener(ctx->xdg_surface, &xdg_surface_listener, ctx);
            
            ctx->xdg_toplevel = xdg_surface_get_toplevel(ctx->xdg_surface);
            if (ctx->xdg_toplevel) {
                xdg_toplevel_add_listener(ctx->xdg_toplevel, &xdg_toplevel_listener, ctx);
                xdg_toplevel_set_title(ctx->xdg_toplevel, ctx->title);
                xdg_toplevel_set_app_id(ctx->xdg_toplevel, "rootstream");
            }
        }
        
        wl_surface_commit(ctx->surface);
        
        // Wait for configure
        while (!ctx->configured) {
            wl_display_dispatch(ctx->display);
        }
    }
#endif
    
    // Load cursor theme if available
    if (ctx->shm) {
        ctx->cursor_theme = wl_cursor_theme_load(NULL, 24, ctx->shm);
        if (ctx->cursor_theme) {
            ctx->default_cursor = wl_cursor_theme_get_cursor(ctx->cursor_theme, "left_ptr");
            if (ctx->default_cursor) {
                ctx->cursor_surface = wl_compositor_create_surface(ctx->compositor);
            }
        }
    }
    
    // Second roundtrip to complete setup
    wl_display_roundtrip(ctx->display);
    
    *ctx_out = ctx;
    return 0;
#else
    return -1;  // Wayland not available
#endif
}

int vulkan_wayland_create_surface(void *ctx_ptr, void *instance, void *surface_out) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    VkInstance vk_instance = (VkInstance)instance;
    VkSurfaceKHR *vk_surface = (VkSurfaceKHR*)surface_out;
    
    if (!ctx || !ctx->display || !ctx->surface) {
        return -1;
    }
    
    VkWaylandSurfaceCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info.display = ctx->display;
    create_info.surface = ctx->surface;
    
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = 
        (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(vk_instance, "vkCreateWaylandSurfaceKHR");
    
    if (!vkCreateWaylandSurfaceKHR) {
        return -1;
    }
    
    VkResult result = vkCreateWaylandSurfaceKHR(vk_instance, &create_info, NULL, vk_surface);
    return (result == VK_SUCCESS) ? 0 : -1;
#else
    return -1;
#endif
}

int vulkan_wayland_set_fullscreen(void *ctx_ptr, bool fullscreen) {
#if defined(HAVE_WAYLAND_VULKAN) && defined(HAVE_XDG_SHELL)
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !ctx->xdg_toplevel) {
        return -1;
    }
    
    if (fullscreen && !ctx->fullscreen) {
        xdg_toplevel_set_fullscreen(ctx->xdg_toplevel, NULL);
        ctx->fullscreen = true;
    } else if (!fullscreen && ctx->fullscreen) {
        xdg_toplevel_unset_fullscreen(ctx->xdg_toplevel);
        ctx->fullscreen = false;
    }
    
    wl_display_roundtrip(ctx->display);
    return 0;
#else
    return -1;
#endif
}

int vulkan_wayland_set_cursor_visible(void *ctx_ptr, bool visible) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !ctx->pointer) {
        return -1;
    }
    
    ctx->cursor_visible = visible;
    
    if (visible) {
        if (ctx->default_cursor && ctx->cursor_surface) {
            struct wl_cursor_image *image = ctx->default_cursor->images[0];
            wl_pointer_set_cursor(ctx->pointer, 0, ctx->cursor_surface,
                                 image->hotspot_x, image->hotspot_y);
            wl_surface_attach(ctx->cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
            wl_surface_damage(ctx->cursor_surface, 0, 0, image->width, image->height);
            wl_surface_commit(ctx->cursor_surface);
        }
    } else {
        wl_pointer_set_cursor(ctx->pointer, 0, NULL, 0, 0);
    }
    
    return 0;
#else
    return -1;
#endif
}

int vulkan_wayland_confine_cursor(void *ctx_ptr, bool confine) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx) {
        return -1;
    }
    
    ctx->cursor_confined = confine;
    // Note: Actual confinement would require zwp_pointer_constraints_v1 protocol
    // which may not be available. This is a simplified implementation.
    
    return 0;
#else
    return -1;
#endif
}

int vulkan_wayland_set_window_title(void *ctx_ptr, const char *title) {
#if defined(HAVE_WAYLAND_VULKAN) && defined(HAVE_XDG_SHELL)
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !title) {
        return -1;
    }
    
    strncpy(ctx->title, title, sizeof(ctx->title) - 1);
    ctx->title[sizeof(ctx->title) - 1] = '\0';
    
    if (ctx->xdg_toplevel) {
        xdg_toplevel_set_title(ctx->xdg_toplevel, ctx->title);
    }
    
    return 0;
#else
    return -1;
#endif
}

int vulkan_wayland_get_window_size(void *ctx_ptr, int *width, int *height) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !width || !height) {
        return -1;
    }
    
    *width = ctx->width;
    *height = ctx->height;
    return 0;
#else
    return -1;
#endif
}

int vulkan_wayland_process_events(void *ctx_ptr, vulkan_wayland_event_callback_t callback, void *user_data) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !ctx->display) {
        return -1;
    }
    
    // Dispatch pending events
    wl_display_dispatch_pending(ctx->display);
    
    // Process queued events
    int processed = ctx->event_queue.count;
    
    if (callback) {
        for (int i = 0; i < ctx->event_queue.count; i++) {
            callback(&ctx->event_queue.events[i], user_data);
        }
    }
    
    ctx->event_queue.count = 0;
    
    return processed;
#else
    return -1;
#endif
}

int vulkan_wayland_get_monitors(void *ctx_ptr, vulkan_wayland_monitor_t *monitors, int max_monitors) {
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (!ctx || !monitors || max_monitors <= 0) {
        return -1;
    }
    
    int count = (ctx->output_count < max_monitors) ? ctx->output_count : max_monitors;
    
    for (int i = 0; i < count; i++) {
        strncpy(monitors[i].name, ctx->outputs[i].name, sizeof(monitors[i].name) - 1);
        monitors[i].x = ctx->outputs[i].x;
        monitors[i].y = ctx->outputs[i].y;
        monitors[i].width = ctx->outputs[i].width;
        monitors[i].height = ctx->outputs[i].height;
        monitors[i].is_primary = ctx->outputs[i].is_primary;
    }
    
    return count;
#else
    return -1;
#endif
}

void vulkan_wayland_cleanup(void *ctx_ptr) {
    if (!ctx_ptr) {
        return;
    }
    
#ifdef HAVE_WAYLAND_VULKAN
    vulkan_wayland_context_t *ctx = (vulkan_wayland_context_t*)ctx_ptr;
    
    if (ctx->cursor_surface) {
        wl_surface_destroy(ctx->cursor_surface);
    }
    
    if (ctx->cursor_theme) {
        wl_cursor_theme_destroy(ctx->cursor_theme);
    }
    
#ifdef HAVE_XDG_SHELL
    if (ctx->xdg_toplevel) {
        xdg_toplevel_destroy(ctx->xdg_toplevel);
    }
    
    if (ctx->xdg_surface) {
        xdg_surface_destroy(ctx->xdg_surface);
    }
#endif
    
    if (ctx->keyboard) {
        wl_keyboard_destroy(ctx->keyboard);
    }
    
    if (ctx->pointer) {
        wl_pointer_destroy(ctx->pointer);
    }
    
    if (ctx->seat) {
        wl_seat_destroy(ctx->seat);
    }
    
    if (ctx->surface) {
        wl_surface_destroy(ctx->surface);
    }
    
#ifdef HAVE_XDG_SHELL
    if (ctx->xdg_wm_base) {
        xdg_wm_base_destroy(ctx->xdg_wm_base);
    }
#endif
    
    if (ctx->shm) {
        wl_shm_destroy(ctx->shm);
    }
    
    if (ctx->compositor) {
        wl_compositor_destroy(ctx->compositor);
    }
    
    for (int i = 0; i < ctx->output_count; i++) {
        if (ctx->outputs[i].output) {
            wl_output_destroy(ctx->outputs[i].output);
        }
    }
    
    if (ctx->registry) {
        wl_registry_destroy(ctx->registry);
    }
    
    if (ctx->display) {
        wl_display_disconnect(ctx->display);
    }
#endif
    
    free(ctx_ptr);
}
