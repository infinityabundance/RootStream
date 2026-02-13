/**
 * @file dxvk_interop.h
 * @brief DXVK interoperability layer for D3D11 games
 * 
 * Provides interface to DXVK (DirectX 11 over Vulkan) for
 * frame capture and resource sharing.
 */

#ifndef DXVK_INTEROP_H
#define DXVK_INTEROP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque DXVK adapter handle
 */
typedef struct dxvk_adapter_s dxvk_adapter_t;

/**
 * Shader cache statistics
 */
typedef struct {
    uint32_t total_shaders;
    uint32_t cached_shaders;
    uint32_t compiled_shaders;
    uint32_t cache_size_mb;
    uint32_t compilation_time_ms;
} dxvk_shader_stats_t;

/**
 * Initialize DXVK interop from Proton environment
 * 
 * @return DXVK adapter handle, or NULL on failure
 */
dxvk_adapter_t* dxvk_init_from_env(void);

/**
 * Query DXVK version
 * 
 * @param adapter DXVK adapter
 * @param version Output buffer for version string
 * @param max_len Maximum buffer length
 * @return 0 on success, -1 on failure
 */
int dxvk_query_version(dxvk_adapter_t *adapter, char *version, size_t max_len);

/**
 * Enable async shader compilation
 * 
 * @param adapter DXVK adapter
 * @return 0 on success, -1 on failure
 */
int dxvk_enable_async_compilation(dxvk_adapter_t *adapter);

/**
 * Query shader cache statistics
 * 
 * @param adapter DXVK adapter
 * @param stats Output structure for statistics
 * @return 0 on success, -1 on failure
 */
int dxvk_query_shader_stats(dxvk_adapter_t *adapter, dxvk_shader_stats_t *stats);

/**
 * Clear shader cache
 * 
 * @param adapter DXVK adapter
 * @return 0 on success, -1 on failure
 */
int dxvk_clear_shader_cache(dxvk_adapter_t *adapter);

/**
 * Get GPU utilization percentage
 * 
 * @param adapter DXVK adapter
 * @param percent Output variable for utilization (0.0-100.0)
 * @return 0 on success, -1 on failure
 */
int dxvk_get_gpu_utilization(dxvk_adapter_t *adapter, float *percent);

/**
 * Clean up DXVK adapter
 * 
 * @param adapter DXVK adapter
 */
void dxvk_cleanup(dxvk_adapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif /* DXVK_INTEROP_H */
