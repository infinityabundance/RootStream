/**
 * @file proton_renderer.c
 * @brief Proton renderer implementation
 */

#include "proton_renderer.h"
#include "vulkan_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

/**
 * Proton renderer context
 */
struct proton_context_s {
    proton_info_t info;
    proton_config_t config;
    void *backend_context;  // Vulkan context (since DXVK/VKD3D use Vulkan)
    char last_error[256];
};

bool proton_is_available(void) {
    proton_info_t info;
    return proton_detect(&info);
}

proton_context_t* proton_init_with_config(void *native_window, const proton_config_t *config) {
    proton_context_t *ctx = (proton_context_t*)calloc(1, sizeof(proton_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Detect Proton environment
    if (!proton_detect(&ctx->info)) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Not running under Proton/Wine");
        free(ctx);
        return NULL;
    }
    
    // Apply configuration
    if (config) {
        ctx->config = *config;
    } else {
        // Default configuration
        ctx->config.width = DEFAULT_RENDER_WIDTH;
        ctx->config.height = DEFAULT_RENDER_HEIGHT;
        ctx->config.enable_dxvk = true;
        ctx->config.enable_vkd3d = true;
        ctx->config.enable_async_shader_compile = true;
        ctx->config.prefer_d3d11 = true;
        ctx->config.shader_cache_max_mb = 1024;
    }
    
    // DXVK and VKD3D both use Vulkan underneath
    // We can use the Vulkan renderer as our backend
#ifdef HAVE_VULKAN_RENDERER
    ctx->backend_context = vulkan_init(native_window);
    if (!ctx->backend_context) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to initialize Vulkan backend for Proton");
        free(ctx);
        return NULL;
    }
#else
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Vulkan renderer not compiled in (required for Proton)");
    free(ctx);
    return NULL;
#endif
    
    // Enable async shader compilation if DXVK is available
    if (ctx->info.has_dxvk && ctx->config.enable_async_shader_compile) {
        setenv("DXVK_ASYNC", "1", 1);
    }
    
    return ctx;
}

proton_context_t* proton_init(void *native_window) {
    return proton_init_with_config(native_window, NULL);
}

const proton_info_t* proton_get_info(proton_context_t *ctx) {
    if (!ctx) {
        return NULL;
    }
    return &ctx->info;
}

int proton_upload_frame(proton_context_t *ctx, const frame_t *frame) {
    if (!ctx || !frame) {
        return -1;
    }
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        return vulkan_upload_frame((vulkan_context_t*)ctx->backend_context, frame);
    }
#endif
    
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "No backend context available");
    return -1;
}

int proton_render(proton_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        return vulkan_render((vulkan_context_t*)ctx->backend_context);
    }
#endif
    
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "No backend context available");
    return -1;
}

int proton_present(proton_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        return vulkan_present((vulkan_context_t*)ctx->backend_context);
    }
#endif
    
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "No backend context available");
    return -1;
}

int proton_set_vsync(proton_context_t *ctx, bool enabled) {
    if (!ctx) {
        return -1;
    }
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        return vulkan_set_vsync((vulkan_context_t*)ctx->backend_context, enabled);
    }
#endif
    
    return -1;
}

int proton_resize(proton_context_t *ctx, int width, int height) {
    if (!ctx || width <= 0 || height <= 0) {
        return -1;
    }
    
    ctx->config.width = width;
    ctx->config.height = height;
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        return vulkan_resize((vulkan_context_t*)ctx->backend_context, width, height);
    }
#endif
    
    return -1;
}

const char* proton_get_compatibility_layer(proton_context_t *ctx) {
    if (!ctx) {
        return "unknown";
    }
    
    if (ctx->info.has_dxvk) {
        return "dxvk";
    } else if (ctx->info.has_vkd3d) {
        return "vkd3d";
    }
    
    return "unknown";
}

bool proton_is_d3d11_game(proton_context_t *ctx) {
    return ctx && ctx->info.has_d3d11;
}

bool proton_is_d3d12_game(proton_context_t *ctx) {
    return ctx && ctx->info.has_d3d12;
}

uint32_t proton_get_shader_cache_size(proton_context_t *ctx) {
    if (!ctx) {
        return 0;
    }
    
    // Try to get DXVK shader cache size
    if (ctx->info.has_dxvk) {
        // Cache location: ~/.cache/dxvk-cache/
        const char *home = getenv("HOME");
        if (home) {
            char cache_path[PATH_MAX];
            int written = snprintf(cache_path, sizeof(cache_path), "%s/.cache/dxvk-cache", home);
            
            if (written < 0 || written >= (int)sizeof(cache_path)) {
                return 0;  // Path too long
            }
            
            // Count total size of files in cache directory
            DIR *dir = opendir(cache_path);
            if (dir) {
                struct dirent *entry;
                uint64_t total_bytes = 0;
                
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_REG) {
                        size_t cache_len = strlen(cache_path);
                        size_t name_len = strlen(entry->d_name);
                        
                        // Check if path would fit (with / and null terminator)
                        if (cache_len + name_len + 2 > PATH_MAX) {
                            continue;  // Skip files with too long names
                        }
                        
                        char file_path[PATH_MAX];
                        // We already checked the length above, safe to ignore truncation warning
                        #pragma GCC diagnostic push
                        #pragma GCC diagnostic ignored "-Wformat-truncation"
                        snprintf(file_path, sizeof(file_path), "%s/%s", 
                                cache_path, entry->d_name);
                        #pragma GCC diagnostic pop
                        
                        struct stat st;
                        if (stat(file_path, &st) == 0) {
                            total_bytes += st.st_size;
                        }
                    }
                }
                
                closedir(dir);
                return (uint32_t)(total_bytes / (1024 * 1024)); // Convert to MB
            }
        }
    }
    
    return 0;
}

const char* proton_get_error(proton_context_t *ctx) {
    if (!ctx || ctx->last_error[0] == '\0') {
        return NULL;
    }
    return ctx->last_error;
}

void proton_cleanup(proton_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
#ifdef HAVE_VULKAN_RENDERER
    if (ctx->backend_context) {
        vulkan_cleanup((vulkan_context_t*)ctx->backend_context);
    }
#endif
    
    free(ctx);
}
