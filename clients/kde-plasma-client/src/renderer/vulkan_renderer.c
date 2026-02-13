/**
 * @file vulkan_renderer.c
 * @brief Vulkan renderer implementation
 */

#include "vulkan_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform detection
#ifdef __linux__
#include <unistd.h>

// Vulkan headers (only if available)
#if __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#define HAVE_VULKAN_HEADERS 1
#endif

// Wayland headers (only if available)
#if __has_include(<wayland-client.h>)
#include <wayland-client.h>
#define HAVE_WAYLAND_HEADERS 1
#endif

// X11 headers (usually available)
#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#define HAVE_X11_HEADERS 1
#endif

#endif // __linux__

// Fallback definitions if Vulkan headers not available
#ifndef HAVE_VULKAN_HEADERS
typedef void* VkInstance;
typedef void* VkPhysicalDevice;
typedef void* VkDevice;
typedef void* VkQueue;
typedef void* VkSurfaceKHR;
typedef void* VkSwapchainKHR;
typedef void* VkImage;
typedef void* VkImageView;
typedef void* VkDeviceMemory;
typedef void* VkCommandPool;
typedef void* VkCommandBuffer;
typedef void* VkSemaphore;
typedef void* VkFence;
typedef uint32_t VkFormat;
typedef struct { uint32_t width, height; } VkExtent2D;
typedef uint32_t VkResult;
#define VK_NULL_HANDLE NULL
#define VK_SUCCESS 0
#endif

/**
 * Vulkan context structure
 */
struct vulkan_context_s {
    vulkan_backend_t backend;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
    
    // Backend-specific
    void *backend_context;  // Points to wayland/x11/headless context
    
    // Swapchain (for Wayland/X11)
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    uint32_t swapchain_image_count;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    
    // Rendering resources
    VkImage nv12_y_image;
    VkImage nv12_uv_image;
    VkDeviceMemory nv12_y_memory;
    VkDeviceMemory nv12_uv_memory;
    VkImageView nv12_y_view;
    VkImageView nv12_uv_view;
    
    // Command buffers
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    
    // Synchronization
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    
    // Configuration
    bool vsync_enabled;
    int width;
    int height;
    
    char last_error[256];
};

vulkan_backend_t vulkan_detect_backend(void) {
#ifdef HAVE_WAYLAND_HEADERS
    // Priority 1: Check for Wayland
    struct wl_display *wl_display = wl_display_connect(NULL);
    if (wl_display) {
        wl_display_disconnect(wl_display);
        return VULKAN_BACKEND_WAYLAND;
    }
#endif
    
#ifdef HAVE_X11_HEADERS
    // Priority 2: Check for X11
    Display *x11_display = XOpenDisplay(NULL);
    if (x11_display) {
        XCloseDisplay(x11_display);
        return VULKAN_BACKEND_X11;
    }
#endif
    
    // Priority 3: Fallback to headless
    return VULKAN_BACKEND_HEADLESS;
}

