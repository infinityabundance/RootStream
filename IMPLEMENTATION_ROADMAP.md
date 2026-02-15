# RootStream: Implementation Roadmap
**Date:** February 15, 2026  
**Purpose:** Phased plan to complete all TODO/stub implementations  
**Based on:** VERIFICATION_REPORT.md analysis

---

## Executive Summary

**Current State:**
- ✅ 39% of subsystems fully functional (host-side complete)
- ⚠️ 28% of subsystems partially working (client needs completion)
- ❌ 33% of subsystems are complete stubs (VR, web, mobile)

**Goal:** Achieve 100% functionality for documented features

**Timeline:** 27-33 weeks for full completion (phased approach)

---

## Phase 1: Critical Client Functionality (4-5 weeks) 🔴 HIGH PRIORITY

**Goal:** Make the KDE Plasma client actually work (rendering, audio, input)

### Week 1-2: Vulkan Renderer Core
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

#### Tasks:
1. **Implement Frame Upload** (Line 913)
   ```c
   // Current: Returns 0 immediately (stub)
   // Needed:
   // - Create staging buffer for YUV data
   // - Allocate device-local image
   // - Implement vkCmdCopyBufferToImage
   // - Add pipeline barrier for layout transitions
   ```
   - **Estimated Lines:** 200-250 LOC
   - **Time:** 3-4 days
   - **Dependencies:** Vulkan texture management

2. **Implement Rendering Pipeline** (Line 982)
   ```c
   // Current: TODO: Bind pipeline and draw
   // Needed:
   // - Create graphics pipeline
   // - Load YUV→RGB shaders (SPIR-V)
   // - Bind descriptor sets
   // - Issue draw commands
   ```
   - **Estimated Lines:** 250-300 LOC
   - **Time:** 4-5 days
   - **Dependencies:** Shader compilation, descriptor sets

3. **Implement Present Mode Switching** (Line 1071)
   ```c
   // Current: TODO: Recreate swapchain with new present mode
   // Needed:
   // - Save current state
   // - Destroy old swapchain
   // - Create new swapchain
   // - Rebuild command buffers
   ```
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1-2 days
   - **Dependencies:** Swapchain management

4. **Implement Window Resize** (Line 1082)
   ```c
   // Current: TODO: Recreate swapchain
   // Needed:
   // - Wait for device idle
   // - Destroy framebuffers
   // - Recreate swapchain
   // - Recreate framebuffers
   ```
   - **Estimated Lines:** 100-120 LOC
   - **Time:** 1-2 days
   - **Dependencies:** Swapchain management

5. **Create YUV→RGB Shaders**
   - Write vertex shader (GLSL)
   - Write fragment shader (GLSL) with YUV conversion
   - Compile to SPIR-V
   - Embed in binary or load from files
   - **Estimated Lines:** 150-200 LOC (including shaders)
   - **Time:** 2-3 days

**Deliverables:**
- [ ] Frames render on screen
- [ ] VSync toggle works
- [ ] Window resize works
- [ ] No memory leaks

**Testing:**
```bash
# Should show rendered video
./rootstream-client --connect <host> --backend vulkan
```

---

### Week 3: X11 Backend Implementation
**File:** `clients/kde-plasma-client/src/renderer/vulkan_x11.c`

#### Tasks:
1. **Implement X11 Initialization** (Line ~45)
   ```c
   // Current: Returns 0 (stub)
   // Needed:
   // - Open X11 display (XOpenDisplay)
   // - Get default screen
   // - Create window (XCreateWindow)
   // - Map window (XMapWindow)
   // - Set event mask
   // - Create Vulkan surface (vkCreateXlibSurfaceKHR)
   ```
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2-3 days
   - **Dependencies:** Xlib, Vulkan X11 extensions

