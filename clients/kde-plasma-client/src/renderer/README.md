# VideoRenderer - OpenGL Implementation

## Overview

High-performance video renderer for RootStream with OpenGL 3.3+ backend. Provides NV12→RGB color space conversion using GPU shaders and achieves 60+ FPS @ 1080p.

## Directory Structure

```
renderer/
├── renderer.h              # Public API (abstraction layer)
├── renderer.c              # Renderer lifecycle and backend management
├── opengl_renderer.h       # OpenGL backend interface
├── opengl_renderer.c       # OpenGL implementation (GLX, textures, rendering)
├── opengl_utils.h          # OpenGL utility functions
├── opengl_utils.c          # Shader compilation, texture management
├── color_space.h           # Color space conversion interface
├── color_space.c           # BT.709 YUV→RGB matrices
├── frame_buffer.h          # Frame queue interface
├── frame_buffer.c          # Thread-safe ring buffer
└── shader/
    └── nv12_to_rgb.glsl    # GPU shader for color conversion
```

## Quick Start

### Include Header

```c
#include "renderer/renderer.h"
```

### Basic Usage

```c
// 1. Create renderer
renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);

// 2. Initialize with native window
Window window = ...; // X11 window handle
if (renderer_init(renderer, &window) != 0) {
    fprintf(stderr, "Failed to init: %s\n", renderer_get_error(renderer));
    return -1;
}

// 3. Submit frames (from decoder thread)
frame_t frame = {
    .width = 1920,
    .height = 1080,
    .format = 0x3231564E,  // NV12 fourcc
    .size = 1920 * 1080 * 3 / 2,
    .data = /* NV12 frame data */,
    .timestamp_us = /* presentation time */,
    .is_keyframe = false
};
renderer_submit_frame(renderer, &frame);

// 4. Present frames (from render thread, e.g., 60 FPS)
renderer_present(renderer);

// 5. Monitor performance
struct renderer_metrics metrics = renderer_get_metrics(renderer);
printf("FPS: %.2f, Frame time: %.2fms\n", metrics.fps, metrics.frame_time_ms);

// 6. Cleanup
renderer_cleanup(renderer);
```

## Features

### Supported Backends
- ✅ **OpenGL 3.3+** (Phase 11)
- 🔜 **Vulkan** (Phase 12)
- 🔜 **Proton** (Phase 13)
- ✅ **Auto-detect** (selects best available)

### Pixel Formats
- ✅ **NV12** (YUV 4:2:0, most common)
- 🔜 **I420/YV12** (planar YUV)
- 🔜 **RGBA** (direct RGB, no conversion)

### Performance
- **Frame Rate**: 60+ FPS @ 1080p
- **Frame Time**: 1.5-4ms (upload + render + present)
- **GPU Upload**: 1-3ms (memory bandwidth limited)
- **Shader Exec**: <0.5ms (GPU compute)
- **Memory**: <100MB (textures + buffers)

### Color Space
- **Standard**: BT.709 (Rec. 709)
- **Range**: Limited (Y: 16-235, UV: 16-240)
- **Conversion**: GPU shader (hardware-accelerated)

### Thread Safety
- ✅ `renderer_submit_frame()`: Thread-safe (decoder thread)
- ❌ `renderer_present()`: Single-threaded (render thread)
- ❌ `renderer_set_*()`: Single-threaded (render thread)

## API Reference

### Types

```c
// Opaque renderer handle
typedef struct renderer_s renderer_t;

// Video frame
typedef struct frame_s {
    uint8_t *data;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t format;        // DRM fourcc
    uint64_t timestamp_us;
    bool is_keyframe;
} frame_t;

// Backend selection
typedef enum {
    RENDERER_OPENGL,
    RENDERER_VULKAN,
    RENDERER_PROTON,
    RENDERER_AUTO
} renderer_backend_t;

// Performance metrics
struct renderer_metrics {
    double fps;
    double frame_time_ms;
    double gpu_upload_ms;
    uint64_t frames_dropped;
    uint64_t total_frames;
};
```