static int create_vulkan_instance(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "RootStream Client";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    // Required extensions based on backend
    const char *extensions[10];
    uint32_t extension_count = 0;
    
    extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
    
    switch (ctx->backend) {
        case VULKAN_BACKEND_WAYLAND:
            extensions[extension_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
            break;
        case VULKAN_BACKEND_X11:
            extensions[extension_count++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
            break;
        case VULKAN_BACKEND_HEADLESS:
            // No surface extension needed
            extension_count--;  // Remove surface extension
            break;
    }
    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledLayerCount = 0;
    
    VkResult result = vkCreateInstance(&create_info, NULL, &ctx->instance);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create Vulkan instance: %d", result);
        return -1;
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int select_physical_device(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, NULL);
    
    if (device_count == 0) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "No Vulkan-capable GPUs found");
        return -1;
    }
    
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);
    
    // Prefer discrete GPU
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            ctx->physical_device = devices[i];
            free(devices);
            return 0;
        }
    }
    
    // Fallback to first device
    ctx->physical_device = devices[0];
    free(devices);
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int find_queue_families(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, NULL);
    
    VkQueueFamilyProperties *queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, queue_families);
    
    ctx->graphics_queue_family = UINT32_MAX;
    ctx->present_queue_family = UINT32_MAX;
    
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            ctx->graphics_queue_family = i;
            
            // For headless, graphics queue is also present queue
            if (ctx->backend == VULKAN_BACKEND_HEADLESS) {
                ctx->present_queue_family = i;
                break;
            }
            
            // Check present support
            if (ctx->surface != VK_NULL_HANDLE) {
                VkBool32 present_support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, i, ctx->surface, &present_support);
                if (present_support) {
                    ctx->present_queue_family = i;
                    break;
                }
            }
        }
    }
    
    free(queue_families);
    
    if (ctx->graphics_queue_family == UINT32_MAX || ctx->present_queue_family == UINT32_MAX) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to find suitable queue families");
        return -1;
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int create_logical_device(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    float queue_priority = 1.0f;
    
    VkDeviceQueueCreateInfo queue_create_infos[2];
    uint32_t queue_create_info_count = 0;
    
    // Graphics queue
    queue_create_infos[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[queue_create_info_count].pNext = NULL;
    queue_create_infos[queue_create_info_count].flags = 0;
    queue_create_infos[queue_create_info_count].queueFamilyIndex = ctx->graphics_queue_family;
    queue_create_infos[queue_create_info_count].queueCount = 1;
    queue_create_infos[queue_create_info_count].pQueuePriorities = &queue_priority;
    queue_create_info_count++;
    
    if (ctx->present_queue_family != ctx->graphics_queue_family) {
        queue_create_infos[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[queue_create_info_count].pNext = NULL;
        queue_create_infos[queue_create_info_count].flags = 0;
        queue_create_infos[queue_create_info_count].queueFamilyIndex = ctx->present_queue_family;
        queue_create_infos[queue_create_info_count].queueCount = 1;
        queue_create_infos[queue_create_info_count].pQueuePriorities = &queue_priority;
        queue_create_info_count++;
    }
    
    // Device extensions
    const char *device_extensions[10];
    uint32_t device_extension_count = 0;
    
    if (ctx->backend != VULKAN_BACKEND_HEADLESS) {
        device_extensions[device_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }
    
    VkPhysicalDeviceFeatures device_features = {0};
    
    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_info_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = device_extension_count;
    create_info.ppEnabledExtensionNames = device_extensions;
    create_info.enabledLayerCount = 0;
    
    VkResult result = vkCreateDevice(ctx->physical_device, &create_info, NULL, &ctx->device);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create logical device: %d", result);
        return -1;
    }
    
    // Get queue handles
    vkGetDeviceQueue(ctx->device, ctx->graphics_queue_family, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->device, ctx->present_queue_family, 0, &ctx->present_queue);
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

vulkan_context_t* vulkan_init(void *native_window) {
    vulkan_context_t *ctx = calloc(1, sizeof(vulkan_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Detect backend
    ctx->backend = vulkan_detect_backend();
    ctx->vsync_enabled = true;
    ctx->width = 1920;  // Default, will be updated
    ctx->height = 1080;
    
    // Create Vulkan instance
    if (create_vulkan_instance(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create surface (for Wayland/X11 backends)
    // TODO: Implement backend-specific surface creation
    ctx->surface = VK_NULL_HANDLE;
    
    // Select physical device
    if (select_physical_device(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Find queue families
    if (find_queue_families(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create logical device
    if (create_logical_device(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // TODO: Create swapchain, command pool, etc.
    
    return ctx;
}

int vulkan_upload_frame(vulkan_context_t *ctx, const frame_t *frame) {
    if (!ctx || !frame) {
        return -1;
    }
    
    // TODO: Implement frame upload
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Frame upload not yet implemented");
    return -1;
}

int vulkan_render(vulkan_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // TODO: Implement rendering
    return 0;
}

int vulkan_present(vulkan_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // TODO: Implement present
    return 0;
}

int vulkan_set_vsync(vulkan_context_t *ctx, bool enabled) {
    if (!ctx) {
        return -1;
    }
    
    ctx->vsync_enabled = enabled;
    // TODO: Recreate swapchain with new present mode
    return 0;
}

int vulkan_resize(vulkan_context_t *ctx, int width, int height) {
    if (!ctx || width <= 0 || height <= 0) {
        return -1;
    }
    
    ctx->width = width;
    ctx->height = height;
    // TODO: Recreate swapchain
    return 0;
}

const char* vulkan_get_backend_name(vulkan_context_t *ctx) {
    if (!ctx) {
        return "unknown";
    }
    
    switch (ctx->backend) {
        case VULKAN_BACKEND_WAYLAND:
            return "wayland";
        case VULKAN_BACKEND_X11:
            return "x11";
        case VULKAN_BACKEND_HEADLESS:
            return "headless";
        default:
            return "unknown";
    }
}

void vulkan_cleanup(vulkan_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
#ifdef HAVE_VULKAN_HEADERS
    // Wait for device to be idle
    if (ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx->device);
    }
    
    // TODO: Clean up swapchain, images, etc.
    
    // Destroy device
    if (ctx->device != VK_NULL_HANDLE) {
        vkDestroyDevice(ctx->device, NULL);
    }
    
    // Destroy surface
    if (ctx->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    }
    
    // Destroy instance
    if (ctx->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(ctx->instance, NULL);
    }
#endif
    
    free(ctx);
}
