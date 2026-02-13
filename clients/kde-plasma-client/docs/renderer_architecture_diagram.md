# VideoRenderer Architecture Diagram

## High-Level Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                      RootStream Application                       │
│                    (Qt/QML KDE Plasma Client)                    │
└────────────────────────────┬─────────────────────────────────────┘
                             │
                             │ renderer_*() API
                             │
┌────────────────────────────▼─────────────────────────────────────┐
│                    Renderer Abstraction Layer                     │
│                      (renderer.h/renderer.c)                      │
│                                                                    │
│  • Factory Pattern (RENDERER_OPENGL/VULKAN/PROTON/AUTO)         │
│  • Lifecycle Management (create, init, present, cleanup)          │
│  • Frame Queue Management                                         │
│  • Performance Metrics (FPS, latency, drops)                      │
│  • Error Handling                                                 │
└────────────────────────────┬─────────────────────────────────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
    ┌─────────▼────────┐ ┌──▼───────────┐ ┌▼──────────────┐
    │   OpenGL Backend │ │Vulkan Backend│ │Proton Backend │
    │  (Phase 11) ✅   │ │  (Phase 12)  │ │  (Phase 13)   │
    └──────────────────┘ └──────────────┘ └───────────────┘
```

## OpenGL Renderer Data Flow

```
┌─────────────────┐
│  Network Stream │ (Decoder Thread)
│   (NV12 frames) │
└────────┬────────┘
         │
         │ Decoded Frame Data
         │
┌────────▼────────────────────────────────────────────────────┐
│                    Frame Buffer (Ring Buffer)                │
│                 (frame_buffer.h/frame_buffer.c)              │
│                                                              │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                       │
│  │Frame0│→│Frame1│→│Frame2│→│Frame3│  (4 frame capacity)   │
│  └──────┘ └──────┘ └──────┘ └──────┘                       │
│                                                              │
│  • Thread-safe (pthread mutex)                              │
│  • Automatic frame dropping on overflow                     │
│  • Copy-on-enqueue, zero-copy dequeue                       │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        │ Dequeue Frame (Render Thread)
                        │
┌───────────────────────▼──────────────────────────────────────┐
│                   OpenGL Renderer                             │
│           (opengl_renderer.h/opengl_renderer.c)              │
│                                                              │
│  ┌─────────────────────────────────────────────────┐        │
│  │          1. Texture Upload                      │        │
│  │  ┌──────────┐         ┌──────────┐              │        │
│  │  │ Y Plane  │ GL_R8   │ UV Plane │ GL_RG8       │        │
│  │  │1920×1080 │────────→│ 960×540  │              │        │
│  │  └──────────┘         └──────────┘              │        │
│  │  (Full resolution)    (Half resolution)         │        │
│  └─────────────────────────────────────────────────┘        │
│                                                              │
│  ┌─────────────────────────────────────────────────┐        │
│  │      2. Shader-Based Color Conversion            │        │
│  │                                                  │        │
│  │  Vertex Shader: Fullscreen Quad                 │        │
│  │  ┌────────────────────────────────┐             │        │
│  │  │ position: (-1,-1) → (1,1)      │             │        │
│  │  │ texCoord: (0,0) → (1,1)        │             │        │
│  │  └────────────────────────────────┘             │        │
│  │                                                  │        │
│  │  Fragment Shader: NV12→RGB (BT.709)             │        │
│  │  ┌────────────────────────────────┐             │        │
│  │  │ Y  = texture(y_plane, uv)      │             │        │
│  │  │ UV = texture(uv_plane, uv)     │             │        │
│  │  │                                │             │        │
│  │  │ yuv.x = (Y - 0.0625) * 1.164   │             │        │
│  │  │ yuv.y = UV.r - 0.5             │             │        │
│  │  │ yuv.z = UV.g - 0.5             │             │        │
│  │  │                                │             │        │
│  │  │ rgb = yuv_to_rgb_matrix * yuv  │             │        │
│  │  │ rgb = clamp(rgb, 0.0, 1.0)     │             │        │
│  │  └────────────────────────────────┘             │        │
│  └─────────────────────────────────────────────────┘        │
│                                                              │
│  ┌─────────────────────────────────────────────────┐        │
│  │          3. Frame Presentation                   │        │
│  │  ┌──────────────────────────────┐               │        │
│  │  │ glXSwapBuffers()             │               │        │
│  │  │ (with VSync if enabled)      │               │        │
│  │  └──────────────────────────────┘               │        │
│  └─────────────────────────────────────────────────┘        │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        │ Display Output
                        │
                  ┌─────▼──────┐
                  │   Monitor   │
                  │  (60Hz+)    │
                  └────────────┘
