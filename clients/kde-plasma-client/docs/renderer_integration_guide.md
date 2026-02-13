# VideoRenderer Integration Guide

## Overview

The VideoRenderer provides a modular, backend-agnostic API for rendering video frames in the RootStream KDE client. This guide explains how to integrate the renderer into your application.

## Architecture

```
┌────────────────────────────────────────┐
│         Your Application               │
│  (Qt/QML, native window, etc.)        │
└─────────────┬──────────────────────────┘
              │
              │ renderer_*() API
              │
┌─────────────▼──────────────────────────┐
│      Renderer Abstraction Layer        │
│  (renderer.h / renderer.c)             │
└─────────────┬──────────────────────────┘
              │
              │ Backend selection
              │
    ┌─────────┴─────────┬─────────────┐
    │                   │             │
┌───▼────┐      ┌───────▼──┐   ┌─────▼────┐
│ OpenGL │      │ Vulkan   │   │ Proton   │
│Backend │      │ (Phase12)│   │(Phase 13)│
└────────┘      └──────────┘   └──────────┘
```

## Quick Start

### 1. Include Headers

```c
#include "renderer/renderer.h"
```

### 2. Create Renderer

```c
// Create renderer with OpenGL backend at 1920x1080
renderer_t *renderer = renderer_create(RENDERER_OPENGL, 1920, 1080);
if (!renderer) {
    fprintf(stderr, "Failed to create renderer\n");
    return -1;
}

// Or use auto-detection
renderer_t *renderer = renderer_create(RENDERER_AUTO, 1920, 1080);
```

### 3. Initialize with Window

```c
// Get native window handle (X11 Window)
Window window = ...; // from Qt: reinterpret_cast<Window>(winId())

// Initialize renderer
if (renderer_init(renderer, &window) != 0) {
    fprintf(stderr, "Failed to initialize renderer: %s\n", 
            renderer_get_error(renderer));
    renderer_cleanup(renderer);
    return -1;
}
```

### 4. Submit Frames

```c
// Create or receive NV12 frame
frame_t frame;
frame.width = 1920;
frame.height = 1080;
frame.format = 0x3231564E; // NV12 fourcc
frame.size = frame.width * frame.height * 3 / 2;
frame.data = /* your decoded frame data */;
frame.timestamp_us = /* presentation timestamp */;
frame.is_keyframe = true;

// Submit for rendering
if (renderer_submit_frame(renderer, &frame) != 0) {
    fprintf(stderr, "Failed to submit frame\n");
}
```

### 5. Present Frames

```c
// In your render loop (e.g., 60 FPS)
if (renderer_present(renderer) != 0) {
    fprintf(stderr, "Failed to present frame\n");
}
```

### 6. Monitor Performance

```c
struct renderer_metrics metrics = renderer_get_metrics(renderer);
printf("FPS: %.2f, Frame time: %.2fms, Dropped: %lu\n",
       metrics.fps, metrics.frame_time_ms, metrics.frames_dropped);
```

### 7. Cleanup

```c
renderer_cleanup(renderer);
```

## Qt/QML Integration

### QOpenGLWidget Integration

```cpp
class VideoWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent), renderer_(nullptr)
    {
    }

    ~VideoWidget() {
        makeCurrent();
        if (renderer_) {
            renderer_cleanup(renderer_);
        }
        doneCurrent();
    }

protected:
    void initializeGL() override {
        // Create renderer
        renderer_ = renderer_create(RENDERER_OPENGL, 1920, 1080);
        
        // Get X11 window handle
        Window window = reinterpret_cast<Window>(winId());
        
        // Initialize renderer
        if (renderer_init(renderer_, &window) != 0) {
            qWarning() << "Failed to initialize renderer:" 
                       << renderer_get_error(renderer_);
        }
    }

    void paintGL() override {
        // Present current frame
        renderer_present(renderer_);
    }

public slots:
    void onNewFrame(const QByteArray &frameData, int width, int height) {
        // Convert to frame_t
        frame_t frame;
        frame.width = width;
        frame.height = height;
        frame.format = 0x3231564E; // NV12
        frame.size = frameData.size();
        frame.data = (uint8_t*)frameData.constData();
        frame.timestamp_us = QDateTime::currentMSecsSinceEpoch() * 1000;
        frame.is_keyframe = false;
        
        // Submit frame
        renderer_submit_frame(renderer_, &frame);
        
        // Trigger repaint
        update();
    }

private:
    renderer_t *renderer_;
};
```

