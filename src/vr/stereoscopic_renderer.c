#include "stereoscopic_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct StereoscopicRenderer {
    EyeFramebuffer leftEye;
    EyeFramebuffer rightEye;
    
    DistortionMesh distortionLeft;
    DistortionMesh distortionRight;
    
    uint32_t compositeTexture;
    bool initialized;
    VRHeadsetParams headsetParams;
};

StereoscopicRenderer* stereoscopic_renderer_create(void) {
    StereoscopicRenderer *renderer = (StereoscopicRenderer*)calloc(1, sizeof(StereoscopicRenderer));
    if (!renderer) {
        fprintf(stderr, "Failed to allocate StereoscopicRenderer\n");
        return NULL;
    }
    
    renderer->initialized = false;
    
    // Set default headset parameters (similar to Oculus Rift)
    renderer->headsetParams.k1 = 0.22f;
    renderer->headsetParams.k2 = 0.24f;
    renderer->headsetParams.p1 = 0.0f;
    renderer->headsetParams.p2 = 0.0f;
    renderer->headsetParams.chromatic_r = -0.015f;
    renderer->headsetParams.chromatic_b = 0.02f;
    
    return renderer;
}

int stereoscopic_renderer_init(StereoscopicRenderer *renderer, 
                               uint32_t eye_width, uint32_t eye_height) {
    if (!renderer) {
        return -1;
    }
    
    // Initialize left eye framebuffer
    renderer->leftEye.width = eye_width;
    renderer->leftEye.height = eye_height;
    renderer->leftEye.colorTexture = 0;  // Would be allocated with glGenTextures
    renderer->leftEye.depthTexture = 0;
    renderer->leftEye.framebuffer = 0;
    
    // Initialize right eye framebuffer
    renderer->rightEye.width = eye_width;
    renderer->rightEye.height = eye_height;
    renderer->rightEye.colorTexture = 0;
    renderer->rightEye.depthTexture = 0;
    renderer->rightEye.framebuffer = 0;
    
    // Composite texture for side-by-side rendering
    renderer->compositeTexture = 0;
    
    // Generate distortion mesh
    stereoscopic_renderer_generate_distortion_mesh(renderer, &renderer->headsetParams);
    
    renderer->initialized = true;
    
    printf("Stereoscopic renderer initialized: %dx%d per eye\n", eye_width, eye_height);
    
    return 0;
}

int stereoscopic_renderer_render_left_eye(StereoscopicRenderer *renderer,
                                         const VideoFrame *frame,
                                         const float projection[16],
                                         const float view[16]) {
    if (!renderer || !renderer->initialized || !frame) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Bind left eye framebuffer
    // 2. Set projection and view matrices
    // 3. Render the video frame with proper 3D positioning
    // 4. Apply any shader effects
    
    // For now, stub implementation
    (void)projection;
    (void)view;
    
    return 0;
}

int stereoscopic_renderer_render_right_eye(StereoscopicRenderer *renderer,
                                          const VideoFrame *frame,
                                          const float projection[16],
                                          const float view[16]) {
    if (!renderer || !renderer->initialized || !frame) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Bind right eye framebuffer
    // 2. Set projection and view matrices
    // 3. Render the video frame with proper 3D positioning
    // 4. Apply any shader effects
    
    // For now, stub implementation
    (void)projection;
    (void)view;
    
    return 0;
}

int stereoscopic_renderer_apply_distortion(StereoscopicRenderer *renderer,
                                          EyeFramebuffer *eyeFB) {
    if (!renderer || !eyeFB) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Use the distortion mesh to warp the rendered image
    // 2. Apply barrel/pincushion distortion based on headset parameters
    // 3. Compensate for lens distortion
    
    return 0;
}

int stereoscopic_renderer_apply_chromatic_aberration(StereoscopicRenderer *renderer,
                                                    EyeFramebuffer *eyeFB) {
    if (!renderer || !eyeFB) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Sample the texture with different UV offsets for R, G, B channels
    // 2. Compensate for lens chromatic aberration
    // 3. Use headset-specific chromatic aberration parameters
    
    return 0;
}

