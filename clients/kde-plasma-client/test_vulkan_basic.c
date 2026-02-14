/**
 * @file test_vulkan_basic.c
 * @brief Basic test for Vulkan renderer initialization
 */

#include "src/renderer/vulkan_renderer.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("RootStream Vulkan Renderer Test\n");
    printf("================================\n\n");
    
    // Detect backend
    vulkan_backend_t backend = vulkan_detect_backend();
    printf("Detected backend: ");
    switch (backend) {
        case VULKAN_BACKEND_WAYLAND:
            printf("Wayland\n");
            break;
        case VULKAN_BACKEND_X11:
            printf("X11\n");
            break;
        case VULKAN_BACKEND_HEADLESS:
            printf("Headless\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
    
    // Try to initialize Vulkan
    printf("\nInitializing Vulkan renderer...\n");
    vulkan_context_t *ctx = vulkan_init(NULL);
    
    if (ctx) {
        printf("✓ Vulkan initialization successful!\n");
        printf("  Backend: %s\n", vulkan_get_backend_name(ctx));
        
        // Clean up
        vulkan_cleanup(ctx);
        printf("\n✓ Cleanup successful\n");
        return 0;
    } else {
        printf("✗ Vulkan initialization failed\n");
        return 1;
    }
}