### Core Functions

| Function | Description |
|----------|-------------|
| `renderer_create()` | Create renderer instance |
| `renderer_init()` | Initialize with native window |
| `renderer_submit_frame()` | Submit frame for rendering (thread-safe) |
| `renderer_present()` | Present current frame to display |
| `renderer_cleanup()` | Destroy renderer |

### Configuration

| Function | Description |
|----------|-------------|
| `renderer_set_vsync()` | Enable/disable vertical sync |
| `renderer_set_fullscreen()` | Toggle fullscreen mode |
| `renderer_resize()` | Update rendering dimensions |

### Monitoring

| Function | Description |
|----------|-------------|
| `renderer_get_metrics()` | Get performance metrics |
| `renderer_get_error()` | Get last error message |

## Building

### Requirements
- OpenGL 3.3+
- X11 (libX11)
- GLX 1.3+
- pthreads

### CMake
```bash
cmake -DENABLE_RENDERER_OPENGL=ON ..
make
```

### Manual Compilation
```bash
gcc -c renderer.c opengl_renderer.c opengl_utils.c \
        color_space.c frame_buffer.c \
        -I. -DHAVE_OPENGL_RENDERER

gcc -o librenderer.a *.o -lGL -lX11 -lpthread
```

## Testing

### Unit Tests
```bash
cd build
ctest --output-on-failure -R test_renderer
```

### Generate Test Fixtures
```bash
cd tests/fixtures
./generate_test_frames.py
```

## Documentation

Detailed documentation available in `docs/`:
- **Integration Guide**: `renderer_integration_guide.md`
- **Color Space**: `color_space_conversion.md`
- **Architecture**: `renderer_architecture_diagram.md`
- **Summary**: `PHASE11_IMPLEMENTATION_SUMMARY.md`

## Performance Tuning

### VSync Control
```c
// Disable for benchmarking
renderer_set_vsync(renderer, false);

// Enable for tear-free display
renderer_set_vsync(renderer, true);
```

### Monitor Dropped Frames
```c
struct renderer_metrics metrics = renderer_get_metrics(renderer);
if (metrics.frames_dropped > 0) {
    fprintf(stderr, "Warning: %lu frames dropped\n", 
            metrics.frames_dropped);
}
```

### Check GPU Upload Time
```c
if (metrics.gpu_upload_ms > 5.0) {
    fprintf(stderr, "Warning: Slow GPU upload (%.2fms)\n",
            metrics.gpu_upload_ms);
}
```

## Troubleshooting

### Black Screen
- Check if frames are being submitted: `metrics.total_frames > 0`
- Verify OpenGL context creation succeeded
- Check error message: `renderer_get_error()`

### Low FPS
- Verify GPU supports OpenGL 3.3+
- Check for frame drops: `metrics.frames_dropped`
- Monitor GPU upload time: `metrics.gpu_upload_ms`
- Try lower resolution (720p instead of 1080p)

### Compilation Errors
- Ensure OpenGL headers installed: `libgl1-mesa-dev`
- Ensure X11 headers installed: `libx11-dev`
- Check CMake finds dependencies: `cmake -DENABLE_RENDERER_OPENGL=ON ..`

## Known Limitations

1. **X11 Only**: GLX-based, no Wayland support yet
2. **Single Window**: One renderer per window
3. **NV12 Only**: Other formats not yet supported
4. **No Zero-Copy**: Manual texture upload (VA-API integration planned)

## Future Enhancements

### Phase 12: Vulkan Backend
- Modern API with lower overhead
- Better multi-threading support
- Compute shader optimizations

### Phase 13: Proton Backend
- Windows game compatibility
- DirectX translation
- Enhanced streaming features

### Additional Features
- Wayland support (EGL contexts)
- Hardware decode integration (VA-API)
- Zero-copy texture sharing
- HDR support
- Multiple pixel formats (I420, RGBA, etc.)

## License

See main project LICENSE file.

## Contributing

See main project CONTRIBUTING.md for guidelines.
