// NV12 to RGB conversion shader
// Converts NV12 (Y + interleaved UV) to RGB using BT.709 color space

#version 330 core

// Vertex shader
#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 v_texCoord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    v_texCoord = texCoord;
}

#endif // VERTEX_SHADER

// Fragment shader
#ifdef FRAGMENT_SHADER

uniform sampler2D y_plane;   // Y plane (luminance)
uniform sampler2D uv_plane;  // UV plane (chrominance, interleaved)

in vec2 v_texCoord;
out vec4 fragColor;

// BT.709 YUV to RGB conversion matrix
const mat3 yuv_to_rgb = mat3(
    1.164,  1.164,  1.164,
    0.000, -0.391,  2.018,
    1.596, -0.813,  0.000
);

void main() {
    // Sample Y plane (luminance)
    float y = texture(y_plane, v_texCoord).r;
    
    // Sample UV plane (chrominance)
    vec2 uv = texture(uv_plane, v_texCoord).rg;
    
    // Convert from limited range (16-235) to full range (0-255)
    // and center UV around 0
    vec3 yuv;
    yuv.x = (y - 0.0625) * 1.164;  // Y: (Y - 16/255) * 255/219
    yuv.y = uv.r - 0.5;            // U: (U - 128/255)
    yuv.z = uv.g - 0.5;            // V: (V - 128/255)
    
    // Apply color space conversion
    vec3 rgb = yuv_to_rgb * yuv;
    
    // Clamp to valid range
    rgb = clamp(rgb, 0.0, 1.0);
    
    fragColor = vec4(rgb, 1.0);
}

#endif // FRAGMENT_SHADER
