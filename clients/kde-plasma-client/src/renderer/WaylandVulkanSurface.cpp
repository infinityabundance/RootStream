/**
 * @file WaylandVulkanSurface.cpp
 * @brief Implementation of WaylandVulkanSurface
 *
 * Delegates to the C vulkan_wayland API.  All resource management is handled
 * inside the C layer; this wrapper simply holds the native handles and
 * forwards calls.
 */

#include "WaylandVulkanSurface.h"

namespace RootStream {

WaylandVulkanSurface::WaylandVulkanSurface(wl_display *display, wl_surface *surface)
    : m_display(display)
    , m_surface(surface)
    , m_ctx(nullptr)
{
    /* The Wayland C backend receives native_window as a void* that it may cast
     * to a wl_surface*.  Pass the surface pointer so the backend can bind it. */
    vulkan_wayland_init(&m_ctx, static_cast<void *>(surface));
}

WaylandVulkanSurface::~WaylandVulkanSurface()
{
    vulkan_wayland_cleanup(m_ctx);
    m_ctx = nullptr;
}

int WaylandVulkanSurface::createSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    return vulkan_wayland_create_surface(m_ctx,
                                         static_cast<void *>(instance),
                                         static_cast<void *>(surface));
}

int WaylandVulkanSurface::processEvents()
{
    return vulkan_wayland_process_events(m_ctx, nullptr, nullptr);
}

wl_display *WaylandVulkanSurface::getDisplay() const
{
    return m_display;
}

wl_surface *WaylandVulkanSurface::getSurface() const
{
    return m_surface;
}

} /* namespace RootStream */
