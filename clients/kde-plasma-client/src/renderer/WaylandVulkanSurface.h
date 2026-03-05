/**
 * @file WaylandVulkanSurface.h
 * @brief C++ wrapper around the C vulkan_wayland backend
 *
 * Provides an RAII class that owns the Wayland/Vulkan surface lifetime and
 * delegates to the underlying C API (vulkan_wayland_init,
 * vulkan_wayland_create_surface, vulkan_wayland_process_events,
 * vulkan_wayland_cleanup).
 *
 * The header compiles without errors even when Wayland or Vulkan SDK headers
 * are absent; opaque pointer fallbacks are supplied via __has_include guards.
 */

#ifndef WAYLAND_VULKAN_SURFACE_H
#define WAYLAND_VULKAN_SURFACE_H

#include "vulkan_wayland.h"

/* ── Wayland type availability ───────────────────────────────────────────── */
#if __has_include(<wayland-client.h>)
#  include <wayland-client.h>
#else
   /** Fallback opaque type when Wayland headers are unavailable. */
   struct wl_display;
   /** Fallback opaque type when Wayland headers are unavailable. */
   struct wl_surface;
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
 * @brief RAII wrapper for the Wayland Vulkan surface backend.
 *
 * Typical usage:
 * @code
 *   wl_display *dpy = wl_display_connect(nullptr);
 *   wl_surface *srf = wl_compositor_create_surface(compositor);
 *   WaylandVulkanSurface surf(dpy, srf);
 *   VkSurfaceKHR vkSurf = VK_NULL_HANDLE;
 *   surf.createSurface(instance, &vkSurf);
 *   while (running) surf.processEvents();
 * @endcode
 */
class WaylandVulkanSurface {
public:
    /**
     * @brief Construct and initialise the Wayland backend.
     * @param display  Connected Wayland display
     * @param surface  Wayland compositor surface
     */
    WaylandVulkanSurface(wl_display *display, wl_surface *surface);

    /**
     * @brief Destructor – calls vulkan_wayland_cleanup().
     */
    ~WaylandVulkanSurface();

    /* Non-copyable */
    WaylandVulkanSurface(const WaylandVulkanSurface &) = delete;
    WaylandVulkanSurface &operator=(const WaylandVulkanSurface &) = delete;

    /**
     * @brief Create a VkSurfaceKHR for the underlying Wayland surface.
     * @param instance  Initialised Vulkan instance
     * @param surface   Output surface handle
     * @return 0 on success, -1 on failure
     */
    int createSurface(VkInstance instance, VkSurfaceKHR *surface);

    /**
     * @brief Dispatch pending Wayland events (call once per render loop iteration).
     * @return Number of events processed, or -1 on error
     */
    int processEvents();

    /**
     * @brief Return the Wayland display connection passed at construction.
     */
    wl_display *getDisplay() const;

    /**
     * @brief Return the Wayland surface handle passed at construction.
     */
    wl_surface *getSurface() const;

private:
    wl_display *m_display; /**< Wayland display connection (not owned) */
    wl_surface *m_surface; /**< Wayland surface handle (not owned) */
    void       *m_ctx;     /**< Opaque vulkan_wayland context */
};

} /* namespace RootStream */

#endif /* __cplusplus */
#endif /* WAYLAND_VULKAN_SURFACE_H */
