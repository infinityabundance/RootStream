# Quick Start: Phase 26 Implementation
**Next Phase:** Complete KDE Plasma Client  
**Priority:** CRITICAL  
**Status:** Ready to begin

---

## What to Build

The KDE Plasma client exists as a framework but **all core functionality is stubbed out**. Phase 26 makes it actually work.

---

## Goals

By the end of Phase 26:
- ✅ Client connects to host and displays video
- ✅ Audio plays in sync with video  
- ✅ Keyboard and mouse input works end-to-end
- ✅ Works on both X11 and Wayland
- ✅ Measured latency < 30ms on LAN

---

## Week-by-Week Plan

### Week 1: Vulkan Core + X11
**Days 1-2:** Vulkan initialization and surface creation
- Initialize Vulkan instance
- Create logical device and queues
- Set up X11 surface (VK_KHR_xlib_surface)

**Days 3-4:** Swapchain and command buffers
- Create swapchain with optimal settings
- Allocate command pools and buffers
- Set up synchronization (fences, semaphores)

**Day 5:** Frame upload
- Implement `vulkan_renderer_upload_frame()`
- Convert YUV/RGB formats to GPU textures
- Test with sample frames

---

### Week 2: Rendering + Audio
**Days 1-2:** Render pipeline
- Create render pass
- Implement `vulkan_renderer_render()`
- Implement `vulkan_renderer_present()`
- Test end-to-end video display

**Days 3-4:** Audio playback
- Integrate PipeWire audio backend
- Implement audio/video sync logic
- Add buffer management

**Day 5:** Integration testing
- Test video + audio together
- Measure A/V sync accuracy
- Fix timing issues

---

### Week 3: Input + Wayland
**Days 1-2:** Input handling
- Capture keyboard events
- Capture mouse events (relative + absolute)
- Send input packets to host

**Days 3-4:** Wayland backend
- Implement Wayland surface creation
- Handle Wayland protocol differences
- Test on KDE Plasma Wayland

**Day 5:** Polish and testing
- Add error handling
- Add logging/debugging
- Performance profiling

---

## Files to Modify

### Critical Files (Must Implement)
```
clients/kde-plasma-client/src/renderer/
├── vulkan_renderer.c          # Core renderer (350-420 lines to implement)
├── vulkan_x11.c               # X11 backend (~100 lines)
├── vulkan_wayland.c           # Wayland backend (~100 lines)
└── vulkan_headless.c          # Testing backend (~80 lines)
```

### Audio Files
```
clients/kde-plasma-client/src/audio/
├── audio_player.cpp           # PipeWire integration
└── audio_sync.cpp             # A/V synchronization
```

### Input Files
```
clients/kde-plasma-client/src/input/
├── input_manager.cpp          # Input capture
└── input_protocol.cpp         # Network protocol for input
```

---

## Dependencies Needed

### Build Dependencies
```bash
# Arch Linux
sudo pacman -S vulkan-headers vulkan-icd-loader vulkan-validation-layers \
               pipewire libpipewire qt6-base qt6-declarative

# Ubuntu/Debian  
sudo apt install libvulkan-dev vulkan-validationlayers-dev \
                 libpipewire-0.3-dev qt6-base-dev qt6-declarative-dev

# Fedora
sudo dnf install vulkan-headers vulkan-loader-devel vulkan-validation-layers-devel \
                 pipewire-devel qt6-qtbase-devel qt6-qtdeclarative-devel
```

### Runtime Dependencies
```bash
# Arch Linux
sudo pacman -S vulkan-icd-loader pipewire

# Ubuntu/Debian
sudo apt install libvulkan1 pipewire

# Fedora  
sudo dnf install vulkan-loader pipewire
```

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│  Network Layer (existing)                           │
│  - Receives encrypted video/audio packets          │
│  - Decrypts with ChaCha20-Poly1305                 │
└────────────┬────────────────────────────────────────┘
             │
             ├──→ Video packets
             │    │
             │    ▼
             │   ┌──────────────────────────────────┐
             │   │ VA-API Decoder (existing)        │
             │   │ - Decodes H.264 to YUV frames    │
             │   └──────────┬───────────────────────┘
             │              │
             │              ▼
             │   ┌──────────────────────────────────┐
             │   │ Vulkan Renderer (TO IMPLEMENT)   │
             │   │ - Upload YUV to GPU texture      │
             │   │ - Convert YUV → RGB in shader    │
             │   │ - Render to swapchain            │
             │   │ - Present to X11/Wayland surface │
             │   └──────────────────────────────────┘
             │
             └──→ Audio packets
                  │
                  ▼
                 ┌──────────────────────────────────┐
                 │ Opus Decoder (existing)          │
                 │ - Decodes Opus to PCM            │
                 └──────────┬───────────────────────┘
                            │
                            ▼
                 ┌──────────────────────────────────┐
                 │ PipeWire Player (TO IMPLEMENT)   │
                 │ - Output PCM to speakers         │
                 │ - Sync with video timestamps     │
                 └──────────────────────────────────┘