2. **Implement X11 Surface Creation** (Line ~87)
   ```c
   // Current: Returns VK_NULL_HANDLE (stub)
   // Needed:
   // - Fill VkXlibSurfaceCreateInfoKHR
   // - Call vkCreateXlibSurfaceKHR
   // - Return surface handle
   ```
   - **Estimated Lines:** 30-40 LOC
   - **Time:** Half day
   - **Dependencies:** X11 context from init

3. **Implement X11 Event Handling**
   - Handle ConfigureNotify (resize)
   - Handle Expose (redraw)
   - Handle DestroyNotify (cleanup)
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

4. **Implement X11 Cleanup**
   - Destroy Vulkan surface
   - Destroy X11 window
   - Close X11 display
   - **Estimated Lines:** 40-50 LOC
   - **Time:** Half day

**Deliverables:**
- [ ] Client runs on X11
- [ ] Window creation works
- [ ] Event handling works
- [ ] Clean shutdown

**Testing:**
```bash
# On X11 desktop
DISPLAY=:0 ./rootstream-client --backend x11
```

---

### Week 4: Wayland Backend Implementation
**File:** `clients/kde-plasma-client/src/renderer/vulkan_wayland.c`

#### Tasks:
1. **Implement Wayland Initialization** (Line ~45)
   ```c
   // Current: Returns 0 (stub)
   // Needed:
   // - Connect to Wayland compositor (wl_display_connect)
   // - Get registry (wl_display_get_registry)
   // - Bind compositor, shell, seat interfaces
   // - Create Wayland surface (wl_compositor_create_surface)
   // - Create xdg_surface and xdg_toplevel
   // - Create Vulkan surface (vkCreateWaylandSurfaceKHR)
   ```
   - **Estimated Lines:** 200-250 LOC
   - **Time:** 3-4 days
   - **Dependencies:** wayland-client, xdg-shell

2. **Implement Wayland Surface Creation** (Line ~87)
   ```c
   // Current: Returns VK_NULL_HANDLE (stub)
   // Needed:
   // - Fill VkWaylandSurfaceCreateInfoKHR
   // - Call vkCreateWaylandSurfaceKHR
   // - Configure xdg_toplevel
   // - Return surface handle
   ```
   - **Estimated Lines:** 50-70 LOC
   - **Time:** 1 day
   - **Dependencies:** Wayland context from init

3. **Implement Wayland Event Handling**
   - Handle configure events (resize)
   - Handle close events
   - Handle frame callbacks
   - **Estimated Lines:** 100-120 LOC
   - **Time:** 1-2 days

4. **Implement Wayland Cleanup**
   - Destroy Vulkan surface
   - Destroy xdg_toplevel and xdg_surface
   - Destroy Wayland surface
   - Disconnect from compositor
   - **Estimated Lines:** 50-60 LOC
   - **Time:** Half day

**Deliverables:**
- [ ] Client runs on Wayland
- [ ] Window creation works
- [ ] Event handling works
- [ ] Clean shutdown

**Testing:**
```bash
# On Wayland session (KDE Plasma 6, GNOME)
./rootstream-client --backend wayland
```

---

### Week 5: Client Audio & Integration Testing
**File:** `clients/kde-plasma-client/src/audio_playback.c`

#### Tasks:
1. **Complete PipeWire Support**
   - Initialize PipeWire context
   - Create audio stream
   - Handle buffer callbacks
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2-3 days

2. **Complete PulseAudio Support**
   - Initialize PulseAudio simple API
   - Create playback stream
   - Handle buffer callbacks
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

3. **Improve Buffer Underrun Handling**
   - Detect underruns
   - Implement adaptive buffering
   - Add latency measurement
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

4. **Integration Testing**
   - Test video + audio sync
   - Test X11 + Wayland backends
   - Test different audio backends
   - Measure latency
   - **Time:** 1-2 days

**Deliverables:**
- [ ] Audio plays on PipeWire
- [ ] Audio plays on PulseAudio
- [ ] Audio/video sync < 50ms
- [ ] No audio crackling

**Testing:**
```bash
# Test audio backends
./rootstream-client --audio-backend pipewire
./rootstream-client --audio-backend pulseaudio
./rootstream-client --audio-backend alsa
```

