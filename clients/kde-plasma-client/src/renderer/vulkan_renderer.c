/**
 * @file vulkan_renderer.c
 * @brief Vulkan renderer implementation
 */

#include "vulkan_renderer.h"
#include "vulkan_x11.h"
#include "vulkan_wayland.h"
#include "vulkan_headless.h"
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

static int create_swapchain(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    if (ctx->backend == VULKAN_BACKEND_HEADLESS) {
        return 0;  // No swapchain needed for headless
    }
    
    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->surface, &capabilities);
    
    // Query surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, NULL);
    if (format_count == 0) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "No surface formats available");
        return -1;
    }
    
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, formats);
    
    // Select best format (prefer B8G8R8A8_SRGB)
    VkSurfaceFormatKHR selected_format = formats[0];
    for (uint32_t i = 0; i < format_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selected_format = formats[i];
            break;
        }
    }
    free(formats);
    
    // Query present modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->surface, &present_mode_count, NULL);
    VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physical_device, ctx->surface, &present_mode_count, present_modes);
    
    // Select present mode (prefer MAILBOX for low latency, fallback to FIFO)
    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;  // Always available
    if (!ctx->vsync_enabled) {
        for (uint32_t i = 0; i < present_mode_count; i++) {
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                selected_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
    free(present_modes);
    
    // Set swapchain extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = ctx->width;
        extent.height = ctx->height;
        
        if (extent.width < capabilities.minImageExtent.width) {
            extent.width = capabilities.minImageExtent.width;
        } else if (extent.width > capabilities.maxImageExtent.width) {
            extent.width = capabilities.maxImageExtent.width;
        }
        
        if (extent.height < capabilities.minImageExtent.height) {
            extent.height = capabilities.minImageExtent.height;
        } else if (extent.height > capabilities.maxImageExtent.height) {
            extent.height = capabilities.maxImageExtent.height;
        }
    }
    
    // Image count (request one more than minimum for triple buffering)
    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }
    
    // Create swapchain
    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = ctx->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = selected_format.format;
    create_info.imageColorSpace = selected_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queue_family_indices[] = {ctx->graphics_queue_family, ctx->present_queue_family};
    if (ctx->graphics_queue_family != ctx->present_queue_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }
    
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = selected_present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    VkResult result = vkCreateSwapchainKHR(ctx->device, &create_info, NULL, &ctx->swapchain);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create swapchain: %d", result);
        return -1;
    }
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, NULL);
    ctx->swapchain_images = malloc(sizeof(VkImage) * ctx->swapchain_image_count);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, ctx->swapchain_images);
    
    ctx->swapchain_format = selected_format.format;
    ctx->swapchain_extent = extent;
    
    // Create image views
    ctx->swapchain_image_views = malloc(sizeof(VkImageView) * ctx->swapchain_image_count);
    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = ctx->swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = ctx->swapchain_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        result = vkCreateImageView(ctx->device, &view_info, NULL, &ctx->swapchain_image_views[i]);
        if (result != VK_SUCCESS) {
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Failed to create image view %d: %d", i, result);
            return -1;
        }
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int create_command_pool(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = ctx->graphics_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkResult result = vkCreateCommandPool(ctx->device, &pool_info, NULL, &ctx->command_pool);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create command pool: %d", result);
        return -1;
    }
    
    // Allocate command buffers (one per swapchain image)
    if (ctx->swapchain_image_count > 0) {
        ctx->command_buffers = malloc(sizeof(VkCommandBuffer) * ctx->swapchain_image_count);
        
        VkCommandBufferAllocateInfo alloc_info = {0};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = ctx->command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = ctx->swapchain_image_count;
        
        result = vkAllocateCommandBuffers(ctx->device, &alloc_info, ctx->command_buffers);
        if (result != VK_SUCCESS) {
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Failed to allocate command buffers: %d", result);
            return -1;
        }
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int create_sync_objects(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled
    
    VkResult result;
    
    result = vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->image_available_semaphore);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create image available semaphore: %d", result);
        return -1;
    }
    
    result = vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->render_finished_semaphore);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create render finished semaphore: %d", result);
        return -1;
    }
    
    result = vkCreateFence(ctx->device, &fence_info, NULL, &ctx->in_flight_fence);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create in-flight fence: %d", result);
        return -1;
    }
    
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
    ctx->width = DEFAULT_RENDER_WIDTH;
    ctx->height = DEFAULT_RENDER_HEIGHT;
    
    // Create Vulkan instance
    if (create_vulkan_instance(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create backend-specific surface
    int surface_result = -1;
    switch (ctx->backend) {
        case VULKAN_BACKEND_X11:
            // Initialize X11 backend
            if (vulkan_x11_init(&ctx->backend_context, native_window) == 0) {
                // Create X11 surface
                surface_result = vulkan_x11_create_surface(
                    ctx->backend_context,
                    ctx->instance,
                    &ctx->surface
                );
            }
            break;
            
        case VULKAN_BACKEND_WAYLAND:
            // Initialize Wayland backend (TODO: implement)
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Wayland backend not yet implemented");
            break;
            
        case VULKAN_BACKEND_HEADLESS:
            // Headless doesn't need a surface
            surface_result = 0;
            break;
    }
    
    if (surface_result != 0 && ctx->backend != VULKAN_BACKEND_HEADLESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create %s surface", vulkan_get_backend_name(ctx));
        vulkan_cleanup(ctx);
        return NULL;
    }
    
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
    
    // Create swapchain
    if (create_swapchain(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create command pool and buffers
    if (create_command_pool(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create synchronization objects
    if (create_sync_objects(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
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
    
    // Destroy synchronization objects
    if (ctx->in_flight_fence != VK_NULL_HANDLE) {
        vkDestroyFence(ctx->device, ctx->in_flight_fence, NULL);
    }
    if (ctx->render_finished_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx->device, ctx->render_finished_semaphore, NULL);
    }
    if (ctx->image_available_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx->device, ctx->image_available_semaphore, NULL);
    }
    
    // Free command buffers
    if (ctx->command_buffers && ctx->command_pool != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, ctx->swapchain_image_count, ctx->command_buffers);
        free(ctx->command_buffers);
    }
    
    // Destroy command pool
    if (ctx->command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(ctx->device, ctx->command_pool, NULL);
    }
    
    // Destroy swapchain image views
    if (ctx->swapchain_image_views) {
        for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
            if (ctx->swapchain_image_views[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(ctx->device, ctx->swapchain_image_views[i], NULL);
            }
        }
        free(ctx->swapchain_image_views);
    }
    
    // Free swapchain images array (images themselves are owned by swapchain)
    if (ctx->swapchain_images) {
        free(ctx->swapchain_images);
    }
    
    // Destroy swapchain
    if (ctx->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
    }
    
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
    
    // Clean up backend-specific context
    if (ctx->backend_context) {
        switch (ctx->backend) {
            case VULKAN_BACKEND_X11:
                vulkan_x11_cleanup(ctx->backend_context);
                break;
            case VULKAN_BACKEND_WAYLAND:
                // TODO: vulkan_wayland_cleanup(ctx->backend_context);
                break;
            case VULKAN_BACKEND_HEADLESS:
                // TODO: vulkan_headless_cleanup(ctx->backend_context);
                break;
        }
    }
    
    free(ctx);
}