```

---

## Key Implementation Details

### 1. Vulkan Renderer Initialization
**Function:** `vulkan_renderer_init()`  
**Location:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c:350`

**Steps:**
1. Create Vulkan instance (`vkCreateInstance`)
2. Select physical device (`vkEnumeratePhysicalDevices`)
3. Create logical device (`vkCreateDevice`)
4. Get queue handles (`vkGetDeviceQueue`)
5. Create command pool (`vkCreateCommandPool`)

**References:**
- [Vulkan Tutorial](https://vulkan-tutorial.com/Drawing_a_triangle/Setup)
- [Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)

---

### 2. Surface Creation (X11)
**Function:** `vulkan_x11_create_surface()`  
**Location:** `clients/kde-plasma-client/src/renderer/vulkan_x11.c:87`

**Steps:**
1. Connect to X11 display (`XOpenDisplay`)
2. Create X11 window (`XCreateWindow`)
3. Create Vulkan surface (`vkCreateXlibSurfaceKHR`)

**Key Considerations:**
- Handle window events (resize, close)
- Set window hints for optimal performance
- Support fullscreen mode

---

### 3. Swapchain Creation
**Function:** `vulkan_renderer_create_swapchain()`  
**Location:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c:352`

**Steps:**
1. Query surface capabilities
2. Choose surface format (prefer B8G8R8A8_SRGB)
3. Choose present mode (prefer MAILBOX for low latency)
4. Create swapchain (`vkCreateSwapchainKHR`)
5. Get swapchain images

**Latency Optimization:**
- Use `VK_PRESENT_MODE_MAILBOX_KHR` if available (triple buffering)
- Fall back to `VK_PRESENT_MODE_IMMEDIATE_KHR` (tearing but lowest latency)
- Avoid `VK_PRESENT_MODE_FIFO_KHR` (adds vsync latency)

---

### 4. Frame Upload
**Function:** `vulkan_renderer_upload_frame()`  
**Location:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c:386`

**Input:** YUV420 or RGB frame from decoder  
**Output:** GPU texture ready for rendering

**Steps:**
1. Create staging buffer
2. Copy frame data to staging buffer
3. Transition image layout to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
4. Copy buffer to image (`vkCmdCopyBufferToImage`)
5. Transition to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`

**Optimization:**
- Reuse staging buffers (ring buffer)
- Use async transfer queue if available
- Consider using VK_EXT_external_memory_dma_buf for zero-copy

---

### 5. Rendering
**Function:** `vulkan_renderer_render()`  
**Location:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c:392`

**Steps:**
1. Wait for fence from previous frame
2. Acquire next swapchain image
3. Record command buffer:
   - Begin render pass
   - Bind pipeline
   - Bind descriptor sets (texture)
   - Draw fullscreen quad
   - End render pass
4. Submit command buffer
5. Queue present

**Shader Pipeline:**
```glsl
// Vertex shader (fullscreen quad)
#version 450
layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = (positions[gl_VertexIndex] + 1.0) / 2.0;
}

// Fragment shader (YUV to RGB conversion)
#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texY;
layout(binding = 1) uniform sampler2D texU;
layout(binding = 2) uniform sampler2D texV;

void main() {
    float y = texture(texY, fragTexCoord).r;
    float u = texture(texU, fragTexCoord).r - 0.5;
    float v = texture(texV, fragTexCoord).r - 0.5;
    
    // BT.709 conversion
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;
    
    outColor = vec4(r, g, b, 1.0);
}
```

---

### 6. Audio/Video Synchronization
**Function:** `audio_sync_update()`  
**Location:** `clients/kde-plasma-client/src/audio/audio_sync.cpp`

**Algorithm:**
```cpp
// Each packet has a timestamp
uint64_t video_pts = video_packet->timestamp;
uint64_t audio_pts = audio_packet->timestamp;

// Calculate drift
int64_t drift_ms = (int64_t)video_pts - (int64_t)audio_pts;

// Adjust audio playback speed
if (drift_ms > 100) {
    // Video ahead, speed up audio slightly
    audio_speed = 1.05;
} else if (drift_ms < -100) {
    // Audio ahead, slow down audio slightly
    audio_speed = 0.95;
} else {
    // In sync
    audio_speed = 1.0;
}
```

---

### 7. Input Capture and Send
**Function:** `input_manager_send_event()`  
**Location:** `clients/kde-plasma-client/src/input/input_manager.cpp`

**Keyboard Event:**
```cpp
void send_keyboard_event(uint32_t keycode, bool pressed) {
    input_event_pkt_t pkt;
    pkt.type = INPUT_EVENT_KEYBOARD;
    pkt.keyboard.keycode = keycode;
    pkt.keyboard.pressed = pressed;
    pkt.timestamp = get_timestamp_ms();
    
    network_send(&pkt, sizeof(pkt));
}
```

**Mouse Event:**
```cpp
void send_mouse_event(int32_t dx, int32_t dy, uint32_t buttons) {
    input_event_pkt_t pkt;
    pkt.type = INPUT_EVENT_MOUSE;
    pkt.mouse.dx = dx;
    pkt.mouse.dy = dy;
    pkt.mouse.buttons = buttons;
    pkt.timestamp = get_timestamp_ms();
    
    network_send(&pkt, sizeof(pkt));
}
```

---

## Testing Strategy

### Unit Tests
```bash
cd clients/kde-plasma-client/build
ctest --output-on-failure
```

**Test Coverage:**
- [ ] Vulkan initialization
- [ ] Surface creation (X11, Wayland, headless)
- [ ] Frame upload and format conversion
- [ ] Render pipeline
- [ ] Audio playback
- [ ] Input capture

### Integration Tests
```bash
# Start host
./rootstream host --display 0

# Start client
./rootstream-client localhost
```

**Test Scenarios:**
- [ ] Video displays correctly
- [ ] Audio plays without crackling
- [ ] A/V sync within 50ms
- [ ] Input response time < 10ms
- [ ] Window resize works
- [ ] Fullscreen mode works
- [ ] Reconnection after disconnect

### Performance Tests
```bash
# Enable latency logging
./rootstream-client localhost --show-stats
```

**Metrics to Measure:**
- [ ] End-to-end latency (capture → display)
- [ ] Frame decode time
- [ ] Frame render time
- [ ] CPU usage (should be < 10%)
- [ ] GPU usage (varies by GPU)
- [ ] Memory usage (should be < 50MB)

---

## Common Issues and Solutions

### Issue 1: Black Screen
**Symptoms:** Client window opens but shows black

**Possible Causes:**
- Vulkan surface not created
- Swapchain not working
- Render pass not executing
- YUV textures not uploaded

**Debug Steps:**
```cpp
// Add debug logging
printf("Swapchain image count: %u\n", swapchain_image_count);
printf("Current image index: %u\n", image_index);
printf("Frame uploaded: %s\n", frame ? "yes" : "no");
```

### Issue 2: Audio Crackling
**Symptoms:** Audio plays but has pops/clicks

**Possible Causes:**
- Buffer underrun (audio buffer too small)
- A/V sync trying to catch up too aggressively
- PipeWire latency too high

**Solutions:**
- Increase audio buffer size
- Smooth A/V sync adjustments
- Reduce PipeWire latency setting

### Issue 3: Input Lag
**Symptoms:** Keyboard/mouse feels delayed

**Possible Causes:**
- Input events queued instead of sent immediately
- Network buffering
- Host-side input processing slow

**Solutions:**
- Send input events immediately (no batching)
- Use separate thread for input
- Prioritize input packets (send before video)

---

## Success Checklist

Before marking Phase 26 complete:

### Functionality
- [ ] Client connects to host successfully
- [ ] Video displays at target FPS (60fps)
- [ ] Audio plays without glitches
- [ ] A/V sync within 50ms
- [ ] Keyboard input works (typing, shortcuts)
- [ ] Mouse input works (cursor, clicks, wheel)
- [ ] Window can be resized
- [ ] Fullscreen mode works
- [ ] Works on X11
- [ ] Works on Wayland

### Performance
- [ ] Latency < 30ms on gigabit LAN
- [ ] CPU usage < 10% on modern hardware
- [ ] Memory usage < 50MB
- [ ] No memory leaks after 1 hour
- [ ] Stable for 4+ hours continuous use

### Code Quality
- [ ] All TODO comments removed from implemented functions
- [ ] Error handling added for all Vulkan calls
- [ ] Logging added for debugging
- [ ] Code reviewed by team
- [ ] Unit tests pass
- [ ] Integration tests pass

### Documentation
- [ ] Implementation notes added to ARCHITECTURE.md
- [ ] User guide updated with client usage
- [ ] Troubleshooting guide updated
- [ ] Known issues documented

---

## Next Steps After Phase 26

Once the client is complete:
1. **Phase 27:** Recording features (MP4/MKV, replay buffer)
2. **Phase 30:** Security fixes (password validation)
3. **Phase 32:** Testing and stability

---

## Resources

### Vulkan Learning
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)
- [Vulkan Guide](https://vkguide.dev/)

### PipeWire Audio
- [PipeWire Documentation](https://docs.pipewire.org/)
- [PipeWire Examples](https://gitlab.freedesktop.org/pipewire/pipewire/-/tree/master/src/examples)

### Qt/QML Integration
- [Qt Documentation](https://doc.qt.io/)
- [QML with Vulkan](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html)

---

**Last Updated:** February 14, 2026  
**Estimated Time:** 2-3 weeks  
**Status:** Ready to begin  
**Dependencies:** Vulkan SDK, PipeWire, Qt6
