/**
 * @file opengl_utils.c
 * @brief OpenGL utility functions implementation
 */

#include "opengl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>

// OpenGL 2.0+ function pointers
static PFNGLCREATESHADERPROC glCreateShader = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
static PFNGLDELETESHADERPROC glDeleteShader = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
static PFNGLATTACHSHADERPROC glAttachShader = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
static PFNGLVALIDATEPROGRAMPROC glValidateProgram = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
static PFNGLUNIFORM1IPROC glUniform1i = NULL;

// OpenGL 1.5+ function pointers (VBO)
static PFNGLGENBUFFERSPROC glGenBuffers = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
static PFNGLBUFFERDATAPROC glBufferData = NULL;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;

// OpenGL 2.0+ function pointers (vertex attributes)
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;

// OpenGL 3.0+ function pointers (VAO)
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;

static int gl_functions_loaded = 0;

static void load_gl_functions(void) {
    if (gl_functions_loaded) return;
    
    // Load OpenGL 2.0+ functions
    glCreateShader = (PFNGLCREATESHADERPROC)glXGetProcAddress((const GLubyte*)"glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)glXGetProcAddress((const GLubyte*)"glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)glXGetProcAddress((const GLubyte*)"glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)glXGetProcAddress((const GLubyte*)"glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glXGetProcAddress((const GLubyte*)"glGetShaderInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)glXGetProcAddress((const GLubyte*)"glDeleteShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)glXGetProcAddress((const GLubyte*)"glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glXGetProcAddress((const GLubyte*)"glGetProgramiv");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glDeleteProgram");
    glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glValidateProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glXGetProcAddress((const GLubyte*)"glGetUniformLocation");
    glUseProgram = (PFNGLUSEPROGRAMPROC)glXGetProcAddress((const GLubyte*)"glUseProgram");
    glUniform1i = (PFNGLUNIFORM1IPROC)glXGetProcAddress((const GLubyte*)"glUniform1i");
    
    // Load OpenGL 1.5+ functions
    glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress((const GLubyte*)"glBufferData");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteBuffers");
    
    // Load OpenGL 2.0+ functions
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glXGetProcAddress((const GLubyte*)"glVertexAttribPointer");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glXGetProcAddress((const GLubyte*)"glEnableVertexAttribArray");
    
    // Load OpenGL 3.0+ functions
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glXGetProcAddress((const GLubyte*)"glBindVertexArray");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glXGetProcAddress((const GLubyte*)"glDeleteVertexArrays");
    
    gl_functions_loaded = 1;
}

GLuint glsl_compile_shader(const char *source, GLenum shader_type) {
    if (!source) {
        return 0;
    }
    
    load_gl_functions();
    
    GLuint shader = glCreateShader(shader_type);
    if (!shader) {
        return 0;
    }
    
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    // Check compilation status
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        gl_log_shader_error(shader);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint glsl_link_program(GLuint vs, GLuint fs) {
    if (!vs || !fs) {
        return 0;
    }
    
    load_gl_functions();
    
    GLuint program = glCreateProgram();
    if (!program) {
        return 0;
    }
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    
    // Check link status
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        gl_log_shader_error(program);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

int glsl_validate_program(GLuint program) {
    if (!program) {
        return -1;
    }
    
    load_gl_functions();
    
    glValidateProgram(program);
    
    GLint status;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE) {
        gl_log_shader_error(program);
        return -1;
    }
    
    return 0;
}

GLuint gl_create_texture_2d(GLenum internal_format, int width, int height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    if (!texture) {
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture storage
    GLenum format = (internal_format == GL_R8) ? GL_RED : GL_RG;
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, 
                 format, GL_UNSIGNED_BYTE, NULL);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "Failed to create texture: %s\n", gl_get_error_string(error));
        glDeleteTextures(1, &texture);
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

int gl_upload_texture_2d(GLuint texture, const uint8_t *data, int width, int height) {
    if (!texture || !data || width <= 0 || height <= 0) {
        return -1;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Get texture format
    GLint internal_format;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
    GLenum format = (internal_format == GL_R8) ? GL_RED : GL_RG;
    
    // Upload data
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, 
                    format, GL_UNSIGNED_BYTE, data);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "Failed to upload texture: %s\n", gl_get_error_string(error));
        glBindTexture(GL_TEXTURE_2D, 0);
        return -1;
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

int gl_upload_texture_2d_async(GLuint texture, const uint8_t *data, 
                                int width, int height, GLuint *pbo_out) {
    if (!texture || !data || width <= 0 || height <= 0) {
        return -1;
    }
    
    load_gl_functions();
    
    // Get texture format
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint internal_format;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
    GLenum format = (internal_format == GL_R8) ? GL_RED : GL_RG;
    int pixel_size = (format == GL_RED) ? 1 : 2;
    int data_size = width * height * pixel_size;
    
    // Create PBO for async upload
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, data_size, data, GL_STREAM_DRAW);
    
    // Upload from PBO to texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, 
                    format, GL_UNSIGNED_BYTE, 0);
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if (pbo_out) {
        *pbo_out = pbo;
    } else {
        // Clean up PBO immediately if not tracked
        glDeleteBuffers(1, &pbo);
    }
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "Failed to upload texture async: %s\n", gl_get_error_string(error));
        return -1;
    }
    
    return 0;
}

const char* gl_get_error_string(GLenum error) {
    switch (error) {
        case GL_NO_ERROR:          return "No error";
        case GL_INVALID_ENUM:      return "Invalid enum";
        case GL_INVALID_VALUE:     return "Invalid value";
        case GL_INVALID_OPERATION: return "Invalid operation";
        case GL_OUT_OF_MEMORY:     return "Out of memory";
        default:                   return "Unknown error";
    }
}

void gl_log_shader_error(GLuint shader) {
    load_gl_functions();
    
    GLint log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    
    if (log_length > 0) {
        char *log = (char*)malloc(log_length);
        if (log) {
            glGetShaderInfoLog(shader, log_length, NULL, log);
            fprintf(stderr, "Shader error: %s\n", log);
            free(log);
        }
    }
}