---

## Phase 2: Recording & Container Formats (2-3 weeks) 🟡 MEDIUM PRIORITY

**Goal:** Complete recording system with standard formats

### Week 6-7: MP4 Container Support
**File:** New file `src/recording/mp4_muxer.cpp`

#### Tasks:
1. **Create MP4 Muxer Class**
   - Initialize libavformat for MP4
   - Create video stream (H.264)
   - Create audio stream (AAC or Opus)
   - Write file header
   - **Estimated Lines:** 200-250 LOC
   - **Time:** 2-3 days

2. **Implement Frame Muxing**
   - Write video packets with PTS/DTS
   - Write audio packets
   - Handle B-frames
   - Handle timestamp wraparound
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2 days

3. **Implement File Finalization**
   - Write MP4 trailer
   - Fix moov atom position (fast start)
   - Close file
   - **Estimated Lines:** 50-80 LOC
   - **Time:** 1 day

4. **Integrate with Recording Manager**
   - Add MP4 option to recording_manager.cpp
   - Add command-line flag `--format mp4`
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] Record to MP4 format
- [ ] Video plays in VLC/mpv
- [ ] Fast start enabled
- [ ] No corruption

**Testing:**
```bash
./rootstream-host --record output.mp4 --format mp4
vlc output.mp4  # Should play
```

---

### Week 7-8: Replay Buffer Completion
**Files:** 
- `src/recording/replay_buffer.cpp`
- `src/recording/recording_manager.cpp`

#### Tasks:
1. **Complete Audio Encoding** (Line 276 in replay_buffer.cpp)
   ```cpp
   // Current: TODO: Audio encoding not yet fully implemented
   // Needed:
   // - Initialize Opus encoder
   // - Encode audio chunks
   // - Mux with video
   // - Handle timestamp alignment
   ```
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2-3 days

2. **Complete Video Frame Encoding** (Line 235 in recording_manager.cpp)
   ```cpp
   // Current: TODO: Encode frame before adding to replay buffer
   // Needed:
   // - Hook into encoder output
   // - Add encoded frames to buffer
   // - Manage GOP boundaries
   ```
   - **Estimated Lines:** 100-120 LOC
   - **Time:** 1-2 days

3. **Implement Instant Replay Save**
   - Add hotkey support (e.g., F9)
   - Implement save_last_n_seconds()
   - Add UI notification
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

4. **Add Command-Line Interface**
   ```bash
   # Add these options:
   --replay-buffer-seconds 30
   --replay-save-key F9
   --replay-save-now
   ```
   - **Estimated Lines:** 50-80 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] Replay buffer saves with audio
- [ ] Hotkey saves last 30 seconds
- [ ] Output format is MP4
- [ ] No frame drops

**Testing:**
```bash
./rootstream-host --replay-buffer-seconds 30 --replay-save-key F9
# Press F9 → saves replay_TIMESTAMP.mp4
```

---

### Week 8: MKV Container & Chapter Support
**Files:** 
- New file `src/recording/mkv_muxer.cpp`
- `src/recording/recording_metadata.cpp`

#### Tasks:
1. **Create MKV Muxer Class**
   - Initialize libavformat for Matroska
   - Support multiple codecs (H.264, VP9, AV1)
   - Support multiple audio tracks
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2 days

2. **Implement Chapter Support** (Line 168 in recording_metadata.cpp)
   ```cpp
   // Current: TODO: Implement proper chapter support
   // Needed:
   // - Create chapter entries
   // - Add to MKV metadata
   // - Support chapter titles
   ```
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

3. **Add Chapter Hotkeys**
   - Add hotkey to create chapter (e.g., F8)
   - Add chapter title prompts
   - Display chapter count
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] Record to MKV format
- [ ] Chapters work in mpv
- [ ] Multiple audio tracks work
- [ ] VP9/AV1 codecs work

