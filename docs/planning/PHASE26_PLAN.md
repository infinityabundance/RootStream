# Phase 26+: Comprehensive Implementation Plan
## Deep Analysis & Recommended Next Steps

**Generated:** February 14, 2026  
**Purpose:** Bring RootStream implementation to parity with documentation and complete critical stubs

---

## Executive Summary

After deep analysis of the RootStream codebase, documentation, and phase summaries, we've identified **30+ stub functions** and several critical gaps between documented features and actual implementation. This document outlines a phased approach to:

1. **Complete the KDE Plasma Client** (currently 95% stubs)
2. **Implement missing recording features** (MP4/MKV containers, replay buffer)
3. **Add web monitoring infrastructure** (API/WebSocket servers are stubs)
4. **Implement advanced features** (multi-monitor, adaptive bitrate)
5. **Complete VR/OpenXR support** (entire system is placeholder)
6. **Achieve documentation parity**

---

## Critical Findings

### 🔴 High Priority Stubs (Blocking Core Functionality)

#### 1. **KDE Plasma Client - Critical Path** ⚠️
**Status:** Framework exists, but core functionality is NOT implemented

**Files with Stubs:**
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.c` - 18+ TODO items
- `clients/kde-plasma-client/src/renderer/vulkan_x11.c` - Initialization, surface creation stubs
- `clients/kde-plasma-client/src/renderer/vulkan_wayland.c` - Initialization, surface creation stubs
- `clients/kde-plasma-client/src/renderer/vulkan_headless.c` - Headless rendering stubs

**Documented Claims (README.md):**
> "Native Qt 6 / QML interface"  
> "Hardware-accelerated decoding (VA-API)"  
> "PulseAudio/PipeWire audio support"

**Reality:** The client builds but cannot render video, play audio, or handle input properly.

**Impact:** Users cannot actually use the client as documented.

---

#### 2. **Recording System Gaps** ⚠️

**What's Implemented:**
- ✅ H.264 encoder wrapper
- ✅ RSTR format recording
- ✅ Opus audio encoding
- ✅ Basic recording manager

**What's NOT Implemented:**
- ❌ MP4 container format (documented but missing)
- ❌ MKV/Matroska container format (documented but missing)
- ❌ VP9 encoder wrapper
- ❌ AV1 encoder wrapper
- ❌ Replay buffer actual implementation (structure exists, logic missing)
- ❌ `--replay-save` command functionality

**Files:**
- `src/recording/replay_buffer.cpp` - Line 150: "TODO: Add video stream setup and muxing"
- `src/recording/recording_metadata.cpp` - "TODO: Implement proper chapter support"

**Documented (README.md lines 110-113):**
> "Multi-Codec Support - H.264, VP9, AV1"  
> "Container Formats - MP4, Matroska/MKV"  
> "Instant Replay - Save the last N seconds"

**Impact:** Users expect MP4 output and instant replay, neither work.

---

#### 3. **Web Infrastructure - Complete Stubs** ⚠️

**Files:**
- `src/web/api_server.c`
  - Line 52: `api_server_register_route()` - Returns 0, does nothing
  - Line 66: `api_server_start()` - No libmicrohttpd integration
- `src/web/websocket_server.c`
  - Line 53: `websocket_server_start()` - No libwebsockets context
  - Line 89: `websocket_server_broadcast_metrics()` - Only printf logging
  - Line 111: `websocket_server_broadcast_event()` - Only printf logging
- `src/web/api_routes.c`
  - Line 233: `api_route_post_auth_login()` - Returns hardcoded token

**Impact:** No remote monitoring, no web dashboard, no API access.

---

#### 4. **Security Stubs - Critical** 🔴

**File:** `src/database/models/user_model.cpp`
- Line 211: `validatePassword()` - **WARNING: Always returns false**
- Comment: "WARNING: validatePassword not implemented - integrate bcrypt or argon2"

**Impact:** Authentication system is non-functional and insecure.

---

### 🟡 Medium Priority (Feature Completeness)

#### 5. **VR/OpenXR System - Placeholder Only**

**File:** `src/vr/openxr_manager.c`
- Entire system is stub (lines 38-273)
- All functions print "stub" messages and return success

**Documented (README.md):** Listed as future work (v2.0)

**Impact:** VR streaming is advertised but completely non-functional.

---

#### 6. **Multi-Monitor Support**

**Documented (ROADMAP.md v1.3):**
- Enumerate outputs/CRTCs
- Choose which monitor to stream
- Handle hotplug/unplug

**Reality:** DRM capture reads full framebuffer only, no monitor selection logic.

**Impact:** Multi-monitor users cannot select specific displays.

---

#### 7. **Client-Side Latency Instrumentation**

**Documented (ROADMAP.md v1.1):**
- Client-side timestamps (recv → decode → present)
- Debug overlay showing FPS + latency

**Reality:** Host-side instrumentation exists, client-side missing.

**Impact:** Cannot measure end-to-end latency from client perspective.

---

### 🟢 Low Priority (Polish & Enhancement)

#### 8. **Adaptive Bitrate Control**
**Status:** Not implemented, documented as future work
**Files:** No implementation exists

#### 9. **H.265/HEVC Support**
**Status:** Documented as v1.2 feature, not started

#### 10. **Windows Client**
**Status:** Partial code exists, incomplete

---

## Phased Implementation Roadmap

### **Phase 26: Complete Core KDE Client** (Priority: CRITICAL)
**Goal:** Make the client actually functional for end-to-end streaming

**Estimated Effort:** 2-3 weeks

#### Tasks:

**26.1: Vulkan Renderer Core**
- [ ] Implement `vulkan_renderer_init()` - Initialize Vulkan instance, device, queues
- [ ] Implement `vulkan_renderer_create_backend_specific_surface()` - X11/Wayland surfaces
- [ ] Implement swapchain creation with proper present modes
- [ ] Implement command buffer allocation and recording
- [ ] Implement `vulkan_renderer_upload_frame()` - Upload YUV/RGB frames to GPU
- [ ] Implement `vulkan_renderer_render()` - Execute render pass
- [ ] Implement `vulkan_renderer_present()` - Present to screen
- [ ] Add proper error handling and cleanup

**26.2: Vulkan Platform Backends**
- [ ] Implement `vulkan_x11.c` - X11 window integration
  - X11 display connection
  - X11 surface creation (VK_KHR_xlib_surface)
  - Window event handling
- [ ] Implement `vulkan_wayland.c` - Wayland integration
  - Wayland display connection
  - Wayland surface creation (VK_KHR_wayland_surface)
  - Surface event handling
- [ ] Implement `vulkan_headless.c` - Headless mode
  - Offscreen rendering
  - Frame readback for testing

**26.3: Audio Playback**
- [ ] Implement PipeWire audio backend
- [ ] Implement PulseAudio fallback
- [ ] Add audio/video synchronization logic
- [ ] Add audio device selection
- [ ] Add volume control

**26.4: Input Handling**
- [ ] Implement keyboard capture
- [ ] Implement mouse capture (absolute + relative)
- [ ] Implement gamepad support
- [ ] Send input events to host via network protocol
- [ ] Add input latency compensation

**26.5: Testing & Validation**
- [ ] Create test suite for Vulkan renderer
- [ ] Test X11 backend on various distros
- [ ] Test Wayland backend (KDE Plasma, GNOME)
- [ ] Test audio playback and sync
- [ ] Test input latency and accuracy
- [ ] Update client documentation

**Success Criteria:**
- ✅ Client can connect to host and display video
- ✅ Audio plays in sync with video
- ✅ Input (keyboard/mouse) works end-to-end
- ✅ Works on X11 and Wayland
- ✅ Latency < 30ms on LAN

---

### **Phase 27: Complete Recording System** (Priority: HIGH)
**Goal:** Implement documented recording features (MP4/MKV, replay buffer)

**Estimated Effort:** 2 weeks

#### Tasks:

**27.1: MP4 Container Support**
- [ ] Integrate FFmpeg libavformat for MP4 muxing
- [ ] Implement MP4 writer with proper metadata
- [ ] Add AAC audio encoding option
- [ ] Support H.264 + AAC in MP4 container
- [ ] Add progress reporting during recording
- [ ] Test with VLC, ffprobe, MediaInfo

**27.2: Matroska/MKV Container Support**
- [ ] Implement MKV muxer via libavformat
- [ ] Support Opus passthrough (no re-encoding)
- [ ] Add chapter marker support
- [ ] Add metadata tagging (game name, date, etc.)
- [ ] Test with VLC, mpv, MediaInfo

**27.3: Replay Buffer Implementation**
- [ ] Implement circular buffer for frames
- [ ] Add memory management (limit buffer size)
- [ ] Implement `--replay-buffer-seconds N` flag
- [ ] Implement `--replay-save FILE` command
- [ ] Add hotkey support for instant save
- [ ] Optimize for minimal CPU/memory overhead

**27.4: VP9 Encoder Wrapper**
- [ ] Create `vp9_encoder_wrapper.h/cpp`
- [ ] Integrate libvpx (FFmpeg)
- [ ] Add quality presets (cpu-used: 0-8)
- [ ] Update recording presets configuration
- [ ] Benchmark vs H.264 (quality, speed, file size)

**27.5: AV1 Encoder Wrapper (Optional)**
- [ ] Create `av1_encoder_wrapper.h/cpp`
- [ ] Integrate libaom (FFmpeg)
- [ ] Add quality presets
- [ ] Document performance characteristics (very slow)
- [ ] Mark as "archival quality" preset

**Success Criteria:**
- ✅ Recording produces standard MP4 files
- ✅ Instant replay saves last 30 seconds
- ✅ VP9 preset works for smaller file sizes
- ✅ All recordings playable in standard players

---

### **Phase 28: Web Monitoring Infrastructure** (Priority: MEDIUM)
**Goal:** Complete web API and WebSocket servers for remote monitoring

**Estimated Effort:** 1-2 weeks

#### Tasks:

**28.1: API Server Implementation**
- [ ] Integrate libmicrohttpd properly
- [ ] Implement route registration system
- [ ] Implement authentication middleware
- [ ] Add rate limiting
- [ ] Add CORS support for web clients
- [ ] Add TLS/HTTPS support

**28.2: WebSocket Server**
- [ ] Integrate libwebsockets library
- [ ] Implement WebSocket handshake
- [ ] Create pub/sub system for events
- [ ] Implement broadcast for metrics
- [ ] Add connection management
- [ ] Add authentication for WebSocket connections

**28.3: API Endpoints**
- [ ] `/api/status` - Server status and metrics
- [ ] `/api/peers` - Connected peers list
- [ ] `/api/recordings` - List recordings
- [ ] `/api/start-recording` - Start recording
- [ ] `/api/stop-recording` - Stop recording
- [ ] `/api/settings` - Get/update settings
- [ ] Document all endpoints with OpenAPI spec

**28.4: Real-time Metrics Broadcasting**
- [ ] Stream FPS, bitrate, latency to WebSocket clients
- [ ] Send peer connection/disconnection events
- [ ] Send recording start/stop events
- [ ] Add configurable update interval

**28.5: Web Dashboard (Optional)**
- [ ] Create simple HTML/JS dashboard
- [ ] Display real-time metrics
- [ ] Show connected peers
- [ ] Control recording from web UI
- [ ] View logs in browser

**Success Criteria:**
- ✅ Web API responds to REST requests
- ✅ WebSocket streams live metrics
- ✅ Can start/stop recording via API
- ✅ Dashboard shows real-time status

---

### **Phase 29: Advanced Features** (Priority: MEDIUM)
**Goal:** Multi-monitor, adaptive bitrate, client latency instrumentation

**Estimated Effort:** 2-3 weeks

#### Tasks:

**29.1: Multi-Monitor Support**
- [ ] Enumerate DRM outputs/CRTCs
- [ ] Add `--display N` flag to select monitor
- [ ] Add `--list-displays` command
- [ ] Capture specific monitor framebuffer
- [ ] Handle monitor hotplug/unplug gracefully
- [ ] Add monitor selection to GUI/tray app

**29.2: Adaptive Bitrate Control**
- [ ] Implement packet loss detection
- [ ] Implement RTT (round-trip time) measurement
- [ ] Create bitrate adjustment algorithm
- [ ] Add smooth bitrate transitions
- [ ] Add quality vs. framerate trade-off logic
- [ ] Log bitrate changes for debugging

**29.3: Client-Side Latency Instrumentation**
- [ ] Add timestamps to received packets
- [ ] Measure decode latency (VA-API timing)
- [ ] Measure present latency
- [ ] Calculate end-to-end latency
- [ ] Add debug overlay showing:
  - FPS (actual vs. target)
  - Latency (recv, decode, present, total)
  - Network stats (packet loss, jitter)
  - Bitrate graph
- [ ] Make overlay togglable (F12 or similar)

**29.4: H.265/HEVC Support**
- [ ] Add HEVC encoder support (VA-API)
- [ ] Add HEVC decoder support (client)
- [ ] Update protocol to negotiate codec
- [ ] Benchmark HEVC vs H.264 (quality, bandwidth)
- [ ] Document GPU compatibility

**Success Criteria:**
- ✅ Can select specific monitor to stream
- ✅ Bitrate adapts to network conditions
- ✅ Client overlay shows latency metrics
- ✅ HEVC optional codec available

---

### **Phase 30: Security & Authentication** (Priority: HIGH)
**Goal:** Fix security stubs and implement proper authentication

**Estimated Effort:** 1 week

#### Tasks:

**30.1: Password Validation**
- [ ] Integrate bcrypt library
- [ ] Implement proper password hashing
- [ ] Implement password verification
- [ ] Add password strength requirements
- [ ] Add rate limiting for failed attempts

**30.2: Session Management**
- [ ] Implement secure session tokens
- [ ] Add session expiration
- [ ] Add session revocation
- [ ] Store sessions securely

**30.3: TOTP/2FA Support**
- [ ] Implement TOTP generation/verification
- [ ] Add QR code for authenticator apps
- [ ] Add backup codes

**30.4: Security Audit**
- [ ] Review all authentication code
- [ ] Review all cryptographic operations
- [ ] Check for injection vulnerabilities
- [ ] Add input validation everywhere
- [ ] Document security model

**Success Criteria:**
- ✅ Password validation works correctly
- ✅ Sessions are secure and expire
- ✅ 2FA optional but available
- ✅ No obvious security vulnerabilities

---

### **Phase 31: VR/OpenXR Implementation** (Priority: LOW)
**Goal:** Complete VR streaming support (currently all stubs)

**Estimated Effort:** 3-4 weeks

#### Tasks:

**31.1: OpenXR Manager**
- [ ] Initialize OpenXR runtime
- [ ] Create OpenXR session
- [ ] Implement frame loop (xrWaitFrame, xrBeginFrame, xrEndFrame)
- [ ] Handle view configurations
- [ ] Support stereo rendering

**31.2: Stereoscopic Renderer**
- [ ] Render left/right eye views
- [ ] Implement proper projection matrices
- [ ] Handle IPD (interpupillary distance)
- [ ] Add lens distortion correction

**31.3: VR Input**
- [ ] Integrate OpenXR input actions
- [ ] Support VR controllers
- [ ] Support hand tracking (if available)
- [ ] Map VR input to game input

**31.4: VR Capture & Encoding**
- [ ] Capture both eye views
- [ ] Encode stereo frames
- [ ] Update protocol for VR data
- [ ] Optimize bandwidth for 2x video streams

**Success Criteria:**
- ✅ VR client connects to host
- ✅ Stereo video streams correctly
- ✅ VR controllers work
- ✅ Latency acceptable for VR (<20ms)

---

### **Phase 32: Testing & Stability** (Priority: HIGH)
**Goal:** Comprehensive testing and bug fixes

**Estimated Effort:** 2 weeks

#### Tasks:

**32.1: Unit Tests**
- [ ] Add tests for all crypto functions
- [ ] Add tests for network protocol
- [ ] Add tests for encoding/decoding
- [ ] Add tests for recording system
- [ ] Achieve >80% code coverage

**32.2: Integration Tests**
- [ ] Test full capture → encode → send → decode → present pipeline
- [ ] Test with multiple clients
- [ ] Test network failures and recovery
- [ ] Test audio/video sync
- [ ] Test recording while streaming

**32.3: Performance Tests**
- [ ] Benchmark latency across hardware
- [ ] Benchmark CPU usage
- [ ] Benchmark memory usage
- [ ] Benchmark bandwidth usage
- [ ] Document performance baselines

**32.4: Compatibility Testing**
- [ ] Test on Intel/AMD/NVIDIA GPUs
- [ ] Test on Arch, Ubuntu, Fedora
- [ ] Test on X11 and Wayland
- [ ] Test with various kernel versions
- [ ] Document known issues

**32.5: Stress Testing**
- [ ] Long-duration streaming (24+ hours)
- [ ] High packet loss scenarios
- [ ] Multiple concurrent clients
- [ ] Memory leak detection
- [ ] CPU spike scenarios

**Success Criteria:**
- ✅ All tests pass on CI/CD
- ✅ No memory leaks
- ✅ Stable for 24+ hour sessions
- ✅ Works on all supported distros

---

### **Phase 33: Documentation Parity** (Priority: MEDIUM)
**Goal:** Update all documentation to match actual implementation

**Estimated Effort:** 1 week

#### Tasks:

**33.1: Update README.md**
- [ ] Remove claims about unimplemented features
- [ ] Add "Implementation Status" section
- [ ] Update feature checklist with actual status
- [ ] Add clear "Roadmap" vs "Implemented" distinction
- [ ] Update performance numbers with disclaimers

**33.2: Update ARCHITECTURE.md**
- [ ] Document actual client architecture
- [ ] Add sequence diagrams for client flow
- [ ] Update latency breakdown with real measurements
- [ ] Document all stub areas clearly

**33.3: Create Implementation Guide**
- [ ] Document how to implement new features
- [ ] Provide code examples
- [ ] Explain design decisions
- [ ] Add troubleshooting guide

**33.4: API Documentation**
- [ ] Generate API docs from code (Doxygen)
- [ ] Document all public functions
- [ ] Add usage examples
- [ ] Document all protocol messages

**Success Criteria:**
- ✅ Documentation matches implementation
- ✅ No false claims
- ✅ Clear roadmap for future work
- ✅ Contributors can understand codebase

---

## Implementation Priority Matrix

| Phase | Priority | Blocking Issues | User Impact | Effort |
|-------|----------|----------------|-------------|--------|
| 26: KDE Client | **CRITICAL** | Client unusable | **HIGH** | 2-3 weeks |
| 30: Security | **HIGH** | Security risk | **HIGH** | 1 week |
| 27: Recording | **HIGH** | Feature mismatch | **MEDIUM** | 2 weeks |
| 28: Web API | **MEDIUM** | No monitoring | **LOW** | 1-2 weeks |
| 29: Advanced | **MEDIUM** | Nice-to-have | **MEDIUM** | 2-3 weeks |
| 32: Testing | **HIGH** | Stability risk | **HIGH** | 2 weeks |
| 33: Docs | **MEDIUM** | Confusing users | **MEDIUM** | 1 week |
| 31: VR | **LOW** | Niche feature | **LOW** | 3-4 weeks |

---

## Recommended Immediate Actions

### Sprint 1 (Week 1-2): Critical Client Work
1. **Start Phase 26.1** - Vulkan renderer core
2. **Start Phase 26.2** - X11 backend (most common)
3. **Start Phase 30.1** - Fix password validation security issue

### Sprint 2 (Week 3-4): Client Completion
4. **Continue Phase 26** - Audio and input
5. **Start Phase 27.1** - MP4 recording
6. **Start Phase 27.3** - Replay buffer

### Sprint 3 (Week 5-6): Recording & Testing
7. **Complete Phase 27** - All recording features
8. **Start Phase 32** - Testing infrastructure
9. **Update Phase 33** - Documentation updates

### Sprint 4 (Week 7-8): Advanced Features
10. **Start Phase 29** - Multi-monitor support
11. **Continue Phase 32** - Integration tests
12. **Start Phase 28** - Web API (if time permits)

---

## Resource Requirements

### Development Team
- **2 C/C++ developers** (client, recording)
- **1 Graphics/Vulkan expert** (renderer)
- **1 QA/Test engineer** (testing)
- **1 Technical writer** (documentation)

### Infrastructure
- Test machines with:
  - Intel GPU (iGPU + discrete)
  - AMD GPU (RDNA/GCN)
  - NVIDIA GPU (consumer + pro)
  - X11 and Wayland setups
  - Various Linux distros

### Dependencies
- Vulkan SDK
- FFmpeg libraries (libavformat, libavcodec)
- libmicrohttpd (web API)
- libwebsockets (real-time updates)
- bcrypt/argon2 (password hashing)
- OpenXR SDK (VR support)

---

## Risk Assessment

### High Risks
1. **Vulkan complexity** - Requires specialized expertise
   - Mitigation: Use Vulkan tutorials, reference examples
2. **GPU compatibility** - Different drivers, capabilities
   - Mitigation: Extensive testing, graceful fallbacks
3. **Audio/video sync** - Timing issues are hard to debug
   - Mitigation: Use proven sync algorithms, add diagnostics

### Medium Risks
1. **Performance degradation** - New features might add latency
   - Mitigation: Benchmark continuously, optimize hot paths
2. **Security vulnerabilities** - Crypto and auth are critical
   - Mitigation: Code review, security audit, use proven libraries

### Low Risks
1. **VR support complexity** - Limited user base, can delay
2. **Web dashboard features** - Nice-to-have, not critical

---

## Success Metrics

### Phase 26 (Client) Success:
- [ ] 95% of test users can stream successfully
- [ ] Latency < 30ms on gigabit LAN
- [ ] Audio/video sync within 50ms
- [ ] Works on 3+ Linux distros

### Phase 27 (Recording) Success:
- [ ] Recordings play in VLC/mpv without issues
- [ ] Replay buffer saves within 2 seconds
- [ ] File sizes within 10% of target

### Overall Project Success:
- [ ] All documented features work
- [ ] No critical security issues
- [ ] Stable for 24+ hour streams
- [ ] Community adoption growing

---

## Conclusion

RootStream has a **solid foundation** with working capture, encoding, network protocol, and crypto. However, the **client is currently non-functional** and several **documented features don't work**.

**The highest priority is Phase 26** - completing the KDE client so users can actually use RootStream end-to-end. Following that, **Phases 27, 30, and 32** address recording, security, and stability.

This plan provides a **clear path** from the current state (host works, client stubs) to a **fully functional, documented, and tested** streaming solution.

**Estimated total effort:** 12-16 weeks with a small team  
**Estimated timeline:** 3-4 months for Phases 26-29, 32-33

---

**Last Updated:** February 14, 2026  
**Next Review:** After Phase 26 completion
