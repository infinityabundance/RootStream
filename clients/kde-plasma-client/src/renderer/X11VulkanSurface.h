/**
 * @file X11VulkanSurface.h
 * @brief C++ wrapper around the C vulkan_x11 backend
 *
 * Provides an RAII class that owns the X11/Vulkan surface lifetime and
 * delegates to the underlying C API (vulkan_x11_init, vulkan_x11_create_surface,
 * vulkan_x11_process_events, vulkan_x11_cleanup).
 *
 * The header compiles without errors even when X11 or Vulkan SDK headers are
 * absent; opaque pointer fallbacks are supplied via __has_include guards.
 */

#ifndef X11_VULKAN_SURFACE_H
#define X11_VULKAN_SURFACE_H

#include "vulkan_x11.h"

/* ── X11 type availability ───────────────────────────────────────────────── */
#if __has_include(<X11/Xlib.h>)
#  include <X11/Xlib.h>
#else
   /** Fallback opaque type when X11 headers are unavailable. */
   typedef void  Display;
   /** Fallback opaque type when X11 headers are unavailable. */
   typedef unsigned long Window;
#endif

/* ── Vulkan type availability ────────────────────────────────────────────── */
#if __has_include(<vulkan/vulkan.h>)
#  include <vulkan/vulkan.h>
#else
   /** Fallback opaque handle when Vulkan SDK headers are unavailable. */
   typedef void *VkInstance;
   /** Fallback opaque handle when Vulkan SDK headers are unavailable. */
   typedef void *VkSurfaceKHR;
#endif

#ifdef __cplusplus

namespace RootStream {

/**
 * @brief RAII wrapper for the X11 Vulkan surface backend.
 *
 * Typical usage:
 * @code
 *   Display *dpy = XOpenDisplay(nullptr);
 *   Window   win = XCreateSimpleWindow(...);
 *   X11VulkanSurface surf(dpy, win);
 *   VkSurfaceKHR vkSurf = VK_NULL_HANDLE;
 *   surf.createSurface(instance, &vkSurf);
 *   while (running) surf.processEvents();
 * @endcode
 */
class X11VulkanSurface {
public:
    /**
     * @brief Construct and initialise the X11 backend.
     * @param display  Open X11 display connection
     * @param window   Target X11 window
     */
    X11VulkanSurface(Display *display, Window window);

    /**
     * @brief Destructor – calls vulkan_x11_cleanup().
     */
    ~X11VulkanSurface();

    /* Non-copyable */
    X11VulkanSurface(const X11VulkanSurface &) = delete;
    X11VulkanSurface &operator=(const X11VulkanSurface &) = delete;

    /**
     * @brief Create a VkSurfaceKHR for the underlying X11 window.
     * @param instance  Initialised Vulkan instance
     * @param surface   Output surface handle
     * @return 0 on success, -1 on failure
     */
    int createSurface(VkInstance instance, VkSurfaceKHR *surface);

    /**
     * @brief Dispatch pending X11 events (call once per render loop iteration).
     * @return Number of events processed, or -1 on error
     */
    int processEvents();

    /**
     * @brief Return the X11 display connection passed at construction.
     */
    Display *getDisplay() const;

    /**
     * @brief Return the X11 window handle passed at construction.
     */
    Window getWindow() const;

private:
    Display *m_display; /**< X11 display connection (not owned) */
    Window   m_window;  /**< X11 window handle (not owned) */
    void    *m_ctx;     /**< Opaque vulkan_x11 context */
};

} /* namespace RootStream */

#endif /* __cplusplus */
#endif /* X11_VULKAN_SURFACE_H */
