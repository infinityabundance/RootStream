/**
 * @file vulkan_x11.c
 * @brief Vulkan X11 backend implementation - Full featured
 */

#include "vulkan_x11.h"
#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#if __has_include(<vulkan/vulkan.h>) && __has_include(<vulkan/vulkan_xlib.h>) && __has_include(<X11/Xlib.h>)
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#define HAVE_X11_VULKAN 1
#endif
#endif

// Forward declarations for when X11 headers are not available
#ifndef HAVE_X11_VULKAN
typedef void* Display;
typedef unsigned long Window;
typedef unsigned long Atom;
typedef unsigned long Cursor;
#endif

struct vulkan_x11_context_s {
    Display *display;
    Window window;
    int screen;
    bool owns_window;
    bool is_fullscreen;
    bool cursor_hidden;
    
    // Window state
    int windowed_x, windowed_y;
    int windowed_width, windowed_height;
    
    // Atoms for window management
#ifdef HAVE_X11_VULKAN
    Atom wm_delete_window;
    Atom wm_state;
    Atom wm_state_fullscreen;
    Atom wm_protocols;
    Cursor invisible_cursor;
#endif
};

int vulkan_x11_init(void *ctx, void *native_window) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)native_window;
    return -1;  // X11/Vulkan headers not available
#else
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)calloc(1, sizeof(vulkan_x11_context_t));
    if (!x11_ctx) {
        return -1;
    }
    
    // Open X11 display connection
    x11_ctx->display = XOpenDisplay(NULL);
    if (!x11_ctx->display) {
        free(x11_ctx);
        return -1;
    }
    
    x11_ctx->screen = DefaultScreen(x11_ctx->display);
    x11_ctx->is_fullscreen = false;
    x11_ctx->cursor_hidden = false;
    
    // Get window management atoms
    x11_ctx->wm_protocols = XInternAtom(x11_ctx->display, "WM_PROTOCOLS", False);
    x11_ctx->wm_delete_window = XInternAtom(x11_ctx->display, "WM_DELETE_WINDOW", False);
    x11_ctx->wm_state = XInternAtom(x11_ctx->display, "_NET_WM_STATE", False);
    x11_ctx->wm_state_fullscreen = XInternAtom(x11_ctx->display, "_NET_WM_STATE_FULLSCREEN", False);
    
    // If native_window provided, use it; otherwise create our own window
    if (native_window) {
        x11_ctx->window = *(Window*)native_window;
        x11_ctx->owns_window = false;
    } else {
        Window root = RootWindow(x11_ctx->display, x11_ctx->screen);
        
        // Create window with event mask
        XSetWindowAttributes attrs = {0};
        attrs.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                          ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                          FocusChangeMask | ExposureMask;
        attrs.background_pixel = BlackPixel(x11_ctx->display, x11_ctx->screen);
        
        x11_ctx->window = XCreateWindow(
            x11_ctx->display,
            root,
            0, 0,                           // x, y
            DEFAULT_RENDER_WIDTH,           // width
            DEFAULT_RENDER_HEIGHT,          // height
            0,                              // border_width
            CopyFromParent,                 // depth
            InputOutput,                    // class
            CopyFromParent,                 // visual
            CWBackPixel | CWEventMask,      // value mask
            &attrs
        );
        
        if (!x11_ctx->window) {
            XCloseDisplay(x11_ctx->display);
            free(x11_ctx);
            return -1;
        }
        
        x11_ctx->owns_window = true;
        x11_ctx->windowed_width = DEFAULT_RENDER_WIDTH;
        x11_ctx->windowed_height = DEFAULT_RENDER_HEIGHT;
        
        // Set window properties
        XStoreName(x11_ctx->display, x11_ctx->window, "RootStream Client");
        
        // Set WM_CLASS for window managers
        XClassHint class_hint;
        class_hint.res_name = (char*)"rootstream";
        class_hint.res_class = (char*)"RootStream";
        XSetClassHint(x11_ctx->display, x11_ctx->window, &class_hint);
        
        // Set size hints
        XSizeHints size_hints = {0};
        size_hints.flags = PMinSize | PMaxSize;
        size_hints.min_width = 640;
        size_hints.min_height = 480;
        size_hints.max_width = 7680;  // 8K width
        size_hints.max_height = 4320; // 8K height
        XSetWMNormalHints(x11_ctx->display, x11_ctx->window, &size_hints);
        
        // Set WM_DELETE_WINDOW protocol
        XSetWMProtocols(x11_ctx->display, x11_ctx->window, &x11_ctx->wm_delete_window, 1);
        
        // Show window
        XMapWindow(x11_ctx->display, x11_ctx->window);
        XFlush(x11_ctx->display);
    }
    
    // Create invisible cursor for when we hide it
    char cursor_data[1] = {0};
    XColor dummy_color = {0};
    Pixmap cursor_pixmap = XCreateBitmapFromData(x11_ctx->display, x11_ctx->window, cursor_data, 1, 1);
    x11_ctx->invisible_cursor = XCreatePixmapCursor(x11_ctx->display, cursor_pixmap, cursor_pixmap,
                                                    &dummy_color, &dummy_color, 0, 0);
    XFreePixmap(x11_ctx->display, cursor_pixmap);
    
    // Store context in the provided pointer location
    *(vulkan_x11_context_t**)ctx = x11_ctx;
    
    return 0;
