/**
 * @file proton_renderer.h
 * @brief Proton/Steam compatibility renderer for RootStream
 * 
 * Provides a renderer that can handle games running under Proton/Wine
 * with DXVK (D3D11) or VKD3D (D3D12) compatibility layers.
 */

#ifndef PROTON_RENDERER_H
#define PROTON_RENDERER_H

#include "renderer.h"
#include "proton_detector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque Proton renderer context
 */
typedef struct proton_context_s proton_context_t;

/**
 * Proton renderer configuration
 */
typedef struct {
    int width;
    int height;
    bool enable_dxvk;
    bool enable_vkd3d;
    bool enable_async_shader_compile;
    bool prefer_d3d11;  // If true, prefer D3D11 over D3D12
    int shader_cache_max_mb;
} proton_config_t;

/**
 * Initialize Proton renderer
 * 
 * Detects Proton environment and initializes appropriate backend
 * (DXVK for D3D11 or VKD3D for D3D12).
 * 
 * @param native_window Native window handle
 * @return Proton context, or NULL on failure
 */
proton_context_t* proton_init(void *native_window);

/**
 * Initialize with specific configuration
 * 
 * @param native_window Native window handle
 * @param config Configuration structure
 * @return Proton context, or NULL on failure
 */
proton_context_t* proton_init_with_config(void *native_window, const proton_config_t *config);

/**
 * Check if Proton is available
 * 
 * @return true if Proton environment detected, false otherwise
 */
bool proton_is_available(void);

/**
 * Get Proton information
 * 
 * @param ctx Proton context
 * @return Pointer to Proton info structure, or NULL if not initialized
 */
const proton_info_t* proton_get_info(proton_context_t *ctx);

/**
 * Upload frame data to GPU
 * 
 * @param ctx Proton context
 * @param frame Frame to upload
 * @return 0 on success, -1 on failure
 */
int proton_upload_frame(proton_context_t *ctx, const frame_t *frame);

/**
 * Render current frame
 * 
 * @param ctx Proton context
 * @return 0 on success, -1 on failure
 */
int proton_render(proton_context_t *ctx);

/**
 * Present rendered frame
 * 
 * @param ctx Proton context
 * @return 0 on success, -1 on failure
 */
int proton_present(proton_context_t *ctx);

/**
 * Enable or disable vsync
 * 
 * @param ctx Proton context
 * @param enabled True to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int proton_set_vsync(proton_context_t *ctx, bool enabled);

/**
 * Resize rendering surface
 * 
 * @param ctx Proton context
 * @param width New width
 * @param height New height
 * @return 0 on success, -1 on failure
 */
int proton_resize(proton_context_t *ctx, int width, int height);

/**
 * Get compatibility layer name
 * 
 * @param ctx Proton context
 * @return "dxvk", "vkd3d", or "unknown"
 */
const char* proton_get_compatibility_layer(proton_context_t *ctx);

/**
 * Check if running D3D11 game
 * 
 * @param ctx Proton context
 * @return true if D3D11 detected
 */
bool proton_is_d3d11_game(proton_context_t *ctx);

/**
 * Check if running D3D12 game
 * 
 * @param ctx Proton context
 * @return true if D3D12 detected
 */
bool proton_is_d3d12_game(proton_context_t *ctx);

/**
 * Get shader cache size in MB
 * 
 * @param ctx Proton context
 * @return Shader cache size, or 0 if unavailable
 */
uint32_t proton_get_shader_cache_size(proton_context_t *ctx);

/**
 * Get last error message
 * 
 * @param ctx Proton context
 * @return Error string, or NULL if no error
 */
const char* proton_get_error(proton_context_t *ctx);

/**
 * Clean up Proton renderer
 * 
 * @param ctx Proton context
 */
void proton_cleanup(proton_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PROTON_RENDERER_H */