```

## Component Relationships

```
renderer.c
    ├─→ frame_buffer.c (frame queuing)
    └─→ opengl_renderer.c
            ├─→ opengl_utils.c (shader/texture helpers)
            ├─→ color_space.c (BT.709 matrices)
            └─→ nv12_to_rgb.glsl (GPU shader)
```

## Thread Model

```
┌────────────────────┐
│  Decoder Thread    │
│                    │
│  ┌──────────────┐  │
│  │ Decode Frame │  │
│  └──────┬───────┘  │
│         │          │
│  ┌──────▼───────┐  │
│  │Submit Frame  │─────────┐
│  │(thread-safe) │  │      │
│  └──────────────┘  │      │
└────────────────────┘      │
                            │
                            │ Frame Buffer
                            │ (mutex protected)
                            │
┌────────────────────┐      │
│  Render Thread     │      │
│  (Qt Main Thread)  │      │
│                    │      │
│  ┌──────────────┐  │      │
│  │Present Frame │◄────────┘
│  │ (60 FPS)     │  │
│  └──────┬───────┘  │
│         │          │
│  ┌──────▼───────┐  │
│  │Upload to GPU │  │
│  └──────┬───────┘  │
│         │          │
│  ┌──────▼───────┐  │
│  │ Render Quad  │  │
│  └──────┬───────┘  │
│         │          │
│  ┌──────▼───────┐  │
│  │ Swap Buffers │  │
│  └──────────────┘  │
└────────────────────┘
```

## Memory Layout - NV12 Frame

```
┌─────────────────────────────────────────┐
│           Y Plane (Luminance)           │  width × height bytes
│                                         │
│  Y Y Y Y Y Y Y Y ... (each pixel)      │
│  Y Y Y Y Y Y Y Y ...                   │
│  ...                                    │
│                                         │
│  Size: 1920 × 1080 = 2,073,600 bytes  │
├─────────────────────────────────────────┤
│       UV Plane (Chrominance)           │  (width/2) × (height/2) × 2 bytes
│                                         │
│  U V U V U V U V ... (interleaved)     │
│  U V U V U V U V ...                   │
│  ...                                    │
│                                         │
│  Size: 960 × 540 × 2 = 1,036,800 bytes│
└─────────────────────────────────────────┘
Total: 3,110,400 bytes (1920×1080 @ NV12)
```

## Performance Pipeline

```
Frame Decode         Frame Upload        Shader Exec      Buffer Swap
    │                    │                   │                │
    ▼                    ▼                   ▼                ▼
┌─────────┐        ┌─────────┐        ┌─────────┐     ┌──────────┐
│  CPU    │        │CPU→GPU  │        │  GPU    │     │  VSync   │
│ 10-30ms │───────→│ 1-3ms   │───────→│ <0.5ms  │────→│ 16.7ms   │
└─────────┘        └─────────┘        └─────────┘     └──────────┘
                                                       (60 FPS)
  Decoding         Texture Upload     Color Convert   Presentation
```

## OpenGL State Machine

```
Initialization:
    Create GLX Context
    Load Function Pointers
    Compile Shaders
    Create VAO/VBO
    Create Textures (initial)

Per-Frame:
    Update Textures (if size changed)
    Upload Y plane → GL_R8 texture
    Upload UV plane → GL_RG8 texture
    Bind Shader Program
    Set Uniforms (sampler indices)
    Bind VAO
    Draw Quad (4 vertices, TRIANGLE_STRIP)
    Swap Buffers (with VSync)

Cleanup:
    Delete VAO/VBO
    Delete Textures
    Delete Shader Program
    Destroy GLX Context
```

## API Usage Example

```c
// 1. Create renderer
renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);

// 2. Initialize with window
Window window = ...; // from Qt
renderer_init(renderer, &window);

// 3. Configure
renderer_set_vsync(renderer, true);

// 4. Frame loop (decode thread)
while (streaming) {
    frame_t *frame = decode_frame();
    renderer_submit_frame(renderer, frame);
}

// 5. Render loop (main thread, 60 FPS)
while (rendering) {
    renderer_present(renderer);
    
    // Check performance
    struct renderer_metrics m = renderer_get_metrics(renderer);
    if (m.fps < 50) {
        fprintf(stderr, "Low FPS: %.2f\n", m.fps);
    }
}

// 6. Cleanup
renderer_cleanup(renderer);
```