#endif
}

int vulkan_x11_create_surface(void *ctx, void *instance, void *surface) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)instance;
    (void)surface;
    return -1;  // X11/Vulkan headers not available
#else
    if (!ctx || !instance || !surface) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    VkInstance vk_instance = (VkInstance)instance;
    VkSurfaceKHR *vk_surface = (VkSurfaceKHR*)surface;
    
    VkXlibSurfaceCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    create_info.dpy = x11_ctx->display;
    create_info.window = x11_ctx->window;
    
    VkResult result = vkCreateXlibSurfaceKHR(vk_instance, &create_info, NULL, vk_surface);
    if (result != VK_SUCCESS) {
        return -1;
    }
    
    return 0;
#endif
}

int vulkan_x11_set_fullscreen(void *ctx, bool fullscreen) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)fullscreen;
    return -1;
#else
    if (!ctx) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    if (x11_ctx->is_fullscreen == fullscreen) {
        return 0;  // Already in desired state
    }
    
    XEvent event = {0};
    event.type = ClientMessage;
    event.xclient.window = x11_ctx->window;
    event.xclient.message_type = x11_ctx->wm_state;
    event.xclient.format = 32;
    event.xclient.data.l[0] = fullscreen ? 1 : 0;  // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = x11_ctx->wm_state_fullscreen;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;
    
    XSendEvent(x11_ctx->display, 
               RootWindow(x11_ctx->display, x11_ctx->screen),
               False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &event);
    
    XFlush(x11_ctx->display);
    x11_ctx->is_fullscreen = fullscreen;
    
    return 0;
#endif
}

int vulkan_x11_set_cursor_visible(void *ctx, bool visible) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)visible;
    return -1;
#else
    if (!ctx) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    if (visible && x11_ctx->cursor_hidden) {
        XUndefineCursor(x11_ctx->display, x11_ctx->window);
        x11_ctx->cursor_hidden = false;
    } else if (!visible && !x11_ctx->cursor_hidden) {
        XDefineCursor(x11_ctx->display, x11_ctx->window, x11_ctx->invisible_cursor);
        x11_ctx->cursor_hidden = true;
    }
    
    XFlush(x11_ctx->display);
    return 0;
#endif
}

int vulkan_x11_confine_cursor(void *ctx, bool confine) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)confine;
    return -1;
#else
    if (!ctx) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    if (confine) {
        // Grab pointer to confine it to our window
        XGrabPointer(x11_ctx->display, x11_ctx->window, True,
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync, x11_ctx->window, None, CurrentTime);
    } else {
        // Release pointer grab
        XUngrabPointer(x11_ctx->display, CurrentTime);
    }
    
    XFlush(x11_ctx->display);
    return 0;
#endif
}

int vulkan_x11_set_window_title(void *ctx, const char *title) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)title;
    return -1;
#else
    if (!ctx || !title) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    XStoreName(x11_ctx->display, x11_ctx->window, title);
    XFlush(x11_ctx->display);
    
    return 0;
#endif
}

int vulkan_x11_get_window_size(void *ctx, int *width, int *height) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)width;
    (void)height;
    return -1;
#else
    if (!ctx) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    XWindowAttributes attrs;
    XGetWindowAttributes(x11_ctx->display, x11_ctx->window, &attrs);
    
    if (width) *width = attrs.width;
    if (height) *height = attrs.height;
    
    return 0;
#endif
}

int vulkan_x11_process_events(void *ctx, vulkan_x11_event_callback_t callback, void *user_data) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)callback;
    (void)user_data;
    return -1;
