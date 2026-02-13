# Color Space Conversion and Shader Implementation

## Overview

This document explains the mathematical foundation of the NV12 to RGB color space conversion used in the VideoRenderer's OpenGL shaders.

## Color Spaces

### YUV (NV12)

**NV12** is a YUV format commonly used in video encoding and hardware acceleration:

- **Y Plane**: Luminance (brightness) information
  - Size: `width × height` bytes
  - Range: 16-235 (limited range) or 0-255 (full range)
  
- **UV Plane**: Chrominance (color) information
  - Size: `(width/2) × (height/2) × 2` bytes
  - Format: Interleaved U and V samples (U₀V₀U₁V₁...)
  - Subsampled: 4:2:0 (each UV sample shared by 2×2 pixels)
  - Range: 16-240 (limited range) or 0-255 (full range)

### RGB

**RGB** is the standard display format:
- **R**: Red channel (0-255)
- **G**: Green channel (0-255)
- **B**: Blue channel (0-255)

## BT.709 Color Space Standard

The BT.709 (Rec. 709) standard defines the conversion for HDTV:

### Limited Range Formula

For limited range video (Y: 16-235, UV: 16-240):

```
R = 1.164(Y - 16) + 1.596(V - 128)
G = 1.164(Y - 16) - 0.391(U - 128) - 0.813(V - 128)
B = 1.164(Y - 16) + 2.018(U - 128)
```

Where:
- `1.164 = 255 / (235 - 16)` - Expands Y from limited to full range
- `1.596 = 255 / (224 * 0.7152)` - V contribution to R
- `0.391 = 255 * 0.114 / (224 * 0.7152)` - U contribution to G
- `0.813 = 255 * 0.299 / (224 * 0.7152)` - V contribution to G
- `2.018 = 255 / (224 * 0.5870)` - U contribution to B

### Matrix Form

The conversion can be expressed as matrix multiplication:

```
┌   ┐   ┌                      ┐   ┌ Y - 16  ┐
│ R │   │ 1.164   0.000   1.596 │   │ U - 128 │
│ G │ = │ 1.164  -0.391  -0.813 │ × │ V - 128 │
│ B │   │ 1.164   2.018   0.000 │   └─────────┘
└   ┘   └                      ┘
```

## Shader Implementation

### Vertex Shader

```glsl
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 v_texCoord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    v_texCoord = texCoord;
}
```

**Purpose**: Pass through vertex positions and texture coordinates for fullscreen quad.

### Fragment Shader

```glsl
#version 330 core

uniform sampler2D y_plane;   // Y plane texture
uniform sampler2D uv_plane;  // UV plane texture

in vec2 v_texCoord;
out vec4 fragColor;

// BT.709 YUV to RGB conversion matrix
const mat3 yuv_to_rgb = mat3(
    1.164,  1.164,  1.164,   // Column 0: Y contribution
    0.000, -0.391,  2.018,   // Column 1: U contribution
    1.596, -0.813,  0.000    // Column 2: V contribution
);

void main() {
    // Sample Y plane (luminance)
    float y = texture(y_plane, v_texCoord).r;
    
    // Sample UV plane (chrominance)
    vec2 uv = texture(uv_plane, v_texCoord).rg;
    
    // Convert from limited range to full range and center UV
    vec3 yuv;
    yuv.x = (y - 0.0625) * 1.164;  // Y: (Y - 16/255) normalized
    yuv.y = uv.r - 0.5;            // U: centered around 0
    yuv.z = uv.g - 0.5;            // V: centered around 0
    
    // Apply color space conversion
    vec3 rgb = yuv_to_rgb * yuv;
    
    // Clamp to valid range
    rgb = clamp(rgb, 0.0, 1.0);
    
    fragColor = vec4(rgb, 1.0);
}
```

**Key Points**:

1. **Texture Sampling**: 
   - Y plane: Single-channel (R8) texture
   - UV plane: Two-channel (RG8) texture with interleaved U/V

