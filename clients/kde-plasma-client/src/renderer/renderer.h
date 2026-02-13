/**
 * @file renderer.h
 * @brief Abstract video renderer API for RootStream client
 * 
 * Provides a unified interface for video rendering with support for multiple
 * backends (OpenGL, Vulkan, Proton). The renderer handles frame upload,
 * color space conversion (NV12→RGB), and display presentation.
 * 
 * Performance targets:
 * - 60 FPS rendering at 1080p
 * - <5ms GPU upload latency
 * - <2ms frame presentation time
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque renderer handle
 */
typedef struct renderer_s renderer_t;

/**
 * Common video frame formats (DRM fourcc codes)
 */
#define FRAME_FORMAT_NV12 0x3231564E  /**< NV12: Y plane + interleaved UV */

/**
 * Video frame structure
 */
typedef struct frame_s {
    uint8_t *data;              /**< Frame data buffer */
    uint32_t size;              /**< Size of data in bytes */
    uint32_t width;             /**< Frame width in pixels */
    uint32_t height;            /**< Frame height in pixels */
    uint32_t format;            /**< DRM fourcc format (e.g., NV12) */
    uint64_t timestamp_us;      /**< Presentation timestamp in microseconds */
    bool is_keyframe;           /**< True if this is a keyframe */
} frame_t;

/**
 * Renderer backend types
 */
typedef enum {
    RENDERER_OPENGL,           /**< OpenGL 3.3+ renderer */
    RENDERER_VULKAN,           /**< Vulkan renderer (Phase 12) */
    RENDERER_PROTON,           /**< Proton renderer (Phase 13) */
    RENDERER_AUTO              /**< Auto-detect best backend */
} renderer_backend_t;

/**
 * Renderer performance metrics
 */
struct renderer_metrics {
    double fps;                /**< Current frames per second */
    double frame_time_ms;      /**< Average frame time in milliseconds */
    double gpu_upload_ms;      /**< GPU upload time in milliseconds */
    uint64_t frames_dropped;   /**< Total number of dropped frames */
    uint64_t total_frames;     /**< Total number of frames rendered */
};

/**
 * Create a new renderer instance
 * 
 * @param backend Renderer backend to use (or RENDERER_AUTO for auto-detect)
 * @param width Initial frame width
 * @param height Initial frame height
 * @return Renderer handle, or NULL on failure
 */
renderer_t* renderer_create(renderer_backend_t backend, int width, int height);

/**
 * Initialize renderer with native window/display
 * 
 * @param renderer Renderer handle
 * @param native_window Native window handle (X11 Window, etc.)
 * @return 0 on success, -1 on failure
 */
int renderer_init(renderer_t *renderer, void *native_window);

/**
 * Submit a frame for rendering
 * 
 * This uploads the frame to GPU memory and queues it for presentation.
 * The function is thread-safe and non-blocking.
 * 
 * @param renderer Renderer handle
 * @param frame Frame to render
 * @return 0 on success, -1 on failure
 */
int renderer_submit_frame(renderer_t *renderer, const frame_t *frame);

/**
 * Present the current frame to the display
 * 
 * This should be called from the rendering thread at the desired frame rate.
 * The function will block if vsync is enabled.
 * 
 * @param renderer Renderer handle
 * @return 0 on success, -1 on failure
 */
int renderer_present(renderer_t *renderer);

/**
 * Enable or disable vertical sync
 * 
 * @param renderer Renderer handle
 * @param enabled True to enable vsync, false to disable
 * @return 0 on success, -1 on failure
 */
int renderer_set_vsync(renderer_t *renderer, bool enabled);

/**
 * Set fullscreen mode
 * 
 * @param renderer Renderer handle
 * @param fullscreen True for fullscreen, false for windowed
 * @return 0 on success, -1 on failure
 */
int renderer_set_fullscreen(renderer_t *renderer, bool fullscreen);

/**
 * Resize the rendering surface
 * 
 * @param renderer Renderer handle
 * @param width New width in pixels
 * @param height New height in pixels
 * @return 0 on success, -1 on failure
 */
int renderer_resize(renderer_t *renderer, int width, int height);

/**
 * Get current performance metrics
 * 
 * @param renderer Renderer handle
 * @return Current metrics structure
 */
struct renderer_metrics renderer_get_metrics(renderer_t *renderer);

/**
 * Get last error message
 * 
 * @param renderer Renderer handle
 * @return Error string, or NULL if no error
 */
const char* renderer_get_error(renderer_t *renderer);

/**
 * Clean up and destroy renderer
 * 
 * @param renderer Renderer handle
 */
void renderer_cleanup(renderer_t *renderer);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_H */