**Testing:**
```bash
./rootstream-host --record output.mkv --format mkv --codec vp9
mpv output.mkv  # Should show chapters
```

---

## Phase 3: Web Monitoring Infrastructure (2 weeks) 🟡 MEDIUM PRIORITY

**Goal:** Enable remote monitoring via web dashboard

### Week 9-10: API Server Implementation
**File:** `src/web/api_server.c`

#### Tasks:
1. **Integrate libmicrohttpd** (Line 52)
   ```c
   // Current: TODO: Implement route registration
   // Needed:
   // - Initialize MHD_Daemon
   // - Register request handler
   // - Parse HTTP methods
   // - Return JSON responses
   ```
   - **Estimated Lines:** 200-250 LOC
   - **Time:** 2-3 days

2. **Implement Route Registration** (Line 66)
   ```c
   // Current: Just prints "started"
   // Needed:
   // - Start MHD_start_daemon
   // - Configure threading mode
   // - Set port and options
   // - Handle connection callbacks
   ```
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2 days

3. **Implement API Endpoints**
   - GET /api/status → system status
   - GET /api/metrics → current metrics
   - POST /api/auth/login → authentication
   - POST /api/recording/start → start recording
   - POST /api/recording/stop → stop recording
   - **Estimated Lines:** 300-400 LOC
   - **Time:** 3-4 days

4. **Add CORS Support**
   - Handle OPTIONS requests
   - Add CORS headers
   - Configure allowed origins
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] API server listens on port 8080
- [ ] All endpoints return JSON
- [ ] Authentication works
- [ ] CORS enabled

**Testing:**
```bash
# Start server
./rootstream-host --api-port 8080

# Test endpoints
curl http://localhost:8080/api/status
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"pass"}'
```

---

### Week 10: WebSocket Server Implementation
**File:** `src/web/websocket_server.c`

#### Tasks:
1. **Integrate libwebsockets** (Line 53)
   ```c
   // Current: TODO: Initialize libwebsockets context
   // Needed:
   // - Create lws_context
   // - Define protocols
   // - Set up event loop
   // - Handle connections
   ```
   - **Estimated Lines:** 200-250 LOC
   - **Time:** 2-3 days

2. **Implement Metrics Broadcasting** (Line 89)
   ```c
   // Current: Just prints to stdout
   // Needed:
   // - Format metrics as JSON
   // - Track connected clients
   // - Broadcast to all clients
   // - Handle disconnections
   ```
   - **Estimated Lines:** 150-200 LOC
   - **Time:** 2 days

3. **Implement Event Broadcasting** (Line 111)
   ```c
   // Current: Just prints to stdout
   // Needed:
   // - Format events as JSON
   // - Add event types (recording_start, client_connect, etc.)
   // - Broadcast immediately
   ```
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

4. **Add Ping/Pong Keep-Alive**
   - Implement WebSocket ping
   - Handle pong responses
   - Disconnect dead clients
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] WebSocket server listens on port 8081
- [ ] Clients receive metrics updates
- [ ] Clients receive event notifications
- [ ] Auto-reconnect works

**Testing:**
```bash
# Start server
./rootstream-host --websocket-port 8081

# Connect with wscat
wscat -c ws://localhost:8081
# Should receive JSON metrics every second
```

---

## Phase 4: Additional Codecs (2-3 weeks) 🟡 MEDIUM PRIORITY

**Goal:** Support VP9, AV1, H.265/HEVC codecs

### Week 11: VP9 Encoder Wrapper
**File:** New file `src/vp9_encoder.c`

#### Tasks:
1. **Create VP9 Encoder**
   - Wrap libvpx VP9 encoder
   - Support realtime mode
   - Support quality mode
   - **Estimated Lines:** 300-400 LOC
   - **Time:** 3-4 days

2. **Add VP9 Rate Control**
   - Constant bitrate (CBR)
   - Variable bitrate (VBR)
   - Target quality (CQ)
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

