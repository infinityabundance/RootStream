/**
 * @file vulkan_x11.c
 * @brief Vulkan X11 backend implementation
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
#define HAVE_X11_VULKAN 1
#endif
#endif

// Forward declarations for when X11 headers are not available
#ifndef HAVE_X11_VULKAN
typedef void* Display;
typedef unsigned long Window;
#endif

struct vulkan_x11_context_s {
    Display *display;
    Window window;
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
    
    // If native_window provided, use it; otherwise create our own window
    if (native_window) {
        x11_ctx->window = *(Window*)native_window;
    } else {
        // Create a basic window for rendering
        int screen = DefaultScreen(x11_ctx->display);
        Window root = RootWindow(x11_ctx->display, screen);
        
        x11_ctx->window = XCreateSimpleWindow(
            x11_ctx->display,
            root,
            0, 0,                           // x, y
            DEFAULT_RENDER_WIDTH,           // width
            DEFAULT_RENDER_HEIGHT,          // height
            1,                              // border_width
            BlackPixel(x11_ctx->display, screen),  // border
            BlackPixel(x11_ctx->display, screen)   // background
        );
        
        if (!x11_ctx->window) {
            XCloseDisplay(x11_ctx->display);
            free(x11_ctx);
            return -1;
        }
        
        // Set window properties
        XStoreName(x11_ctx->display, x11_ctx->window, "RootStream Client");
        
        // Show window
        XMapWindow(x11_ctx->display, x11_ctx->window);
        XFlush(x11_ctx->display);
    }
    
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

void vulkan_x11_cleanup(void *ctx) {
    if (!ctx) {
        return;
    }
    
#ifdef HAVE_X11_VULKAN
    vulkan_x11_context_t *x11_ctx = (vulkan_x11_context_t*)ctx;
    
    if (x11_ctx->display) {
        XCloseDisplay(x11_ctx->display);
    }
#endif
    
    free(ctx);
}