int stereoscopic_renderer_generate_distortion_mesh(StereoscopicRenderer *renderer,
                                                  const VRHeadsetParams *params) {
    if (!renderer || !params) {
        return -1;
    }
    
    // Generate a grid mesh for distortion warping
    // This is a simplified version - real implementation would be more complex
    
    const uint32_t gridWidth = 40;
    const uint32_t gridHeight = 40;
    const uint32_t vertexCount = (gridWidth + 1) * (gridHeight + 1);
    const uint32_t indexCount = gridWidth * gridHeight * 6;  // 2 triangles per quad
    
    // Allocate distortion mesh for left eye
    renderer->distortionLeft.vertices = (float*)malloc(vertexCount * 2 * sizeof(float));
    renderer->distortionLeft.texCoords = (float*)malloc(vertexCount * 2 * sizeof(float));
    renderer->distortionLeft.indices = (uint32_t*)malloc(indexCount * sizeof(uint32_t));
    renderer->distortionLeft.vertexCount = vertexCount;
    renderer->distortionLeft.indexCount = indexCount;
    
    // Allocate distortion mesh for right eye
    renderer->distortionRight.vertices = (float*)malloc(vertexCount * 2 * sizeof(float));
    renderer->distortionRight.texCoords = (float*)malloc(vertexCount * 2 * sizeof(float));
    renderer->distortionRight.indices = (uint32_t*)malloc(indexCount * sizeof(uint32_t));
    renderer->distortionRight.vertexCount = vertexCount;
    renderer->distortionRight.indexCount = indexCount;
    
    // Generate grid vertices and texture coordinates
    for (uint32_t y = 0; y <= gridHeight; y++) {
        for (uint32_t x = 0; x <= gridWidth; x++) {
            uint32_t idx = y * (gridWidth + 1) + x;
            
            // Normalized position [-1, 1]
            float nx = (float)x / (float)gridWidth * 2.0f - 1.0f;
            float ny = (float)y / (float)gridHeight * 2.0f - 1.0f;
            
            // Apply distortion based on radial distance
            float r2 = nx * nx + ny * ny;
            float distortion = 1.0f + params->k1 * r2 + params->k2 * r2 * r2;
            
            float distorted_x = nx * distortion;
            float distorted_y = ny * distortion;
            
            // Store vertices (screen position)
            renderer->distortionLeft.vertices[idx * 2 + 0] = distorted_x;
            renderer->distortionLeft.vertices[idx * 2 + 1] = distorted_y;
            
            // Store texture coordinates (original position)
            renderer->distortionLeft.texCoords[idx * 2 + 0] = (nx + 1.0f) * 0.5f;
            renderer->distortionLeft.texCoords[idx * 2 + 1] = (ny + 1.0f) * 0.5f;
            
            // Same for right eye (could have slight differences)
            renderer->distortionRight.vertices[idx * 2 + 0] = distorted_x;
            renderer->distortionRight.vertices[idx * 2 + 1] = distorted_y;
            renderer->distortionRight.texCoords[idx * 2 + 0] = (nx + 1.0f) * 0.5f;
            renderer->distortionRight.texCoords[idx * 2 + 1] = (ny + 1.0f) * 0.5f;
        }
    }
    
    // Generate indices for triangle strips
    uint32_t indexOffset = 0;
    for (uint32_t y = 0; y < gridHeight; y++) {
        for (uint32_t x = 0; x < gridWidth; x++) {
            uint32_t topLeft = y * (gridWidth + 1) + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (y + 1) * (gridWidth + 1) + x;
            uint32_t bottomRight = bottomLeft + 1;
            
            // First triangle
            renderer->distortionLeft.indices[indexOffset++] = topLeft;
            renderer->distortionLeft.indices[indexOffset++] = bottomLeft;
            renderer->distortionLeft.indices[indexOffset++] = topRight;
            
            // Second triangle
            renderer->distortionLeft.indices[indexOffset++] = topRight;
            renderer->distortionLeft.indices[indexOffset++] = bottomLeft;
            renderer->distortionLeft.indices[indexOffset++] = bottomRight;
        }
    }
    
    // Copy indices to right eye mesh
    memcpy(renderer->distortionRight.indices, renderer->distortionLeft.indices, 
           indexCount * sizeof(uint32_t));
    
    printf("Generated distortion mesh: %d vertices, %d indices\n", vertexCount, indexCount);
    
    return 0;
}

uint32_t stereoscopic_renderer_get_left_texture(StereoscopicRenderer *renderer) {
    if (!renderer) {
        return 0;
    }
    return renderer->leftEye.colorTexture;
}

uint32_t stereoscopic_renderer_get_right_texture(StereoscopicRenderer *renderer) {
    if (!renderer) {
        return 0;
    }
    return renderer->rightEye.colorTexture;
}

uint32_t stereoscopic_renderer_get_composite_texture(StereoscopicRenderer *renderer) {
    if (!renderer) {
        return 0;
    }
    return renderer->compositeTexture;
}

int stereoscopic_renderer_resize(StereoscopicRenderer *renderer,
                                uint32_t eye_width, uint32_t eye_height) {
    if (!renderer) {
        return -1;
    }
    
    // In a real implementation, this would:
    // 1. Destroy old framebuffers
    // 2. Recreate with new size
    
    renderer->leftEye.width = eye_width;
    renderer->leftEye.height = eye_height;
    renderer->rightEye.width = eye_width;
    renderer->rightEye.height = eye_height;
    
    printf("Stereoscopic renderer resized: %dx%d per eye\n", eye_width, eye_height);
    
    return 0;
}

void stereoscopic_renderer_cleanup(StereoscopicRenderer *renderer) {
    if (!renderer) {
        return;
    }
    
    // Free distortion meshes
    if (renderer->distortionLeft.vertices) {
        free(renderer->distortionLeft.vertices);
        renderer->distortionLeft.vertices = NULL;
    }
    if (renderer->distortionLeft.texCoords) {
        free(renderer->distortionLeft.texCoords);
        renderer->distortionLeft.texCoords = NULL;
    }
    if (renderer->distortionLeft.indices) {
        free(renderer->distortionLeft.indices);
        renderer->distortionLeft.indices = NULL;
    }
    
    if (renderer->distortionRight.vertices) {
        free(renderer->distortionRight.vertices);
        renderer->distortionRight.vertices = NULL;
    }
    if (renderer->distortionRight.texCoords) {
        free(renderer->distortionRight.texCoords);
        renderer->distortionRight.texCoords = NULL;
    }
    if (renderer->distortionRight.indices) {
        free(renderer->distortionRight.indices);
        renderer->distortionRight.indices = NULL;
    }
    
    // In a real implementation, would also:
    // 1. Delete GL framebuffers
    // 2. Delete GL textures
    
    renderer->initialized = false;
    
    printf("Stereoscopic renderer cleaned up\n");
}

void stereoscopic_renderer_destroy(StereoscopicRenderer *renderer) {
    if (!renderer) {
        return;
    }
    
    stereoscopic_renderer_cleanup(renderer);
    free(renderer);
}
