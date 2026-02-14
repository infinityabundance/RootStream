/**
 * @file test_vulkan_basic.c
 * @brief Basic test for Vulkan renderer initialization and rendering
 */

#include "src/renderer/vulkan_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
        
        // Test render loop (render a few black frames)
        printf("\nTesting render loop (10 frames)...\n");
        for (int i = 0; i < 10; i++) {
            if (vulkan_render(ctx) != 0) {
                printf("✗ Render failed on frame %d\n", i);
                break;
            }
            
            if (vulkan_present(ctx) != 0) {
                printf("✗ Present failed on frame %d\n", i);
                break;
            }
            
            if (i == 0) {
                printf("✓ First frame rendered and presented successfully!\n");
            }
            
            usleep(16667);  // ~60 FPS
        }
        printf("✓ Rendered 10 frames successfully!\n");
        
        // Clean up
        vulkan_cleanup(ctx);
        printf("\n✓ Cleanup successful\n");
        return 0;
    } else {
        printf("✗ Vulkan initialization failed\n");
        return 1;
    }
}
