/**
 * @file vulkan_headless.h
 * @brief Vulkan headless backend for CI/testing without display server
 */

#ifndef VULKAN_HEADLESS_H
#define VULKAN_HEADLESS_H

#include "vulkan_renderer.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Headless-specific context
 */
typedef struct vulkan_headless_context_s vulkan_headless_context_t;

/**
 * Initialize headless backend
 * 
 * @param ctx Vulkan context
 * @return 0 on success, -1 on failure
 */
int vulkan_headless_init(void *ctx);

/**
 * Readback frame from GPU memory
 * 
 * For testing purposes - reads rendered frame to CPU memory.
 * 
 * @param ctx Headless context
 * @param out_data Output buffer
 * @param max_size Maximum buffer size
 * @return Number of bytes written, or -1 on failure
 */
int vulkan_headless_readback_frame(void *ctx, uint8_t *out_data, size_t max_size);

/**
 * Cleanup headless backend
 * 
 * @param ctx Headless context
 */
void vulkan_headless_cleanup(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VULKAN_HEADLESS_H */
