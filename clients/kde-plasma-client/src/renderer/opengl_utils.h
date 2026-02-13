/**
 * @file opengl_utils.h
 * @brief OpenGL utility functions for shader and texture management
 */

#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <stdint.h>
#include <GL/gl.h>
#include <GL/glext.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compile a GLSL shader from source
 * 
 * @param source Shader source code
 * @param shader_type GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @return Shader handle, or 0 on failure
 */
GLuint glsl_compile_shader(const char *source, GLenum shader_type);

/**
 * Link vertex and fragment shaders into a program
 * 
 * @param vs Vertex shader handle
 * @param fs Fragment shader handle
 * @return Program handle, or 0 on failure
 */
GLuint glsl_link_program(GLuint vs, GLuint fs);

/**
 * Validate a shader program
 * 
 * @param program Program handle
 * @return 0 if valid, -1 if invalid
 */
int glsl_validate_program(GLuint program);

/**
 * Create a 2D texture with specified format
 * 
 * @param internal_format OpenGL internal format (GL_R8, GL_RG8, etc.)
 * @param width Texture width
 * @param height Texture height
 * @return Texture handle, or 0 on failure
 */
GLuint gl_create_texture_2d(GLenum internal_format, int width, int height);

/**
 * Upload data to 2D texture (synchronous)
 * 
 * @param texture Texture handle
 * @param data Pixel data
 * @param width Width in pixels
 * @param height Height in pixels
 * @return 0 on success, -1 on failure
 */
int gl_upload_texture_2d(GLuint texture, const uint8_t *data, int width, int height);

/**
 * Upload data to 2D texture using PBO (asynchronous)
 * 
 * @param texture Texture handle
 * @param data Pixel data
 * @param width Width in pixels
 * @param height Height in pixels
 * @param pbo_out Output PBO handle (for tracking)
 * @return 0 on success, -1 on failure
 */
int gl_upload_texture_2d_async(GLuint texture, const uint8_t *data, 
                                int width, int height, GLuint *pbo_out);

/**
 * Get OpenGL error string
 * 
 * @param error OpenGL error code
 * @return Human-readable error string
 */
const char* gl_get_error_string(GLenum error);

/**
 * Log shader compilation/link errors
 * 
 * @param shader Shader or program handle
 */
void gl_log_shader_error(GLuint shader);

#ifdef __cplusplus
}
#endif

#endif /* OPENGL_UTILS_H */
