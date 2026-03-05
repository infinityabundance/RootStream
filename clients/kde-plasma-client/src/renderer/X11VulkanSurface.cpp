/**
 * @file X11VulkanSurface.cpp
 * @brief Implementation of X11VulkanSurface
 *
 * Delegates to the C vulkan_x11 API.  All resource management is handled
 * inside the C layer; this wrapper simply holds the native handles and
 * forwards calls.
 */

#include "X11VulkanSurface.h"

namespace RootStream {

X11VulkanSurface::X11VulkanSurface(Display *display, Window window)
    : m_display(display)
    , m_window(window)
    , m_ctx(nullptr)
{
    /* Pack both handles into a small struct that vulkan_x11_init can use.
     * The x11 C backend accepts a native_window pointer which, by convention
     * in this codebase, may be NULL (the backend creates its own window) or
     * point to caller-owned native state.  We pass the Window value directly
     * reinterpret-cast to void* to stay ABI-compatible with the C layer. */
    vulkan_x11_init(&m_ctx, reinterpret_cast<void *>(static_cast<uintptr_t>(m_window)));
}

X11VulkanSurface::~X11VulkanSurface()
{
    vulkan_x11_cleanup(m_ctx);
    m_ctx = nullptr;
}

int X11VulkanSurface::createSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    return vulkan_x11_create_surface(m_ctx,
                                     static_cast<void *>(instance),
                                     static_cast<void *>(surface));
}

int X11VulkanSurface::processEvents()
{
    return vulkan_x11_process_events(m_ctx, nullptr, nullptr);
}

Display *X11VulkanSurface::getDisplay() const
{
    return m_display;
}

Window X11VulkanSurface::getWindow() const
{
    return m_window;
}

} /* namespace RootStream */
