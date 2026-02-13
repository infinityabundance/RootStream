/**
 * @file vkd3d_interop.h
 * @brief VKD3D interoperability layer for D3D12 games
 * 
 * Provides interface to VKD3D (DirectX 12 over Vulkan) for
 * frame capture and resource sharing.
 */

#ifndef VKD3D_INTEROP_H
#define VKD3D_INTEROP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque VKD3D context handle
 */
typedef struct vkd3d_context_s vkd3d_context_t;

/**
 * Shader compilation statistics
 */
typedef struct {
    uint32_t total_shaders;
    uint32_t compiled_shaders;
    uint32_t compilation_time_ms;
} vkd3d_shader_stats_t;

/**
 * Initialize VKD3D interop from Proton environment
 * 
 * @return VKD3D context handle, or NULL on failure
 */
vkd3d_context_t* vkd3d_init_from_env(void);

/**
 * Query VKD3D version
 * 
 * @param context VKD3D context
 * @param version Output buffer for version string
 * @param max_len Maximum buffer length
 * @return 0 on success, -1 on failure
 */
int vkd3d_query_version(vkd3d_context_t *context, char *version, size_t max_len);

/**
 * Enable shader debug mode
 * 
 * @param context VKD3D context
 * @return 0 on success, -1 on failure
 */
int vkd3d_enable_shader_debug(vkd3d_context_t *context);

/**
 * Query shader compilation statistics
 * 
 * @param context VKD3D context
 * @param stats Output structure for statistics
 * @return 0 on success, -1 on failure
 */
int vkd3d_query_shader_stats(vkd3d_context_t *context, vkd3d_shader_stats_t *stats);

/**
 * Wait for GPU to become idle
 * 
 * @param context VKD3D context
 * @return 0 on success, -1 on failure
 */
int vkd3d_wait_gpu_idle(vkd3d_context_t *context);

/**
 * Get GPU wait time in milliseconds
 * 
 * @param context VKD3D context
 * @param wait_ms Output variable for wait time
 * @return 0 on success, -1 on failure
 */
int vkd3d_get_gpu_wait_time(vkd3d_context_t *context, uint32_t *wait_ms);

/**
 * Clean up VKD3D context
 * 
 * @param context VKD3D context
 */
void vkd3d_cleanup(vkd3d_context_t *context);

#ifdef __cplusplus
}
#endif

#endif /* VKD3D_INTEROP_H */