3. **Integrate with Encoding Pipeline**
   - Add to encoder selection
   - Add command-line option `--codec vp9`
   - Test with MKV/WebM containers
   - **Estimated Lines:** 80-100 LOC
   - **Time:** 1 day

**Deliverables:**
- [ ] VP9 encoding works
- [ ] Quality comparable to H.264
- [ ] MKV/WebM output works

**Testing:**
```bash
./rootstream-host --codec vp9 --format mkv -o output.mkv
ffprobe output.mkv  # Should show VP9 codec
```

---

### Week 12-13: AV1 & HEVC Support
**Files:** 
- New file `src/av1_encoder.c`
- New file `src/hevc_encoder.c`

#### Tasks (AV1):
1. **Wrap libaom AV1 Encoder**
   - Realtime mode (for streaming)
   - Archive mode (for recording)
   - **Estimated Lines:** 350-450 LOC
   - **Time:** 4-5 days

2. **Optimize AV1 Settings**
   - Tune for latency
   - Tune for quality
   - Add presets (fast, medium, slow)
   - **Estimated Lines:** 100-150 LOC
   - **Time:** 1-2 days

#### Tasks (HEVC):
1. **Wrap x265 HEVC Encoder**
   - Support hardware encoding (VAAPI HEVC)
   - Support software encoding (libx265)
   - **Estimated Lines:** 300-400 LOC
   - **Time:** 3-4 days

2. **Add Patent Warning**
   - Display warning about HEVC patents
   - Require explicit opt-in flag
   - **Estimated Lines:** 30-40 LOC
   - **Time:** Half day

**Deliverables:**
- [ ] AV1 encoding works (slow but high quality)
- [ ] HEVC encoding works
- [ ] Patent warning displayed

---

## Phase 5: VR/OpenXR System (6-8 weeks) 🟢 LOW PRIORITY

**Goal:** Enable VR game streaming (complete subsystem)

### Weeks 14-21: OpenXR Implementation
**File:** `src/vr/openxr_manager.c` (273 lines → 2,000+ lines)

#### High-Level Tasks:
1. **OpenXR Initialization** (Week 14-15)
   - Initialize OpenXR runtime
   - Create instance
   - Select system
   - Enumerate views
   - **Estimated Lines:** 400-500 LOC
   - **Time:** 2 weeks

2. **Session Management** (Week 16)
   - Create session
   - Bind graphics API (Vulkan/OpenGL)
   - Create swapchains for each eye
   - Handle session state changes
   - **Estimated Lines:** 300-400 LOC
   - **Time:** 1 week

3. **Frame Rendering Loop** (Week 17-18)
   - Wait for frame
   - Locate views (get HMD pose)
   - Render each eye
   - Submit layers
   - **Estimated Lines:** 400-500 LOC
   - **Time:** 2 weeks

4. **Head Tracking** (Week 19)
   - Get head pose
   - Transform to world space
   - Send to game
   - **Estimated Lines:** 200-300 LOC
   - **Time:** 1 week

5. **Controller Input** (Week 20-21)
   - Enumerate input sources
   - Bind actions
   - Get controller poses
   - Get button/trigger states
   - Send to game
   - **Estimated Lines:** 400-600 LOC
   - **Time:** 2 weeks

**Deliverables:**
- [ ] SteamVR integration works
- [ ] Head tracking works
- [ ] Controller input works
- [ ] Stereo rendering works

**Testing:**
```bash
# Requires VR headset
./rootstream-host --vr --headset steamvr
./rootstream-client --vr
```

---

## Phase 6: Mobile Clients (12-16 weeks) 🟢 LOW PRIORITY

**Goal:** Enable streaming to Android and iOS devices

### Weeks 22-29: Android Client (8 weeks)
**Files:** `android/RootStream/app/src/main/cpp/*`

#### Tasks:
1. **Vulkan Renderer** (Weeks 22-24)
   - Port desktop Vulkan renderer
   - Adapt for Android NDK
   - Handle surface lifecycle
   - **Estimated Lines:** 1,500-2,000 LOC
   - **Time:** 3 weeks

