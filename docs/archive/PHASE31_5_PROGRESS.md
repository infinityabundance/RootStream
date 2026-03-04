# Phase 31.5: Present Mode Switching - Micro-Task Progress

## Overview
Implement VSync toggle and present mode switching by enabling swapchain recreation with different present modes (FIFO, IMMEDIATE, MAILBOX).

**Goal:** Allow users to toggle VSync and switch between different present modes for optimal performance or visual quality.

---

## Progress Tracker

| Task | Status | Duration | LOC | Description |
|------|--------|----------|-----|-------------|
| 31.5.0 | ✅ | 30m | 0 | Planning |
| 31.5.1 | ⏳ | 1h | 40 | Mode validation |
| 31.5.2 | ⏳ | 1h | 30 | Resource cleanup |
| 31.5.3 | ⏳ | 1.5h | 50 | Swapchain recreation |
| 31.5.4 | ⏳ | 1h | 35 | Main function |
| 31.5.5 | ⏳ | 30m | 20 | Helper functions |
| 31.5.6 | ⏳ | 30m | 15 | Error handling |
| **TOTAL** | **11%** | **0.5h/6h** | **0/190** | |

**Current:** Planning complete (Task 31.5.0)  
**Next:** Task 31.5.1 - Present mode validation

---

## Micro-Task Details

### ✅ Task 31.5.0: Planning (COMPLETE)
**Duration:** 30 minutes  
**Completed:** February 15, 2026

- Created 6 micro-task breakdown
- Analyzed vulkan_renderer.c structure
- Identified integration points
- Ready to begin implementation

### ⏳ Task 31.5.1: Present Mode Validation (NEXT)
**Goal:** Check which present modes are supported  
**Estimated:** 1 hour, 40 LOC

**Function to create:**
```c
static int is_present_mode_supported(vulkan_context_t *ctx, VkPresentModeKHR mode);
```

**What it does:**
- Query available present modes from device
- Check if requested mode is supported
- Return 1 if supported, 0 if not

### ⏳ Task 31.5.2: Resource Cleanup
**Goal:** Helper to clean up swapchain-dependent resources  
**Estimated:** 1 hour, 30 LOC

**Function to create:**
```c
static void cleanup_swapchain_resources(vulkan_context_t *ctx);
```

**What it does:**
- Wait for device idle
- Destroy framebuffers
- Destroy image views
- Destroy swapchain

### ⏳ Task 31.5.3: Swapchain Recreation
**Goal:** Recreate swapchain with new present mode  
**Estimated:** 1.5 hours, 50 LOC

**Function to create:**
```c
static int recreate_swapchain(vulkan_context_t *ctx);
```

**What it does:**
- Create new swapchain
- Recreate image views
- Recreate framebuffers

### ⏳ Task 31.5.4: Main Function Implementation
**Goal:** Complete vulkan_set_present_mode()  
**Estimated:** 1 hour, 35 LOC

**Location:** Line 2100-2110 (currently stub)

**What it does:**
- Validate present mode
- Fall back to FIFO if unsupported
- Skip if already using mode
- Clean up old resources
- Recreate swapchain

### ⏳ Task 31.5.5: Helper Functions
**Goal:** Add convenience functions  
**Estimated:** 30 minutes, 20 LOC

**Functions to add:**
- `vulkan_enable_vsync()` - FIFO mode
- `vulkan_disable_vsync()` - IMMEDIATE mode
- `vulkan_enable_triple_buffer()` - MAILBOX mode

### ⏳ Task 31.5.6: Error Handling
**Goal:** Verify proper error handling  
**Estimated:** 30 minutes, 15 LOC

**Tasks:**
- Verify all error paths
- Add NULL checks
- Test multiple switches
- Verify no leaks

---

## Success Criteria

- [ ] VSync toggle works
- [ ] Present mode switches without crash
- [ ] Fallback to FIFO works
- [ ] No memory leaks (valgrind clean)
- [ ] No validation errors

---

## Present Modes Explained

**VK_PRESENT_MODE_FIFO_KHR:**
- VSync enabled
- Always supported (required by spec)
- No tearing
- May have input latency

**VK_PRESENT_MODE_IMMEDIATE_KHR:**
- VSync disabled
- May show tearing
- Lowest latency
- Not always supported

**VK_PRESENT_MODE_MAILBOX_KHR:**
- Triple buffering
- No tearing
- Low latency
- Not always supported

---

**Status:** Planning complete  
**Ready:** Begin Task 31.5.1  
**Approach:** Continuing proven micro-task methodology
