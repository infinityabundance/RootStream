#version 450

// Vertex shader for fullscreen quad
// Generates a fullscreen quad without requiring vertex buffers

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Generate fullscreen quad positions
    // Triangle strip: (-1,-1), (1,-1), (-1,1), (1,1)
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0),  // Bottom-left
        vec2( 1.0, -1.0),  // Bottom-right
        vec2(-1.0,  1.0),  // Top-left
        vec2( 1.0,  1.0)   // Top-right
    );
    
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    
    // Convert from [-1,1] to [0,1] for texture coordinates
    fragTexCoord = (pos + 1.0) / 2.0;
}