#else
    if (!ctx) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    int event_count = 0;
    
    // Process all pending events
    while (XPending(x11_ctx->display) > 0) {
        XEvent event;
        XNextEvent(x11_ctx->display, &event);
        event_count++;
        
        vulkan_x11_event_t x11_event = {0};
        
        switch (event.type) {
            case ConfigureNotify:
                x11_event.type = VULKAN_X11_EVENT_RESIZE;
                x11_event.resize.width = event.xconfigure.width;
                x11_event.resize.height = event.xconfigure.height;
                break;
                
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == x11_ctx->wm_delete_window) {
                    x11_event.type = VULKAN_X11_EVENT_CLOSE;
                }
                break;
                
            case FocusIn:
                x11_event.type = VULKAN_X11_EVENT_FOCUS_GAINED;
                break;
                
            case FocusOut:
                x11_event.type = VULKAN_X11_EVENT_FOCUS_LOST;
                break;
                
            case KeyPress:
                x11_event.type = VULKAN_X11_EVENT_KEY_PRESS;
                x11_event.key.keycode = event.xkey.keycode;
                x11_event.key.keysym = XLookupKeysym(&event.xkey, 0);
                break;
                
            case KeyRelease:
                x11_event.type = VULKAN_X11_EVENT_KEY_RELEASE;
                x11_event.key.keycode = event.xkey.keycode;
                x11_event.key.keysym = XLookupKeysym(&event.xkey, 0);
                break;
                
            case ButtonPress:
                x11_event.type = VULKAN_X11_EVENT_BUTTON_PRESS;
                x11_event.button.button = event.xbutton.button;
                x11_event.button.x = event.xbutton.x;
                x11_event.button.y = event.xbutton.y;
                break;
                
            case ButtonRelease:
                x11_event.type = VULKAN_X11_EVENT_BUTTON_RELEASE;
                x11_event.button.button = event.xbutton.button;
                x11_event.button.x = event.xbutton.x;
                x11_event.button.y = event.xbutton.y;
                break;
                
            case MotionNotify:
                x11_event.type = VULKAN_X11_EVENT_MOTION;
                x11_event.motion.x = event.xmotion.x;
                x11_event.motion.y = event.xmotion.y;
                break;
                
            case Expose:
                x11_event.type = VULKAN_X11_EVENT_EXPOSE;
                break;
                
            default:
                // Unknown event, skip callback
                continue;
        }
        
        // Call user callback if provided
        if (callback) {
            callback(&x11_event, user_data);
        }
    }
    
    return event_count;
#endif
}

int vulkan_x11_get_monitors(void *ctx, vulkan_x11_monitor_t *monitors, int max_monitors) {
#ifndef HAVE_X11_VULKAN
    (void)ctx;
    (void)monitors;
    (void)max_monitors;
    return -1;
#else
    if (!ctx || !monitors || max_monitors <= 0) {
        return -1;
    }
    
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    int num_monitors = 0;
    XRRScreenResources *screen_res = XRRGetScreenResources(x11_ctx->display, 
                                                           RootWindow(x11_ctx->display, x11_ctx->screen));
    
    if (screen_res) {
        for (int i = 0; i < screen_res->noutput && num_monitors < max_monitors; i++) {
            XRROutputInfo *output_info = XRRGetOutputInfo(x11_ctx->display, screen_res, 
                                                          screen_res->outputs[i]);
            
            if (output_info && output_info->crtc && output_info->connection == RR_Connected) {
                XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(x11_ctx->display, screen_res, 
                                                        output_info->crtc);
                
                if (crtc_info) {
                    monitors[num_monitors].x = crtc_info->x;
                    monitors[num_monitors].y = crtc_info->y;
                    monitors[num_monitors].width = crtc_info->width;
                    monitors[num_monitors].height = crtc_info->height;
                    strncpy(monitors[num_monitors].name, output_info->name, 
                           sizeof(monitors[num_monitors].name) - 1);
                    monitors[num_monitors].name[sizeof(monitors[num_monitors].name) - 1] = '\0';
                    monitors[num_monitors].is_primary = (i == 0);  // First output is typically primary
                    
                    num_monitors++;
                    XRRFreeCrtcInfo(crtc_info);
                }
            }
            
            if (output_info) {
                XRRFreeOutputInfo(output_info);
            }
        }
        
        XRRFreeScreenResources(screen_res);
    }
    
    return num_monitors;
#endif
}

void vulkan_x11_cleanup(void *ctx) {
    if (!ctx) {
        return;
    }
    
#ifdef HAVE_X11_VULKAN
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    if (x11_ctx->display) {
        // Free cursor if created
        if (x11_ctx->invisible_cursor) {
            XFreeCursor(x11_ctx->display, x11_ctx->invisible_cursor);
        }
        
        // Destroy window if we own it
        if (x11_ctx->owns_window && x11_ctx->window) {
            XDestroyWindow(x11_ctx->display, x11_ctx->window);
        }
        
        XCloseDisplay(x11_ctx->display);
    }
#endif
    
    free(ctx);
}