2. **Opus Decoder** (Week 25)
   - Wrap libopus
   - Output to OpenSL ES
   - **Estimated Lines:** 300-400 LOC
   - **Time:** 1 week

3. **Touch Input** (Week 26)
   - Capture touch events
   - Send to host
   - **Estimated Lines:** 200-300 LOC
   - **Time:** 1 week

4. **UI & Settings** (Week 27-29)
   - Server discovery
   - Settings menu
   - Performance overlay
   - **Estimated Lines:** 1,000-1,500 LOC (Java/Kotlin)
   - **Time:** 3 weeks

**Deliverables:**
- [ ] Android APK installs
- [ ] Video streams to phone
- [ ] Audio plays
- [ ] Touch input works

---

### Weeks 30-37: iOS Client (8 weeks)
**Files:** `ios/RootStream/RootStream/*`

#### Tasks:
1. **Metal Renderer** (Weeks 30-32)
   - Create Metal-based renderer
   - YUV→RGB shaders
   - **Estimated Lines:** 1,500-2,000 LOC (Swift)
   - **Time:** 3 weeks

2. **Opus Decoder** (Week 33)
   - Wrap libopus
   - Output to AVAudioEngine
   - **Estimated Lines:** 300-400 LOC (Swift)
   - **Time:** 1 week

3. **Touch Input** (Week 34)
   - Capture UITouch events
   - Send to host
   - **Estimated Lines:** 200-300 LOC (Swift)
   - **Time:** 1 week

4. **UI & Settings** (Week 35-37)
   - Server discovery
   - Settings menu
   - Performance overlay
   - **Estimated Lines:** 1,000-1,500 LOC (Swift)
   - **Time:** 3 weeks

**Deliverables:**
- [ ] iOS IPA installs
- [ ] Video streams to iPhone/iPad
- [ ] Audio plays
- [ ] Touch input works

---

## Timeline Summary

| Phase | Duration | Priority | Status |
|-------|----------|----------|--------|
| Phase 1: Critical Client | 4-5 weeks | 🔴 HIGH | Not Started |
| Phase 2: Recording | 2-3 weeks | 🟡 MEDIUM | Not Started |
| Phase 3: Web Monitoring | 2 weeks | 🟡 MEDIUM | Not Started |
| Phase 4: Additional Codecs | 2-3 weeks | 🟡 MEDIUM | Not Started |
| Phase 5: VR/OpenXR | 6-8 weeks | 🟢 LOW | Not Started |
| Phase 6: Mobile Clients | 12-16 weeks | 🟢 LOW | Not Started |
| **Total** | **28-37 weeks** | | |

---

## Milestones

### Milestone 1: Working Client (Week 5) ✨
- User can stream from host to client
- Video renders on screen
- Audio plays synchronized
- Input (keyboard/mouse) works

### Milestone 2: Feature Complete Recording (Week 8) ✨
- MP4/MKV formats work
- Instant replay works
- Chapters work
- Multiple codecs available

### Milestone 3: Web Dashboard (Week 10) ✨
- Real-time metrics dashboard
- Remote control via API
- Event notifications

### Milestone 4: Advanced Codecs (Week 13) ✨
- VP9 encoding
- AV1 encoding (archival quality)
- HEVC encoding (patent warning)

### Milestone 5: VR Support (Week 21) ✨
- OpenXR integration
- Head tracking
- Controller input
- SteamVR games stream to VR

### Milestone 6: Mobile Support (Week 37) ✨
- Android client works
- iOS client works
- Touch input on mobile

---

## Resource Requirements

### Development Environment:
- Linux workstation with GPU (Intel/AMD/NVIDIA)
- X11 and Wayland sessions for testing
- VR headset (for Phase 5)
- Android device (for Phase 6)
- iOS device + Mac (for Phase 6)

