#version 450

// Fragment shader for NV12 to RGB conversion
// Converts NV12 format (Y plane + interleaved UV plane) to RGB
// Uses BT.709 color space conversion

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

// Descriptor set bindings for Y and UV textures
layout(binding = 0) uniform sampler2D texY;   // Y plane (luminance)
layout(binding = 1) uniform sampler2D texUV;  // UV plane (chrominance, interleaved)

void main() {
    // Sample Y plane
    float y = texture(texY, fragTexCoord).r;
    
    // Sample UV plane (interleaved chroma)
    vec2 uv = texture(texUV, fragTexCoord).rg;
    
    // Convert from [0,1] range with UV centered at 0.5
    // BT.709 YUV to RGB conversion
    float u = uv.r - 0.5;
    float v = uv.g - 0.5;
    
    // BT.709 conversion coefficients
    vec3 rgb;
    rgb.r = y + 1.5748 * v;
    rgb.g = y - 0.1873 * u - 0.4681 * v;
    rgb.b = y + 1.8556 * u;
    
    // Clamp to valid range
    rgb = clamp(rgb, 0.0, 1.0);
    
    outColor = vec4(rgb, 1.0);
}
