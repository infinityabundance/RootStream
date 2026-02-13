/**
 * @file renderer.c
 * @brief Video renderer implementation with backend abstraction
 */

#include "renderer.h"
#include "opengl_renderer.h"
#include "frame_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Renderer structure
 */
struct renderer_s {
    renderer_backend_t backend;
    void *impl;                    // Backend-specific context (e.g., opengl_context_t*)
    frame_buffer_t frame_buffer;   // Frame queue
    
    int width;
    int height;
    
    struct {
        uint64_t total_frames;
        uint64_t dropped_frames;
        uint64_t last_frame_time_us;
        double fps;
        double frame_time_ms;
        double gpu_upload_ms;
    } metrics;
    
    char last_error[256];
};

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

renderer_t* renderer_create(renderer_backend_t backend, int width, int height) {
    if (width <= 0 || height <= 0) {
        return NULL;
    }
    
    renderer_t *renderer = (renderer_t*)calloc(1, sizeof(renderer_t));
    if (!renderer) {
        return NULL;
    }
    
    renderer->backend = backend;
    renderer->width = width;
    renderer->height = height;
    
    // Initialize frame buffer
    if (frame_buffer_init(&renderer->frame_buffer) != 0) {
        free(renderer);
        return NULL;
    }
    
    // Auto-detect backend if requested
    if (backend == RENDERER_AUTO) {
        // For now, always use OpenGL
        // In the future, we can detect Vulkan/Proton support here
        renderer->backend = RENDERER_OPENGL;
    }
    
    return renderer;
}

int renderer_init(renderer_t *renderer, void *native_window) {
    if (!renderer) {
        return -1;
    }
    
    // Initialize backend
    switch (renderer->backend) {
        case RENDERER_OPENGL:
            renderer->impl = opengl_init(native_window);
            if (!renderer->impl) {
                snprintf(renderer->last_error, sizeof(renderer->last_error),
                        "Failed to initialize OpenGL backend");
                return -1;
            }
            break;
            
        case RENDERER_VULKAN:
            snprintf(renderer->last_error, sizeof(renderer->last_error),
                    "Vulkan backend not yet implemented (Phase 12)");
            return -1;
            
        case RENDERER_PROTON:
            snprintf(renderer->last_error, sizeof(renderer->last_error),
                    "Proton backend not yet implemented (Phase 13)");
            return -1;
            
        default:
            snprintf(renderer->last_error, sizeof(renderer->last_error),
                    "Unknown backend type");
            return -1;
    }
    
    return 0;
}

int renderer_submit_frame(renderer_t *renderer, const frame_t *frame) {
    if (!renderer || !frame) {
        return -1;
    }
    
    // Enqueue frame
    if (frame_buffer_enqueue(&renderer->frame_buffer, frame) != 0) {
        renderer->metrics.dropped_frames++;
        snprintf(renderer->last_error, sizeof(renderer->last_error),
                "Failed to enqueue frame");
        return -1;
    }
    
    renderer->metrics.total_frames++;
    
    return 0;
}

int renderer_present(renderer_t *renderer) {
    if (!renderer) {
        return -1;
    }
    
    uint64_t start_time = get_time_us();
    
    // Dequeue frame
    frame_t *frame = frame_buffer_dequeue(&renderer->frame_buffer);
    if (!frame) {
        // No frame available, just present what we have
        if (renderer->backend == RENDERER_OPENGL && renderer->impl) {
            opengl_render((opengl_context_t*)renderer->impl);
            opengl_present((opengl_context_t*)renderer->impl);
        }
        return 0;
    }
    
    // Upload and render frame
    int result = 0;
    switch (renderer->backend) {
        case RENDERER_OPENGL:
            if (renderer->impl) {
                if (opengl_upload_frame((opengl_context_t*)renderer->impl, frame) != 0) {
                    snprintf(renderer->last_error, sizeof(renderer->last_error),
                            "Failed to upload frame to GPU");
                    result = -1;
                } else if (opengl_render((opengl_context_t*)renderer->impl) != 0) {
                    snprintf(renderer->last_error, sizeof(renderer->last_error),
                            "Failed to render frame");
                    result = -1;
                } else if (opengl_present((opengl_context_t*)renderer->impl) != 0) {
                    snprintf(renderer->last_error, sizeof(renderer->last_error),
                            "Failed to present frame");
                    result = -1;
                }
            }
            break;
            
        default:
            snprintf(renderer->last_error, sizeof(renderer->last_error),
                    "Backend not implemented");
            result = -1;
            break;
    }
    
    // Free frame data
    if (frame->data) {
        free(frame->data);
    }
    free(frame);
    
    // Update metrics
    uint64_t end_time = get_time_us();
    renderer->metrics.frame_time_ms = (double)(end_time - start_time) / 1000.0;
    
    if (renderer->metrics.last_frame_time_us > 0) {
        uint64_t delta = end_time - renderer->metrics.last_frame_time_us;
        if (delta > 0) {
            renderer->metrics.fps = 1000000.0 / (double)delta;
        }
    }
    renderer->metrics.last_frame_time_us = end_time;
    
    return result;
}

int renderer_set_vsync(renderer_t *renderer, bool enabled) {
    if (!renderer) {
        return -1;
    }
    
    switch (renderer->backend) {
        case RENDERER_OPENGL:
            if (renderer->impl) {
                return opengl_set_vsync((opengl_context_t*)renderer->impl, enabled);
            }
            break;
            
        default:
            break;
    }
    
    return -1;
}

int renderer_set_fullscreen(renderer_t *renderer, bool fullscreen) {
    if (!renderer) {
        return -1;
    }
    
    // Fullscreen is typically handled by the window manager
    // This is a placeholder for future implementation
    (void)fullscreen;
    
    return 0;
}

int renderer_resize(renderer_t *renderer, int width, int height) {
    if (!renderer || width <= 0 || height <= 0) {
        return -1;
    }
    
    renderer->width = width;
    renderer->height = height;
    
    switch (renderer->backend) {
        case RENDERER_OPENGL:
            if (renderer->impl) {
                return opengl_resize((opengl_context_t*)renderer->impl, width, height);
            }
            break;
            
        default:
            break;
    }
    
    return -1;
}

struct renderer_metrics renderer_get_metrics(renderer_t *renderer) {
    struct renderer_metrics metrics = {0};
    
    if (renderer) {
        metrics.fps = renderer->metrics.fps;
        metrics.frame_time_ms = renderer->metrics.frame_time_ms;
        metrics.gpu_upload_ms = renderer->metrics.gpu_upload_ms;
        metrics.frames_dropped = renderer->metrics.dropped_frames;
        metrics.total_frames = renderer->metrics.total_frames;
    }
    
    return metrics;
}

const char* renderer_get_error(renderer_t *renderer) {
    if (!renderer || renderer->last_error[0] == '\0') {
        return NULL;
    }
    
    return renderer->last_error;
}

void renderer_cleanup(renderer_t *renderer) {
    if (!renderer) {
        return;
    }
    
    // Cleanup backend
    switch (renderer->backend) {
        case RENDERER_OPENGL:
            if (renderer->impl) {
                opengl_cleanup((opengl_context_t*)renderer->impl);
            }
            break;
            
        default:
            break;
    }
    
    // Cleanup frame buffer
    frame_buffer_cleanup(&renderer->frame_buffer);
    
    free(renderer);
}
