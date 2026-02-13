/**
 * @file opengl_renderer.c
 * @brief OpenGL rendering backend implementation
 */

#include "opengl_renderer.h"
#include "opengl_utils.h"
#include "color_space.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

/**
 * OpenGL context structure
 */
struct opengl_context_s {
    // X11/GLX resources
    Display *x11_display;
    Window x11_window;
    GLXContext glx_context;
    GLXDrawable glx_drawable;
    
    // Frame dimensions
    int frame_width;
    int frame_height;
    
    // Textures
    GLuint y_texture;      // Y plane (luminance)
    GLuint uv_texture;     // UV plane (chrominance)
    
    // Shader program
    GLuint shader_program;
    GLint uniform_y_sampler;
    GLint uniform_uv_sampler;
    
    // Vertex data
    GLuint vao;
    GLuint vbo;
    
    // Frame timing
    uint64_t last_present_time_ns;
    bool vsync_enabled;
    
    // Performance tracking
    double last_upload_time_ms;
};

// Vertex shader source
static const char *vertex_shader_source = 
    "#version 330 core\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "out vec2 v_texCoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "    v_texCoord = texCoord;\n"
    "}\n";

// Fragment shader source (NV12 to RGB conversion)
static const char *fragment_shader_source = 
    "#version 330 core\n"
    "uniform sampler2D y_plane;\n"
    "uniform sampler2D uv_plane;\n"
    "in vec2 v_texCoord;\n"
    "out vec4 fragColor;\n"
    "const mat3 yuv_to_rgb = mat3(\n"
    "    1.164,  1.164,  1.164,\n"
    "    0.000, -0.391,  2.018,\n"
    "    1.596, -0.813,  0.000\n"
    ");\n"
    "void main() {\n"
    "    float y = texture(y_plane, v_texCoord).r;\n"
    "    vec2 uv = texture(uv_plane, v_texCoord).rg;\n"
    "    vec3 yuv;\n"
    "    yuv.x = (y - 0.0625) * 1.164;\n"
    "    yuv.y = uv.r - 0.5;\n"
    "    yuv.z = uv.g - 0.5;\n"
    "    vec3 rgb = yuv_to_rgb * yuv;\n"
    "    rgb = clamp(rgb, 0.0, 1.0);\n"
    "    fragColor = vec4(rgb, 1.0);\n"
    "}\n";

// Fullscreen quad vertices (position + texcoord)
static const float quad_vertices[] = {
    // Position     // TexCoord
    -1.0f, -1.0f,   0.0f, 1.0f,  // Bottom-left
     1.0f, -1.0f,   1.0f, 1.0f,  // Bottom-right
    -1.0f,  1.0f,   0.0f, 0.0f,  // Top-left
     1.0f,  1.0f,   1.0f, 0.0f   // Top-right
};

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

