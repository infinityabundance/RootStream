# Phase 31.6: Window Resize Support - Progress Tracker

## Overview
Phase 31.6 implements window resize and minimization handling by leveraging the swapchain recreation infrastructure built in Phase 31.5.

**Start Date:** February 15, 2026  
**Goal:** Enable dynamic window resizing with minimization support  
**Estimated:** 3.5 hours, 65 LOC

---

## Micro-Task Progress

| # | Task | Est. | LOC | Status | Notes |
|---|------|------|-----|--------|-------|
| 31.6.0 | Planning | 30m | 0 | ✅ | Complete |
| 31.6.1 | Minimization | 30m | 15 | ⏳ | Next |
| 31.6.2 | Size detection | 30m | 10 | ⏳ | |
| 31.6.3 | Swapchain resize | 1h | 25 | ⏳ | |
| 31.6.4 | Render update | 30m | 10 | ⏳ | |
| 31.6.5 | Error handling | 30m | 5 | ⏳ | |

**Total:** 6 tasks, 3.5 hours, 65 LOC

---

## Task Details

### Task 31.6.1: Minimization Handling
**Goal:** Handle window minimization (0x0) safely

**Implementation:**
- Add `bool minimized` to vulkan_context_s
- Check for width==0 || height==0
- Set minimized flag and return early
- Skip swapchain recreation when minimized

**Files:** vulkan_renderer.c

---

### Task 31.6.2: Size Change Detection
**Goal:** Skip recreation if size unchanged

**Implementation:**
- Compare new size with current
- Return early if same
- Optimization to avoid unnecessary work

**Files:** vulkan_renderer.c

---

### Task 31.6.3: Swapchain Resize
**Goal:** Recreate swapchain with new dimensions

**Implementation:**
- Wait for device idle
- Call cleanup_swapchain_resources() (from 31.5)
- Update ctx->width and ctx->height
- Call recreate_swapchain() (from 31.5)
- Error handling

**Files:** vulkan_renderer.c

---

### Task 31.6.4: Render Loop Update
**Goal:** Skip rendering when minimized

**Implementation:**
- Check minimized flag in vulkan_render()
- Return early if minimized
- Avoid validation errors from 0x0 swapchain

**Files:** vulkan_renderer.c

---

### Task 31.6.5: Error Handling & Verification
**Goal:** Verify all error paths and edge cases

**Tasks:**
- Verify NULL checks
- Verify error messages
- Test resize scenarios
- Test minimization
- Verify no memory leaks

---

## Dependencies

**Requires (Complete):**
- ✅ cleanup_swapchain_resources() (Phase 31.5)
- ✅ recreate_swapchain() (Phase 31.5)
- ✅ Swapchain infrastructure

**Provides:**
- Window resize API
- Minimization handling
- Dynamic window support

---

## Success Criteria

- [ ] Window resizes smoothly
- [ ] No crashes during resize
- [ ] Minimization works
- [ ] Restoration from minimize works
- [ ] Size change optimization works
- [ ] No memory leaks

---

## Progress Log

**2026-02-15:** Planning complete (Task 31.6.0)