2. **Range Conversion**:
   - Y: `(y - 16/255)` removes offset, then multiply by 1.164
   - UV: `(uv - 128/255)` centers around 0 (becomes ±0.5)

3. **Matrix Multiplication**:
   - GLSL matrices are column-major
   - Each column represents the contribution of Y, U, V to R, G, B

4. **Clamping**:
   - Ensures output is valid [0, 1] range
   - Handles minor numerical errors

## OpenGL Texture Setup

### Y Plane Texture

```c
GLuint y_texture = gl_create_texture_2d(GL_R8, width, height);
gl_upload_texture_2d(y_texture, y_data, width, height);
```

- **Internal Format**: `GL_R8` (8-bit red channel)
- **Size**: Full resolution (width × height)

### UV Plane Texture

```c
GLuint uv_texture = gl_create_texture_2d(GL_RG8, width/2, height/2);
gl_upload_texture_2d(uv_texture, uv_data, width/2, height/2);
```

- **Internal Format**: `GL_RG8` (8-bit RG channels)
- **Size**: Half resolution (width/2 × height/2)
- **Filtering**: Linear (bilinear interpolation for upsampling)

## Verification

### Test Cases

| Input (YUV)       | Expected RGB | Description |
|-------------------|--------------|-------------|
| (16, 128, 128)    | (0, 0, 0)    | Black       |
| (235, 128, 128)   | (255, 255, 255) | White    |
| (82, 90, 240)     | (255, 0, 0)  | Red         |
| (145, 54, 34)     | (0, 255, 0)  | Green       |
| (41, 240, 110)    | (0, 0, 255)  | Blue        |
| (128, 128, 128)   | (128, 128, 128) | Gray     |

### Manual Calculation Example

Convert Red (Y=82, U=90, V=240) to RGB:

```
Y_norm = (82/255 - 16/255) * 1.164 = (0.322 - 0.0627) * 1.164 = 0.302
U_norm = 90/255 - 0.5 = 0.353 - 0.5 = -0.147
V_norm = 240/255 - 0.5 = 0.941 - 0.5 = 0.441

R = 1.164 * 0.302 + 0.000 * (-0.147) + 1.596 * 0.441 = 0.352 + 0.704 = 1.056 → 1.0
G = 1.164 * 0.302 + (-0.391) * (-0.147) + (-0.813) * 0.441 = 0.352 + 0.057 - 0.358 = 0.051
B = 1.164 * 0.302 + 2.018 * (-0.147) + 0.000 * 0.441 = 0.352 - 0.297 = 0.055

RGB = (1.0, 0.05, 0.05) × 255 ≈ (255, 13, 13) ≈ Red
```

Note: Small deviations due to rounding and limited range quantization.

## Performance Considerations

### GPU Optimization

1. **Texture Format**: `GL_R8` and `GL_RG8` are native formats on most GPUs
2. **Linear Filtering**: Hardware-accelerated bilinear interpolation for UV upsampling
3. **Constant Matrix**: Shader compiler can optimize the constant matrix multiplication
4. **No Branching**: Simple arithmetic operations, fully parallel

### Expected Performance

- **Shader Execution**: <0.5ms @ 1080p on modern GPU
- **Texture Upload**: 1-3ms @ 1080p (DMA transfer)
- **Total Frame Time**: 1.5-4ms @ 1080p

### Bottlenecks

- **Memory Bandwidth**: Texture upload is memory-bound
- **Solution**: Use PBO (Pixel Buffer Objects) for async transfer
- **Alternative**: Use hardware decode with zero-copy (VA-API)

## References

1. **ITU-R BT.709**: Parameter values for HDTV standard
2. **ISO/IEC 23001-8**: Coding-independent code points (fourcc codes)
3. **Khronos OpenGL Wiki**: Texture formats and color spaces

## See Also

- `renderer/color_space.h`: C API for color space matrices
- `renderer/shader/nv12_to_rgb.glsl`: Complete shader source
- `tests/fixtures/`: Test frames for verification
