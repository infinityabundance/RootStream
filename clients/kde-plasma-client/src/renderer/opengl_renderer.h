/**
 * @file opengl_renderer.h
 * @brief OpenGL 3.3+ rendering backend
 * 
 * Implements video rendering using OpenGL with NV12→RGB conversion.
 * Requires OpenGL 3.3+ with support for:
 * - GL_ARB_texture_rg (for UV plane)
 * - GL_ARB_pixel_buffer_object (for async uploads)
 * - GLX 1.3+ (for X11 integration)
 */

#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque OpenGL context handle
 */
typedef struct opengl_context_s opengl_context_t;

/**
 * Initialize OpenGL renderer backend
 * 
 * Creates OpenGL context, compiles shaders, and sets up textures.
 * 
 * @param native_window Native window handle (X11 Window*)
 * @return OpenGL context, or NULL on failure
 */
opengl_context_t* opengl_init(void *native_window);

/**
 * Upload frame data to GPU textures
 * 
 * Uploads Y and UV planes to separate textures using PBO for async transfer.
 * 
 * @param ctx OpenGL context
 * @param frame Frame to upload
 * @return 0 on success, -1 on failure
 */
int opengl_upload_frame(opengl_context_t *ctx, const frame_t *frame);

/**
 * Render current frame to screen
 * 
 * Applies NV12→RGB conversion shader and presents to display.
 * 
 * @param ctx OpenGL context
 * @return 0 on success, -1 on failure
 */
int opengl_render(opengl_context_t *ctx);

/**
 * Present rendered frame (swap buffers)
 * 
 * Blocks if vsync is enabled.
 * 
 * @param ctx OpenGL context
 * @return 0 on success, -1 on failure
 */
int opengl_present(opengl_context_t *ctx);

/**
 * Enable or disable vsync
 * 
 * @param ctx OpenGL context
 * @param enabled True to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int opengl_set_vsync(opengl_context_t *ctx, bool enabled);

/**
 * Resize rendering surface
 * 
 * @param ctx OpenGL context
 * @param width New width
 * @param height New height
 * @return 0 on success, -1 on failure
 */
int opengl_resize(opengl_context_t *ctx, int width, int height);

/**
 * Clean up OpenGL resources
 * 
 * Destroys context, textures, and shaders.
 * 
 * @param ctx OpenGL context
 */
void opengl_cleanup(opengl_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* OPENGL_RENDERER_H */
