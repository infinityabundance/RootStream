#ifndef STEREOSCOPIC_RENDERER_H
#define STEREOSCOPIC_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Eye framebuffer structure
typedef struct {
    uint32_t colorTexture;
    uint32_t depthTexture;
    uint32_t framebuffer;
    uint32_t width;
    uint32_t height;
} EyeFramebuffer;

// Video frame structure (placeholder)
typedef struct {
    uint8_t *data;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint64_t timestamp;
} VideoFrame;

// VR headset parameters for distortion correction
typedef struct {
    float k1, k2;  // Radial distortion coefficients
    float p1, p2;  // Tangential distortion coefficients
    float chromatic_r, chromatic_b;  // Chromatic aberration offsets
} VRHeadsetParams;

// Distortion mesh
typedef struct {
    float *vertices;      // x, y positions
    float *texCoords;     // u, v texture coordinates
    uint32_t *indices;
    uint32_t vertexCount;
    uint32_t indexCount;
} DistortionMesh;

// Stereoscopic renderer structure
typedef struct StereoscopicRenderer StereoscopicRenderer;

// Creation and initialization
StereoscopicRenderer* stereoscopic_renderer_create(void);
int stereoscopic_renderer_init(StereoscopicRenderer *renderer, 
                               uint32_t eye_width, uint32_t eye_height);
void stereoscopic_renderer_cleanup(StereoscopicRenderer *renderer);
void stereoscopic_renderer_destroy(StereoscopicRenderer *renderer);

// Rendering functions
int stereoscopic_renderer_render_left_eye(StereoscopicRenderer *renderer,
                                         const VideoFrame *frame,
                                         const float projection[16],
                                         const float view[16]);

int stereoscopic_renderer_render_right_eye(StereoscopicRenderer *renderer,
                                          const VideoFrame *frame,
                                          const float projection[16],
                                          const float view[16]);

// Post-processing
int stereoscopic_renderer_apply_distortion(StereoscopicRenderer *renderer,
                                          EyeFramebuffer *eyeFB);

int stereoscopic_renderer_apply_chromatic_aberration(StereoscopicRenderer *renderer,
                                                    EyeFramebuffer *eyeFB);

// Distortion mesh generation
int stereoscopic_renderer_generate_distortion_mesh(StereoscopicRenderer *renderer,
                                                  const VRHeadsetParams *params);

// Get rendered textures
uint32_t stereoscopic_renderer_get_left_texture(StereoscopicRenderer *renderer);
uint32_t stereoscopic_renderer_get_right_texture(StereoscopicRenderer *renderer);
uint32_t stereoscopic_renderer_get_composite_texture(StereoscopicRenderer *renderer);

// Utility functions
int stereoscopic_renderer_resize(StereoscopicRenderer *renderer,
                                uint32_t eye_width, uint32_t eye_height);

#ifdef __cplusplus
}
#endif

#endif // STEREOSCOPIC_RENDERER_H
