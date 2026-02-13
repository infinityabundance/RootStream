/**
 * @file opengl_utils.c
 * @brief OpenGL utility functions implementation
 */

#include "opengl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLuint glsl_compile_shader(const char *source, GLenum shader_type) {
    if (!source) {
        return 0;
    }
    
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