opengl_context_t* opengl_init(void *native_window) {
    if (!native_window) {
        return NULL;
    }
    
    // Allocate context
    opengl_context_t *ctx = (opengl_context_t*)calloc(1, sizeof(opengl_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Get X11 display and window
    ctx->x11_window = *(Window*)native_window;
    ctx->x11_display = XOpenDisplay(NULL);
    if (!ctx->x11_display) {
        fprintf(stderr, "Failed to open X11 display\n");
        free(ctx);
        return NULL;
    }
    
    // Choose GLX framebuffer config
    int visual_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    
    int fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig(ctx->x11_display, DefaultScreen(ctx->x11_display),
                                         visual_attribs, &fbcount);
    if (!fbc || fbcount == 0) {
        fprintf(stderr, "Failed to find suitable GLX framebuffer config\n");
        XCloseDisplay(ctx->x11_display);
        free(ctx);
        return NULL;
    }
    
    // Create GLX context
    ctx->glx_context = glXCreateNewContext(ctx->x11_display, fbc[0], GLX_RGBA_TYPE, NULL, True);
    if (!ctx->glx_context) {
        fprintf(stderr, "Failed to create GLX context\n");
        XFree(fbc);
        XCloseDisplay(ctx->x11_display);
        free(ctx);
        return NULL;
    }
    
    ctx->glx_drawable = ctx->x11_window;
    XFree(fbc);
    
    // Make context current
    if (!glXMakeCurrent(ctx->x11_display, ctx->glx_drawable, ctx->glx_context)) {
        fprintf(stderr, "Failed to make GLX context current\n");
        glXDestroyContext(ctx->x11_display, ctx->glx_context);
        XCloseDisplay(ctx->x11_display);
        free(ctx);
        return NULL;
    }
    
    // Compile shaders
    GLuint vs = glsl_compile_shader(vertex_shader_source, GL_VERTEX_SHADER);
    GLuint fs = glsl_compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER);
    if (!vs || !fs) {
        fprintf(stderr, "Failed to compile shaders\n");
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        glXMakeCurrent(ctx->x11_display, None, NULL);
        glXDestroyContext(ctx->x11_display, ctx->glx_context);
        XCloseDisplay(ctx->x11_display);
        free(ctx);
        return NULL;
    }
    
    // Link shader program
    ctx->shader_program = glsl_link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    if (!ctx->shader_program) {
        fprintf(stderr, "Failed to link shader program\n");
        glXMakeCurrent(ctx->x11_display, None, NULL);
        glXDestroyContext(ctx->x11_display, ctx->glx_context);
        XCloseDisplay(ctx->x11_display);
        free(ctx);
        return NULL;
    }
    
    // Get uniform locations
    ctx->uniform_y_sampler = glGetUniformLocation(ctx->shader_program, "y_plane");
    ctx->uniform_uv_sampler = glGetUniformLocation(ctx->shader_program, "uv_plane");
    
    // Create vertex array and buffer
    glGenVertexArrays(1, &ctx->vao);
    glGenBuffers(1, &ctx->vbo);
    
    glBindVertexArray(ctx->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Set default vsync
    ctx->vsync_enabled = true;
    opengl_set_vsync(ctx, true);
    
    return ctx;
}

int opengl_upload_frame(opengl_context_t *ctx, const frame_t *frame) {
    if (!ctx || !frame || !frame->data) {
        return -1;
    }
    
    uint64_t start_time = get_time_ns();
    
    // Update frame dimensions if changed
    if (ctx->frame_width != (int)frame->width || ctx->frame_height != (int)frame->height) {
        // Destroy old textures
        if (ctx->y_texture) {
            glDeleteTextures(1, &ctx->y_texture);
        }
        if (ctx->uv_texture) {
            glDeleteTextures(1, &ctx->uv_texture);
        }
        
        // Create new textures
        ctx->frame_width = frame->width;
        ctx->frame_height = frame->height;
        
        // Y plane: full resolution, single channel
        ctx->y_texture = gl_create_texture_2d(GL_R8, frame->width, frame->height);
        
        // UV plane: half resolution, two channels (interleaved)
        ctx->uv_texture = gl_create_texture_2d(GL_RG8, frame->width / 2, frame->height / 2);
        
        if (!ctx->y_texture || !ctx->uv_texture) {
            fprintf(stderr, "Failed to create textures\n");
            return -1;
        }
    }
    
    // Upload Y plane
    int y_size = frame->width * frame->height;
    if (gl_upload_texture_2d(ctx->y_texture, frame->data, frame->width, frame->height) != 0) {
        fprintf(stderr, "Failed to upload Y plane\n");
        return -1;
    }
    
    // Upload UV plane (interleaved U and V)
    uint8_t *uv_data = frame->data + y_size;
    if (gl_upload_texture_2d(ctx->uv_texture, uv_data, frame->width / 2, frame->height / 2) != 0) {
        fprintf(stderr, "Failed to upload UV plane\n");
        return -1;
    }
    
    uint64_t end_time = get_time_ns();
    ctx->last_upload_time_ms = (double)(end_time - start_time) / 1000000.0;
    
    return 0;
}

int opengl_render(opengl_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Clear screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use shader program
    glUseProgram(ctx->shader_program);
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ctx->y_texture);
    glUniform1i(ctx->uniform_y_sampler, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ctx->uv_texture);
    glUniform1i(ctx->uniform_uv_sampler, 1);
    
    // Draw quad
    glBindVertexArray(ctx->vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    
    return 0;
}

int opengl_present(opengl_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Swap buffers
    glXSwapBuffers(ctx->x11_display, ctx->glx_drawable);
    
    ctx->last_present_time_ns = get_time_ns();
    
    return 0;
}

int opengl_set_vsync(opengl_context_t *ctx, bool enabled) {
    if (!ctx) {
        return -1;
    }
    
    ctx->vsync_enabled = enabled;
    
    // Set swap interval (requires GLX_EXT_swap_control)
    typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
    PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = 
        (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
    
    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(ctx->x11_display, ctx->glx_drawable, enabled ? 1 : 0);
    }
    
    return 0;
}

int opengl_resize(opengl_context_t *ctx, int width, int height) {
    if (!ctx || width <= 0 || height <= 0) {
        return -1;
    }
    
    // Update viewport
    glViewport(0, 0, width, height);
    
    return 0;
}

void opengl_cleanup(opengl_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Make context current for cleanup
    if (ctx->glx_context) {
        glXMakeCurrent(ctx->x11_display, ctx->glx_drawable, ctx->glx_context);
    }
    
    // Delete OpenGL resources
    if (ctx->vao) {
        glDeleteVertexArrays(1, &ctx->vao);
    }
    if (ctx->vbo) {
        glDeleteBuffers(1, &ctx->vbo);
    }
    if (ctx->y_texture) {
        glDeleteTextures(1, &ctx->y_texture);
    }
    if (ctx->uv_texture) {
        glDeleteTextures(1, &ctx->uv_texture);
    }
    if (ctx->shader_program) {
        glDeleteProgram(ctx->shader_program);
    }
    
    // Destroy GLX context
    if (ctx->glx_context) {
        glXMakeCurrent(ctx->x11_display, None, NULL);
        glXDestroyContext(ctx->x11_display, ctx->glx_context);
    }
    
    // Close X11 display
    if (ctx->x11_display) {
        XCloseDisplay(ctx->x11_display);
    }
    
    free(ctx);
}
