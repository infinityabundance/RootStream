# Vulkan Video Renderer - Phase 12

## Overview

The Vulkan renderer is a fallback video rendering backend for the RootStream client, providing an alternative to the OpenGL renderer. It supports three display backends with automatic fallback:

1. **Wayland (Primary)** - Modern Linux compositors
2. **X11 (Fallback)** - Traditional X11 display servers
3. **Headless (Final Fallback)** - For CI/testing without display

## Architecture

```
┌─────────────────────────────────────────────────────┐
│         VideoRenderer Abstraction Layer             │
│  (renderer.h - Same API for OpenGL & Vulkan)        │
└──────────────┬──────────────────────────────────────┘
               │
      ┌────────┴────────┐
      │                 │
      ▼                 ▼
 ┌─────────────┐  ┌──────────────┐
 │   OpenGL    │  │   Vulkan     │
 │ (Phase 11)  │  │ (Phase 12)   │
 └─────────────┘  └──────┬───────┘
                         │
          ┌──────────────┼──────────────┐
          │              │              │
          ▼              ▼              ▼
      ┌────────┐  ┌────────┐  ┌──────────────┐
      │ Wayland│  │  X11   │  │   Headless   │
      │(primary)  │(fallback)  │ (final mode) │
      └────────┘  └────────┘  └──────────────┘
```

## Files

### Core Implementation
- `vulkan_renderer.h` - Vulkan renderer API
- `vulkan_renderer.c` - Core Vulkan implementation with backend detection
- `renderer.c` - Updated to support both OpenGL and Vulkan backends

### Backend-Specific
- `vulkan_wayland.h/c` - Wayland surface integration
- `vulkan_x11.h/c` - X11 surface integration
- `vulkan_headless.h/c` - Headless/offscreen rendering

### Tests
- `tests/unit/test_vulkan_renderer.cpp` - Vulkan renderer unit tests

## Building

### Prerequisites

**Required:**
- CMake 3.16+
- C11 compiler (GCC/Clang)
- Qt6 (Core, Gui, Qml, Quick, Widgets)

**Optional (for Vulkan renderer):**
- Vulkan SDK 1.0+
- Vulkan headers and libraries
- Wayland client libraries (for Wayland backend)
- X11 libraries (for X11 backend)

### Build Options

```bash
cmake -DENABLE_RENDERER_VULKAN=ON ..
make
```

The Vulkan renderer is enabled by default but will gracefully degrade if:
- Vulkan headers are not available at compile time
- Vulkan runtime is not available on the system
- No Vulkan-capable GPU is present

### Testing

```bash
ctest -R test_vulkan_renderer
```

## Backend Detection

The renderer automatically detects the best available backend at runtime:

1. **Wayland Detection**: Attempts `wl_display_connect(NULL)`
2. **X11 Detection**: Attempts `XOpenDisplay(NULL)`
3. **Headless Fallback**: Used if both fail

This ensures maximum compatibility across different Linux environments.

## API Usage

### Creating a Vulkan Renderer

```c
// Create renderer with Vulkan backend
renderer_t *renderer = renderer_create(RENDERER_VULKAN, 1920, 1080);

// Or use auto-detection (prefers OpenGL, falls back to Vulkan)
renderer_t *renderer = renderer_create(RENDERER_AUTO, 1920, 1080);

// Initialize with native window (or NULL for headless)
renderer_init(renderer, native_window);
```

### Submitting Frames

```c
frame_t frame = {
    .data = frame_data,
    .size = frame_size,
    .width = 1920,
    .height = 1080,
    .format = FRAME_FORMAT_NV12,  // Use constant instead of magic number
    .timestamp_us = timestamp,
    .is_keyframe = true
};

renderer_submit_frame(renderer, &frame);
```

### Presenting Frames

```c
// Called from render loop
renderer_present(renderer);
```

### Cleanup

```c
renderer_cleanup(renderer);
```

## Implementation Status

### ✅ Completed
- [x] Core Vulkan renderer structure
- [x] Backend detection logic (Wayland → X11 → Headless)
- [x] Integration with renderer abstraction layer
- [x] CMake build system updates
- [x] Basic unit tests
- [x] Conditional compilation for missing headers
- [x] Fallback type definitions

### 🚧 In Progress (Future Work)
- [ ] Wayland surface creation and presentation
- [ ] X11 surface creation and presentation
- [ ] Headless offscreen rendering
- [ ] Vulkan swapchain management
- [ ] NV12 → RGB shader conversion
- [ ] Frame upload/render pipeline
- [ ] Performance optimization
- [ ] Additional tests

## Conditional Compilation

The code is designed to compile even without Vulkan headers:

```c
#if __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#define HAVE_VULKAN_HEADERS 1
#else
// Fallback type definitions
typedef void* VkInstance;
typedef void* VkDevice;
// etc.
#endif
```

This ensures the codebase remains buildable in environments without Vulkan SDK.

## Performance Targets

- **Frame Rate**: 60 FPS @ 1080p
- **GPU Upload**: < 5ms
- **Frame Latency**: < 8ms total
- **Memory**: < 200MB GPU memory
- **CPU Overhead**: < 10% of frame time

## Error Handling

All functions return appropriate error codes:
- `0` on success
- `-1` on failure

Error messages can be retrieved via:
```c
const char *error = renderer_get_error(renderer);
```

## Backend-Specific Notes

### Wayland
- Uses `VK_KHR_wayland_surface` extension
- Zero-copy via dmabuf (if supported)
- Automatic vsync via Wayland frame callbacks

### X11
- Uses `VK_KHR_xlib_surface` or `VK_KHR_xcb_surface`
- DRI3/Present for optimal performance
- MIT-SHM fallback for software rendering

### Headless
- No display server required
- Renders to offscreen buffer
- Suitable for CI/testing
- Supports frame readback for validation

## Troubleshooting

### Vulkan Not Available

If Vulkan is not available, the renderer will fail gracefully:

```
Failed to initialize Vulkan backend
```

The application can fall back to OpenGL or display an error.

### No Vulkan-Capable GPU

The renderer will attempt to use any Vulkan-capable device, preferring discrete GPUs. If none are found:

```
No Vulkan-capable GPUs found
```

### Backend Detection Issues

Enable verbose logging to see which backend was detected:

```c
vulkan_backend_t backend = vulkan_detect_backend();
const char *backend_name = vulkan_get_backend_name(ctx);
printf("Using backend: %s\n", backend_name);
```

## Future Enhancements

1. **Complete Backend Implementation**
   - Full Wayland surface integration
   - Full X11 surface integration
   - Complete headless rendering

2. **Shader Pipeline**
   - NV12 → RGB compute shader
   - Color space correction (BT.709)
   - HDR support

3. **Performance Optimization**
   - Async compute for format conversion
   - Descriptor set pooling
   - Pipeline caching
   - Memory aliasing

4. **Additional Features**
   - Multi-GPU support
   - Adaptive sync (VRR/FreeSync)
   - HDR output
   - 10-bit color depth

## Contributing

When contributing to the Vulkan renderer:

1. Ensure code compiles with and without Vulkan headers
2. Test on multiple backends (Wayland, X11, headless)
3. Add unit tests for new functionality
4. Update documentation
5. Follow existing code style

## References

- [Vulkan Specification](https://www.khronos.org/vulkan/)
- [Wayland Protocol](https://wayland.freedesktop.org/)
- [X11 Documentation](https://www.x.org/wiki/)
- [RootStream Phase 11 - OpenGL Renderer](../docs/phase11-opengl-renderer.md)

## License

This code is part of the RootStream project. See the main repository for license information.
