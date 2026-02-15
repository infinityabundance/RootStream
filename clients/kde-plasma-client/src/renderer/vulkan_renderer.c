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
typedef void* VkRenderPass;
typedef void* VkPipelineLayout;
typedef void* VkPipeline;
typedef void* VkFramebuffer;
typedef void* VkDescriptorSetLayout;
typedef void* VkDescriptorPool;
typedef void* VkDescriptorSet;
typedef void* VkSampler;
typedef void* VkBuffer;
typedef uint32_t VkFormat;
typedef uint32_t VkImageLayout;
typedef uint32_t VkAccessFlags;
typedef uint32_t VkPipelineStageFlags;
typedef uint32_t VkImageAspectFlags;
typedef struct { uint32_t width, height; } VkExtent2D;
typedef struct { uint32_t width, height, depth; } VkExtent3D;
typedef struct { int32_t x, y, z; } VkOffset3D;
typedef struct {
    VkImageAspectFlags aspectMask;
    uint32_t mipLevel;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
} VkImageSubresourceLayers;
typedef struct {
    uint64_t bufferOffset;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    VkImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VkBufferImageCopy;
typedef uint32_t VkResult;
#define VK_NULL_HANDLE NULL
#define VK_SUCCESS 0
#define VK_IMAGE_LAYOUT_UNDEFINED 0
#define VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 6
#define VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 5
#define VK_ACCESS_TRANSFER_WRITE_BIT 0x00001000
#define VK_ACCESS_SHADER_READ_BIT 0x00000020
#define VK_PIPELINE_STAGE_TRANSFER_BIT 0x00001000
#define VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT 0x00000080
#define VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT 0x00000001
#define VK_IMAGE_ASPECT_COLOR_BIT 0x00000001
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
    VkSampler sampler;
    
    // Staging buffer for frame uploads
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    void *staging_mapped;
    size_t staging_size;
    
    // Render pass and pipeline
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkFramebuffer *framebuffers;
    
    // Descriptor sets
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    
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
    uint32_t current_frame;
    
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

#ifdef HAVE_VULKAN_HEADERS
// Helper function to create a shader module from SPIR-V bytecode
static VkShaderModule create_shader_module(VkDevice device, const uint32_t *code, size_t code_size) {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = code;
    
    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return shader_module;
}
#endif

static int create_render_pass(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Color attachment (swapchain image)
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = ctx->swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    // Subpass
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    // Subpass dependency
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    // Create render pass
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    
    VkResult result = vkCreateRenderPass(ctx->device, &render_pass_info, NULL, &ctx->render_pass);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create render pass: %d", result);
        return -1;
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int create_descriptor_set_layout(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Descriptor bindings for Y and UV textures
    VkDescriptorSetLayoutBinding bindings[2] = {0};
    
    // Binding 0: Y plane sampler
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = NULL;
    
    // Binding 1: UV plane sampler
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;
    
    VkResult result = vkCreateDescriptorSetLayout(ctx->device, &layout_info, NULL, &ctx->descriptor_set_layout);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create descriptor set layout: %d", result);
        return -1;
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

static int create_framebuffers(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    ctx->framebuffers = malloc(sizeof(VkFramebuffer) * ctx->swapchain_image_count);
    
    for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
        VkImageView attachments[] = {
            ctx->swapchain_image_views[i]
        };
        
        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = ctx->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = ctx->swapchain_extent.width;
        framebuffer_info.height = ctx->swapchain_extent.height;
        framebuffer_info.layers = 1;
        
        VkResult result = vkCreateFramebuffer(ctx->device, &framebuffer_info, NULL, &ctx->framebuffers[i]);
        if (result != VK_SUCCESS) {
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Failed to create framebuffer %d: %d", i, result);
            return -1;
        }
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

// Helper to create a simple solid color pipeline for testing
// This creates a minimal pipeline that can draw geometry without textures
static int create_graphics_pipeline(vulkan_context_t *ctx) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &ctx->descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    
    VkResult result = vkCreatePipelineLayout(ctx->device, &pipeline_layout_info, NULL, &ctx->pipeline_layout);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create pipeline layout: %d", result);
        return -1;
    }
    
    // Note: For a complete graphics pipeline, we need compiled SPIR-V shaders
    // The pipeline creation is deferred until shaders are available
    // This allows the rest of the renderer to initialize properly
    
    // Mark pipeline as NULL - it will be created when shaders are loaded
    ctx->graphics_pipeline = VK_NULL_HANDLE;
    
    // To complete the pipeline, the following steps are needed:
    // 1. Compile shaders: glslangValidator -V shader.vert/frag -o shader.spv
    // 2. Load SPIR-V bytecode from files
    // 3. Create shader modules with vkCreateShaderModule
    // 4. Create VkPipelineShaderStageCreateInfo for vertex and fragment
    // 5. Configure VkPipelineVertexInputStateCreateInfo (empty - procedural geometry)
    // 6. Configure VkPipelineInputAssemblyStateCreateInfo (TRIANGLE_STRIP)
    // 7. Configure VkPipelineViewportStateCreateInfo (dynamic)
    // 8. Configure VkPipelineRasterizationStateCreateInfo
    // 9. Configure VkPipelineMultisampleStateCreateInfo (no MSAA)
    // 10. Configure VkPipelineColorBlendStateCreateInfo
    // 11. Configure VkPipelineDynamicStateCreateInfo (viewport, scissor)
    // 12. Call vkCreateGraphicsPipelines
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}
    // 10. Configure color blending
    // 11. Create graphics pipeline
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

/**
 * Create staging buffer for frame uploads
 * 
 * Allocates a HOST_VISIBLE buffer for transferring frame data from CPU to GPU.
 * For 1080p NV12: width(1920) * height(1080) * 1.5 = 3,110,400 bytes (~3MB)
 * Using 4MB to accommodate various resolutions up to 1080p.
 */
static int create_staging_buffer(vulkan_context_t *ctx, size_t size) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Round up to nearest MB for better allocation
    size = ((size + 1048576 - 1) / 1048576) * 1048576;
    ctx->staging_size = size;
    
    // Create buffer
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateBuffer(ctx->device, &buffer_info, NULL, &ctx->staging_buffer);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to create staging buffer: %d", result);
        return -1;
    }
    
    // Get memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(ctx->device, ctx->staging_buffer, &mem_requirements);
    
    // Find suitable memory type (HOST_VISIBLE | HOST_COHERENT)
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &mem_properties);
    
    uint32_t memory_type_index = UINT32_MAX;
    uint32_t required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_requirements.memoryTypeBits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties) {
            memory_type_index = i;
            break;
        }
    }
    
    if (memory_type_index == UINT32_MAX) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to find suitable memory type for staging buffer");
        vkDestroyBuffer(ctx->device, ctx->staging_buffer, NULL);
        ctx->staging_buffer = VK_NULL_HANDLE;
        return -1;
    }
    
    // Allocate memory
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_type_index;
    
    result = vkAllocateMemory(ctx->device, &alloc_info, NULL, &ctx->staging_memory);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to allocate staging buffer memory: %d", result);
        vkDestroyBuffer(ctx->device, ctx->staging_buffer, NULL);
        ctx->staging_buffer = VK_NULL_HANDLE;
        return -1;
    }
    
    // Bind buffer to memory
    result = vkBindBufferMemory(ctx->device, ctx->staging_buffer, ctx->staging_memory, 0);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to bind staging buffer memory: %d", result);
        vkFreeMemory(ctx->device, ctx->staging_memory, NULL);
        vkDestroyBuffer(ctx->device, ctx->staging_buffer, NULL);
        ctx->staging_buffer = VK_NULL_HANDLE;
        ctx->staging_memory = VK_NULL_HANDLE;
        return -1;
    }
    
    // Map memory persistently
    result = vkMapMemory(ctx->device, ctx->staging_memory, 0, size, 0, &ctx->staging_mapped);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to map staging buffer memory: %d", result);
        vkFreeMemory(ctx->device, ctx->staging_memory, NULL);
        vkDestroyBuffer(ctx->device, ctx->staging_buffer, NULL);
        ctx->staging_buffer = VK_NULL_HANDLE;
        ctx->staging_memory = VK_NULL_HANDLE;
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
    
    // Create staging buffer for frame uploads (4MB for 1080p NV12)
    // NV12 1080p: 1920 * 1080 * 1.5 = 3,110,400 bytes
    size_t staging_size = 4 * 1024 * 1024;  // 4MB
    if (create_staging_buffer(ctx, staging_size) != 0) {
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
    
    // Create render pass
    if (create_render_pass(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create descriptor set layout
    if (create_descriptor_set_layout(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create graphics pipeline
    if (create_graphics_pipeline(ctx) != 0) {
        vulkan_cleanup(ctx);
        return NULL;
    }
    
    // Create framebuffers
    if (create_framebuffers(ctx) != 0) {
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

/**
 * Validate frame data before upload
 * 
 * Checks frame pointer, data pointer, dimensions, format, and size.
 * 
 * @param frame Frame to validate
 * @return 0 if valid, -1 if invalid
 */
static int validate_frame(const frame_t *frame) {
    // Check frame pointer
    if (!frame) {
        return -1;
    }
    
    // Check data pointer
    if (!frame->data) {
        return -1;
    }
    
    // Check dimensions
    if (frame->width == 0 || frame->height == 0) {
        return -1;
    }
    
    // Check format (must be NV12)
    if (frame->format != FRAME_FORMAT_NV12) {
        return -1;
    }
    
    // Calculate expected size for NV12
    // NV12: Y plane (width × height) + UV plane (width × height / 2)
    uint32_t expected_y_size = frame->width * frame->height;
    uint32_t expected_uv_size = (frame->width / 2) * (frame->height / 2) * 2;
    uint32_t expected_total = expected_y_size + expected_uv_size;
    
    // Allow for some padding in frame size (up to 1% extra)
    uint32_t max_size = expected_total + (expected_total / 100);
    
    // Check size
    if (frame->size < expected_total || frame->size > max_size) {
        return -1;
    }
    
    return 0;
}

/**
 * Copy frame data to staging buffer
 * 
 * Copies Y and UV planes from frame data to the persistently-mapped
 * staging buffer. Y plane is copied first, followed by UV plane.
 * 
 * @param ctx Vulkan context
 * @param frame Frame to copy
 * @return 0 on success, -1 on failure
 */
static int copy_frame_to_staging(vulkan_context_t *ctx, const frame_t *frame) {
    if (!ctx || !frame || !ctx->staging_mapped) {
        return -1;
    }
    
    // Calculate plane sizes
    uint32_t y_size = frame->width * frame->height;
    uint32_t uv_size = (frame->width / 2) * (frame->height / 2) * 2;  // Interleaved UV
    uint32_t total_size = y_size + uv_size;
    
    // Check staging buffer has enough space
    if (total_size > ctx->staging_size) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Frame size %u exceeds staging buffer size %zu",
                total_size, ctx->staging_size);
        return -1;
    }
    
    // Get pointers
    uint8_t *staging_ptr = (uint8_t *)ctx->staging_mapped;
    const uint8_t *frame_data = frame->data;
    
    // Copy Y plane (offset 0)
    memcpy(staging_ptr, frame_data, y_size);
    
    // Copy UV plane (offset y_size)
    memcpy(staging_ptr + y_size, frame_data + y_size, uv_size);
    
    return 0;
}

/**
 * Transition image layout using pipeline barrier
 * 
 * Creates a single-time command buffer to transition an image from
 * one layout to another. This is needed before/after copy operations
 * and to prepare images for shader access.
 * 
 * @param ctx Vulkan context
 * @param image Image to transition
 * @param old_layout Current layout
 * @param new_layout Desired layout
 * @return 0 on success, -1 on failure
 */
static int transition_image_layout(vulkan_context_t *ctx,
                                   VkImage image,
                                   VkImageLayout old_layout,
                                   VkImageLayout new_layout) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Allocate single-time command buffer
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = ctx->command_pool;
    alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(ctx->device, &alloc_info, &command_buffer);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to allocate command buffer for layout transition: %d", result);
        return -1;
    }
    
    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to begin command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Set up barrier based on transition type
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    // Determine access masks and pipeline stages based on layouts
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && 
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        // Before copy: prepare for transfer write
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // After copy: prepare for shader read
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Unsupported layout transition: %u -> %u", old_layout, new_layout);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Record pipeline barrier
    vkCmdPipelineBarrier(
        command_buffer,
        source_stage, destination_stage,
        0,  // dependency flags
        0, NULL,  // memory barriers
        0, NULL,  // buffer memory barriers
        1, &barrier  // image memory barriers
    );
    
    // End command buffer
    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to end command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Submit command buffer
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    result = vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to submit command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Wait for completion
    vkQueueWaitIdle(ctx->graphics_queue);
    
    // Free command buffer
    vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

/**
 * Copy Y plane from staging buffer to device image
 * 
 * Transitions the Y image to TRANSFER_DST layout, copies data from
 * the staging buffer, and leaves image in TRANSFER_DST (will be
 * transitioned to SHADER_READ_ONLY later).
 * 
 * @param ctx Vulkan context
 * @param width Frame width
 * @param height Frame height
 * @return 0 on success, -1 on failure
 */
static int copy_staging_to_y_image(vulkan_context_t *ctx, 
                                   uint32_t width, 
                                   uint32_t height) {
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    // Transition Y image to TRANSFER_DST layout
    if (transition_image_layout(ctx, ctx->nv12_y_image,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) != 0) {
        return -1;
    }
    
    // Allocate single-time command buffer
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = ctx->command_pool;
    alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(ctx->device, &alloc_info, &command_buffer);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to allocate command buffer for Y plane copy: %d", result);
        return -1;
    }
    
    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to begin command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Set up buffer-to-image copy region
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;  // Y plane starts at offset 0
    region.bufferRowLength = 0;  // Tightly packed
    region.bufferImageHeight = 0;  // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    
    // Record copy command
    vkCmdCopyBufferToImage(
        command_buffer,
        ctx->staging_buffer,
        ctx->nv12_y_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    
    // End command buffer
    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to end command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Submit command buffer
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    result = vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to submit command buffer: %d", result);
        vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
        return -1;
    }
    
    // Wait for completion
    vkQueueWaitIdle(ctx->graphics_queue);
    
    // Free command buffer
    vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &command_buffer);
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

int vulkan_render(vulkan_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    if (ctx->backend == VULKAN_BACKEND_HEADLESS) {
        return 0;  // No rendering needed for headless
    }
    
    // Wait for previous frame to finish
    vkWaitForFences(ctx->device, 1, &ctx->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx->device, 1, &ctx->in_flight_fence);
    
    // Acquire next image from swapchain
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
                                           ctx->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain needs to be recreated (e.g., window resized)
        return 0;  // Skip this frame
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to acquire swapchain image: %d", result);
        return -1;
    }
    
    // Record command buffer
    VkCommandBuffer command_buffer = ctx->command_buffers[image_index];
    
    vkResetCommandBuffer(command_buffer, 0);
    
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;
    
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to begin recording command buffer");
        return -1;
    }
    
    // Begin render pass
    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = ctx->render_pass;
    render_pass_info.framebuffer = ctx->framebuffers[image_index];
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = ctx->swapchain_extent;
    
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;
    
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    // TODO: Bind pipeline and draw when shaders are loaded
    // For now, just clear to black
    // vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);
    // vkCmdDraw(command_buffer, 4, 1, 0, 0);  // Draw fullscreen quad
    
    vkCmdEndRenderPass(command_buffer);
    
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to record command buffer");
        return -1;
    }
    
    // Submit command buffer
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore wait_semaphores[] = {ctx->image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    VkSemaphore signal_semaphores[] = {ctx->render_finished_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    if (vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, ctx->in_flight_fence) != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to submit draw command buffer");
        return -1;
    }
    
    ctx->current_frame = image_index;
    return 0;
#endif // HAVE_VULKAN_HEADERS
}