### Dependencies to Install:
```bash
# Phase 1: Client
sudo apt install libvulkan-dev vulkan-tools glslang-tools
sudo apt install libwayland-dev libxcb1-dev
sudo apt install libpipewire-0.3-dev libpulse-dev

# Phase 2: Recording
sudo apt install libavformat-dev libavcodec-dev

# Phase 3: Web
sudo apt install libmicrohttpd-dev libwebsockets-dev

# Phase 4: Codecs
sudo apt install libvpx-dev libaom-dev libx265-dev

# Phase 5: VR
# Requires SteamVR SDK (manual install)

# Phase 6: Mobile
# Requires Android NDK and Xcode
```

### Estimated Development Hours:
- Phase 1: 160-200 hours
- Phase 2: 80-120 hours
- Phase 3: 80 hours
- Phase 4: 80-120 hours
- Phase 5: 240-320 hours
- Phase 6: 480-640 hours
- **Total: 1,120-1,480 hours** (28-37 weeks at 40 hours/week)

---

## Risk Mitigation

### High-Risk Areas:
1. **Vulkan Renderer** - Complex, many moving parts
   - Mitigation: Incremental testing, reference desktop renderer
   
2. **Wayland Backend** - Protocol complexity
   - Mitigation: Study existing Wayland clients, use xdg-shell

3. **VR/OpenXR** - Requires specialized hardware
   - Mitigation: Use emulator first, then real hardware

4. **iOS Development** - Requires Mac, App Store signing
   - Mitigation: Test with TestFlight first

### Dependency Risks:
- libmicrohttpd API changes → Pin to specific version
- libwebsockets API changes → Pin to specific version
- Vulkan API changes → Use stable Vulkan 1.2

---

## Success Criteria

### Phase 1 Success:
```bash
# This should work:
./rootstream-host -c drm -e vaapi --bitrate 20000
./rootstream-client --connect <host-ip> --backend x11
# Result: Video renders on client, audio plays, input works
```

### Phase 2 Success:
```bash
# This should work:
./rootstream-host --record gameplay.mp4 --format mp4
./rootstream-host --replay-buffer-seconds 30 --replay-save-key F9
# Result: MP4 file plays in VLC, replay saves with audio
```

### Phase 3 Success:
```bash
# This should work:
./rootstream-host --api-port 8080 --websocket-port 8081
curl http://localhost:8080/api/status
# Result: Returns JSON system status
```

### Phase 4 Success:
```bash
# This should work:
./rootstream-host --codec vp9 --format mkv -o output.mkv
./rootstream-host --codec av1 --format mkv -o output.mkv
# Result: Files encode correctly, play in mpv
```

### Phase 5 Success:
```bash
# This should work:
./rootstream-host --vr --headset steamvr
./rootstream-client --vr
# Result: Game renders in VR headset, head tracking works
```

### Phase 6 Success:
```bash
# This should work:
adb install RootStream.apk
# Launch app, connect to host
# Result: Video streams to Android, touch input works
```

---

## Maintenance Plan

### After Implementation:
1. **Documentation Updates**
   - Update README.md with all working features
   - Create user guides for each feature
   - Add troubleshooting sections

2. **Testing Infrastructure**
   - Add unit tests for new code
   - Add integration tests
   - Set up CI/CD pipelines

3. **Performance Optimization**
   - Profile Vulkan renderer
   - Optimize network code
   - Reduce latency

4. **Bug Fixes**
   - Address issues from testing
   - Fix memory leaks
   - Fix race conditions

---

## Conclusion

This roadmap provides a **clear, actionable plan** to complete all TODO/stub implementations in RootStream. The phased approach prioritizes critical functionality first (working client) while deferring lower-priority features (VR, mobile) to later phases.

**Key Takeaway:** Focus on Phase 1 first. Once the client works, users can actually use RootStream for its primary purpose: low-latency game streaming.

---

**Document Version:** 1.0  
**Last Updated:** February 15, 2026  
**Next Review:** After Phase 1 completion  
**Maintained By:** Development Team