### Qt Quick Scene Graph Integration

```cpp
class VideoNode : public QSGGeometryNode
{
public:
    VideoNode() {
        renderer_ = renderer_create(RENDERER_OPENGL, 1920, 1080);
        // ... initialize
    }

    ~VideoNode() {
        renderer_cleanup(renderer_);
    }

    void render() {
        renderer_present(renderer_);
    }

private:
    renderer_t *renderer_;
};
```

## Configuration

### Vsync Control

```c
// Enable vsync (limits to monitor refresh rate)
renderer_set_vsync(renderer, true);

// Disable vsync (for benchmarking)
renderer_set_vsync(renderer, false);
```

### Window Resize

```c
// Handle window resize
void onResize(int new_width, int new_height) {
    renderer_resize(renderer, new_width, new_height);
}
```

### Fullscreen Toggle

```c
// Set fullscreen mode
renderer_set_fullscreen(renderer, true);

// Return to windowed mode
renderer_set_fullscreen(renderer, false);
```

## Frame Formats

Currently supported:
- **NV12** (0x3231564E): Most common, hardware-accelerated

Future support:
- **I420/YV12**: Planar YUV
- **RGBA**: Direct RGB (no conversion)

## Performance Tuning

### Optimal Frame Rate

```c
// Target 60 FPS
const uint64_t frame_interval_us = 16666; // ~60 FPS

void render_loop() {
    uint64_t last_time = get_current_time_us();
    
    while (running) {
        uint64_t now = get_current_time_us();
        if (now - last_time >= frame_interval_us) {
            renderer_present(renderer);
            last_time = now;
        }
    }
}
```

### Monitor Performance

```c
void check_performance() {
    struct renderer_metrics metrics = renderer_get_metrics(renderer);
    
    if (metrics.fps < 50.0) {
        fprintf(stderr, "Warning: Low FPS (%.2f)\n", metrics.fps);
    }
    
    if (metrics.gpu_upload_ms > 5.0) {
        fprintf(stderr, "Warning: High GPU upload time (%.2fms)\n", 
                metrics.gpu_upload_ms);
    }
    
    if (metrics.frames_dropped > 0) {
        fprintf(stderr, "Warning: Dropped %lu frames\n", 
                metrics.frames_dropped);
    }
}
```

## Error Handling

```c
// Always check return values
if (renderer_submit_frame(renderer, &frame) != 0) {
    const char *error = renderer_get_error(renderer);
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
    }
}
```

## Thread Safety

The renderer is designed for multi-threaded use:

- **Frame submission** (`renderer_submit_frame`): Thread-safe, can be called from decoder thread
- **Frame presentation** (`renderer_present`): Must be called from render thread
- **Configuration** (`renderer_set_*`): Should be called from render thread

```c
// Decoder thread
void decoder_thread() {
    while (decoding) {
        frame_t *frame = decode_next_frame();
        renderer_submit_frame(renderer, frame);
    }
}

// Render thread (e.g., Qt main thread)
void render_thread() {
    while (rendering) {
        renderer_present(renderer);
        sleep_ms(16); // ~60 FPS
    }
}
```

## Troubleshooting

### Black Screen

```c
// Check if frames are being submitted
struct renderer_metrics metrics = renderer_get_metrics(renderer);
if (metrics.total_frames == 0) {
    fprintf(stderr, "No frames submitted\n");
}

// Check for errors
const char *error = renderer_get_error(renderer);
if (error) {
    fprintf(stderr, "Renderer error: %s\n", error);
}
```

### Poor Performance

1. **Check GPU upload time**: Should be <5ms
2. **Enable vsync**: Prevents tearing
3. **Reduce resolution**: Try 720p instead of 1080p
4. **Check frame drops**: May indicate buffer overflow

### Compilation Issues

```cmake
# Ensure OpenGL is enabled
cmake -DENABLE_RENDERER_OPENGL=ON ..

# Check dependencies
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
```

## Examples

See `tests/unit/test_renderer.cpp` for complete examples of:
- Renderer initialization
- Frame submission
- Performance monitoring
- Error handling

## API Reference

See `renderer.h` for complete API documentation.
