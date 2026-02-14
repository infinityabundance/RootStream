# Vulkan Shaders for RootStream Client

This directory contains GLSL shaders for the Vulkan video renderer.

## Shaders

### fullscreen.vert
Vertex shader that generates a fullscreen quad procedurally.
- **Input:** None (uses gl_VertexIndex)
- **Output:** Texture coordinates for fragment shader
- **Geometry:** 4 vertices forming a triangle strip

### nv12_to_rgb.frag
Fragment shader that converts NV12 (YUV) video format to RGB.
- **Input:** Texture coordinates from vertex shader
- **Samplers:** 
  - Binding 0: Y plane (luminance)
  - Binding 1: UV plane (chrominance, interleaved)
- **Output:** RGB color
- **Color Space:** BT.709 conversion

## Compilation

### Prerequisites

Install a GLSL to SPIR-V compiler:

**Ubuntu/Debian:**
```bash
sudo apt install glslang-tools
```

**Arch Linux:**
```bash
sudo pacman -S glslang
```

**Fedora:**
```bash
sudo dnf install glslang
```

**From Vulkan SDK:**
Download from https://vulkan.lunarg.com/

### Compile Shaders

Run the compilation script:
```bash
./compile_shaders.sh
```

This will generate:
- `fullscreen.vert.spv` - Compiled vertex shader
- `nv12_to_rgb.frag.spv` - Compiled fragment shader

### Manual Compilation

If you prefer to compile manually:

```bash
# Vertex shader
glslangValidator -V fullscreen.vert -o fullscreen.vert.spv

# Fragment shader
glslangValidator -V nv12_to_rgb.frag -o nv12_to_rgb.frag.spv
```

Or with glslc:
```bash
glslc fullscreen.vert -o fullscreen.vert.spv
glslc nv12_to_rgb.frag -o nv12_to_rgb.frag.spv
```

## Integration

The compiled SPIR-V shaders (.spv files) are loaded at runtime by the Vulkan renderer. They should be:
1. Compiled during the build process
2. Installed to the appropriate shader directory
3. Loaded by the renderer at initialization

## Shader Pipeline

```
Vertex Shader (fullscreen.vert)
  ↓
  Generates fullscreen quad
  Outputs texture coordinates
  ↓
Fragment Shader (nv12_to_rgb.frag)
  ↓
  Samples Y and UV textures
  Converts YUV → RGB (BT.709)
  Outputs final color
  ↓
Framebuffer (swapchain image)
```

## Debugging

To validate shader compilation:
```bash
# Check for errors
glslangValidator -V fullscreen.vert

# Disassemble SPIR-V
spirv-dis fullscreen.vert.spv
```

To enable validation layers in the renderer, set:
```c
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
```

## Performance

- Vertex shader: Minimal cost (4 vertices, no transform)
- Fragment shader: One texture lookup per plane, simple math
- Expected: <1ms GPU time at 1080p60

## Future Enhancements

- [ ] Add HDR support (BT.2020 color space)
- [ ] Add chroma upsampling options
- [ ] Add color correction uniforms
- [ ] Add scaling quality options (bicubic)
- [ ] Add post-processing effects (sharpening, etc.)