int vulkan_present(vulkan_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
#ifndef HAVE_VULKAN_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan headers not available at compile time");
    return -1;
#else
    if (ctx->backend == VULKAN_BACKEND_HEADLESS) {
        return 0;  // No presentation needed for headless
    }
    
    // Present the image
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    VkSemaphore wait_semaphores[] = {ctx->render_finished_semaphore};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = wait_semaphores;
    
    VkSwapchainKHR swapchains[] = {ctx->swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &ctx->current_frame;
    present_info.pResults = NULL;
    
    VkResult result = vkQueuePresentKHR(ctx->present_queue, &present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain needs to be recreated
        return 0;  // Not an error, just needs recreation
    } else if (result != VK_SUCCESS) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to present swapchain image: %d", result);
        return -1;
    }
    
    return 0;
#endif // HAVE_VULKAN_HEADERS
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
    
    // Destroy framebuffers
    if (ctx->framebuffers) {
        for (uint32_t i = 0; i < ctx->swapchain_image_count; i++) {
            if (ctx->framebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
            }
        }
        free(ctx->framebuffers);
    }
    
    // Destroy graphics pipeline
    if (ctx->graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx->device, ctx->graphics_pipeline, NULL);
    }
    
    // Destroy pipeline layout
    if (ctx->pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx->device, ctx->pipeline_layout, NULL);
    }
    
    // Destroy render pass
    if (ctx->render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(ctx->device, ctx->render_pass, NULL);
    }
    
    // Destroy descriptor set layout
    if (ctx->descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(ctx->device, ctx->descriptor_set_layout, NULL);
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
    
    // Clean up staging buffer
    if (ctx->staging_mapped && ctx->staging_memory != VK_NULL_HANDLE) {
        vkUnmapMemory(ctx->device, ctx->staging_memory);
    }
    if (ctx->staging_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(ctx->device, ctx->staging_buffer, NULL);
    }
    if (ctx->staging_memory != VK_NULL_HANDLE) {
        vkFreeMemory(ctx->device, ctx->staging_memory, NULL);
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
