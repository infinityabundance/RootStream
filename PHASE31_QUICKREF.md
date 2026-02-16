# Phase 31 Quick Reference

**Goal:** Implement Vulkan renderer core functionality  
**Status:** ⏳ Planning Complete, Ready to Start  
**Duration:** 10-12 days (2-3 weeks)

---

## Subphases at a Glance

| # | Subphase | Duration | LOC | Status |
|---|----------|----------|-----|--------|
| 31.1 | Frame Upload Infrastructure | 2-3 days | 200-250 | ⏳ Not Started |
| 31.2 | YUV to RGB Shader System | 2 days | 150-200 | ⏳ Not Started |
| 31.3 | Graphics Pipeline | 2 days | 150-200 | ⏳ Not Started |
| 31.4 | Rendering Loop | 1-2 days | 100-150 | ⏳ Not Started |
| 31.5 | Present Mode Switching | 1 day | 80-100 | ⏳ Not Started |
| 31.6 | Window Resize | 1 day | 100-120 | ⏳ Not Started |
| 31.7 | Cleanup & Error Handling | 1 day | 80-100 | ⏳ Not Started |
| **TOTAL** | | **10-12 days** | **860-1,120** | |

---

## Current TODOs to Fix

```c
// Line 913: vulkan_upload_frame()
// TODO: Implement frame upload

// Line 982: vulkan_render()
// TODO: Bind pipeline and draw when shaders are loaded

// Line 1071: vulkan_set_present_mode()
// TODO: Recreate swapchain with new present mode

// Line 1082: vulkan_resize()
// TODO: Recreate swapchain
```

---

## Files to Modify

### Main Implementation:
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

### New Files to Create:
- `clients/kde-plasma-client/src/renderer/shaders/yuv_to_rgb.vert`
- `clients/kde-plasma-client/src/renderer/shaders/yuv_to_rgb.frag`
- `clients/kde-plasma-client/src/renderer/shaders/compile.sh`
- `clients/kde-plasma-client/src/renderer/shaders/README.md`

### Test Files:
- `clients/kde-plasma-client/tests/unit/test_frame_upload.cpp` (new)
- `clients/kde-plasma-client/tests/unit/test_shaders.cpp` (new)
- `clients/kde-plasma-client/tests/unit/test_pipeline.cpp` (new)

---

## Success Criteria

✅ **After Phase 31:**
```bash
./rootstream-host -c drm -e vaapi --bitrate 20000
./rootstream-client --connect <host-ip>
# Result: Video frames render on screen at ≥60 FPS
```

---

## Key Metrics Targets

| Metric | Target |
|--------|--------|
| Frame Rate | ≥60 FPS |
| Upload Latency | <2ms |
| Render Latency | <1ms |
| GPU Usage (idle) | <30% |
| Memory Leaks | 0 |
| Validation Errors | 0 |

---

## Dependencies to Install

```bash
# Vulkan SDK
sudo apt install libvulkan-dev vulkan-tools

# Shader compiler
sudo apt install glslang-tools spirv-tools

# Testing tools
sudo apt install valgrind
```

---

## Development Workflow

1. **For each subphase:**
   - Read subphase details in PHASE31_PLAN.md
   - Write tests first (TDD)
   - Implement functionality
   - Run tests
   - Manual verification
   - Commit with `git commit -m "Phase 31.X: <description>"`
   - Update this file with ✅

2. **Testing:**
   - Unit tests after each function
   - Integration tests after subphase
   - Manual testing continuously
   - valgrind before commit

3. **Documentation:**
   - Comment all Vulkan calls
   - Update README if needed
   - Note any issues in commit message

---

## Next Steps

1. ✅ Read PHASE31_PLAN.md thoroughly
2. ⏳ Begin Phase 31.1: Frame Upload Infrastructure
3. ⏳ Write frame upload tests
4. ⏳ Implement `vulkan_upload_frame()`
5. ⏳ Manual test with mock frames

---

## Quick Commands

```bash
# Build client
cd clients/kde-plasma-client
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run with validation layers
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./rootstream-client

# Memory leak check
valgrind --leak-check=full ./rootstream-client

# Compile shaders
cd src/renderer/shaders
./compile.sh
```

---

## Support Documents

- **[PHASE31_PLAN.md](PHASE31_PLAN.md)** - Detailed plan with pseudo-code
- **[IMPLEMENTATION_ROADMAP.md](IMPLEMENTATION_ROADMAP.md)** - Overall roadmap
- **[VERIFICATION_REPORT.md](VERIFICATION_REPORT.md)** - Current state analysis

---

**Last Updated:** February 15, 2026  
**Ready to Begin:** Phase 31.1
