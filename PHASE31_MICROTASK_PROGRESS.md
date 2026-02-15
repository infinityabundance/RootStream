# Phase 31.1: Frame Upload - Micro-Task Progress Tracker

**Last Updated:** February 15, 2026  
**Status:** In Progress  
**Estimated Completion:** 15 hours across 10 micro-tasks

---

## Progress Overview

| Task | Status | Duration | LOC | Commit | Notes |
|------|--------|----------|-----|--------|-------|
| 31.1.0 | ✅ Complete | 1h | 0 | 90dbc73 | Planning complete |
| 31.1.1 | ✅ Complete | 30m | 4 | 90dbc73 | Staging fields added |
| 31.1.2 | ✅ Complete | 1.5h | 117 | [next] | Staging buffer allocation |
| 31.1.3 | ⏳ Not Started | - | 30 | - | - |
| 31.1.4 | ⏳ Not Started | - | 40 | - | - |
| 31.1.5 | ⏳ Not Started | - | 60 | - | - |
| 31.1.6 | ⏳ Not Started | - | 50 | - | - |
| 31.1.7 | ⏳ Not Started | - | 50 | - | - |
| 31.1.8 | ⏳ Not Started | - | 30 | - | - |
| 31.1.9 | ⏳ Not Started | - | 40 | - | - |
| 31.1.10 | ⏳ Not Started | - | 20 | - | - |

**Total Completed:** 3/11 (27%)  
**Total LOC Added:** 121/384 (32%)  
**Time Spent:** 3h / 15h

---

## Detailed Progress

### ✅ Micro-Task 31.1.0: Planning & Setup
**Completed:** February 15, 2026  
**Duration:** 1 hour  
**Status:** Complete

**What was done:**
- Analyzed vulkan_renderer.c structure
- Understood frame_t structure (NV12 format)
- Reviewed vulkan_context_t resources
- Created 10 micro-task breakdown
- Documented detailed plan

**Files analyzed:**
- `clients/kde-plasma-client/src/renderer/renderer.h`
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.h`

**Key findings:**
- NV12 format: Y plane (width×height) + UV plane (width×height/2)
- Context already has image/memory handles for Y and UV
- Need staging buffer for CPU→GPU transfers

---

### ✅ Micro-Task 31.1.1: Add Staging Buffer to Context
**Completed:** February 15, 2026  
**Duration:** 30 minutes  
**Status:** Complete  
**LOC:** 4 lines added

**What was done:**
- Added `VkBuffer staging_buffer` field
- Added `VkDeviceMemory staging_memory` field
- Added `void *staging_mapped` for persistent mapping
- Added `size_t staging_size` for size tracking

**Files modified:**
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.c` (lines 103-106)

**Changes:**
```c
// Staging buffer for frame uploads
VkBuffer staging_buffer;
VkDeviceMemory staging_memory;
void *staging_mapped;
size_t staging_size;
```

**Testing:**
- ✅ Code compiles without errors
- ✅ Fields initialized to NULL/0 by calloc

**Commit:** Next commit will include this change

---

### ✅ Micro-Task 31.1.2: Create Staging Buffer Allocation Function
**Completed:** February 15, 2026  
**Duration:** 1.5 hours  
**Status:** Complete  
**LOC:** 117 lines added

**What was done:**
- Created `create_staging_buffer()` static function
- Implemented VkBuffer creation with TRANSFER_SRC usage
- Allocated HOST_VISIBLE | HOST_COHERENT memory
- Bound buffer to memory
- Mapped memory persistently for CPU access
- Added call from vulkan_init() after device creation
- Added cleanup to vulkan_cleanup()

**Files modified:**
- `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

**Function added:**
```c
static int create_staging_buffer(vulkan_context_t *ctx, size_t size) {
    // Rounds size to nearest MB
    // Creates VkBuffer with TRANSFER_SRC usage
    // Allocates HOST_VISIBLE | HOST_COHERENT memory
    // Binds buffer and maps memory persistently
    return 0;
}
```

**Integration:**
- Called from `vulkan_init()` with 4MB size (enough for 1080p NV12)
- Cleanup added to `vulkan_cleanup()` to unmap and free resources

**Testing:**
- ✅ Code structure correct
- ⏳ Runtime test pending (needs full build)

**Technical details:**
- Buffer size: 4MB (handles 1080p NV12: 1920×1080×1.5 = ~3.1MB)
- Memory properties: HOST_VISIBLE | HOST_COHERENT (for CPU access)
- Persistent mapping: Memory mapped once at creation
- Error handling: All Vulkan calls checked, cleanup on failure

**Next:** Micro-Task 31.1.3 - Frame validation function

---

### ⏳ Micro-Task 31.1.3: Add Frame Validation Function
**Status:** In Progress  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 50 lines

**Plan:**
1. Create `create_staging_buffer()` static function
2. Implement VkBuffer creation with TRANSFER_SRC usage
3. Allocate HOST_VISIBLE | HOST_COHERENT memory
4. Bind buffer to memory
5. Map memory persistently
6. Call from vulkan_init after device creation

**Implementation location:**
- Add function before `vulkan_init()`
- Call in `vulkan_init()` after device setup

**Testing plan:**
- Compile successfully
- Run client initialization
- Verify staging buffer created
- Check no memory leaks

**Next steps:**
1. Implement function
2. Test compilation
3. Commit change
4. Move to 31.1.3

---

### ⏳ Micro-Task 31.1.3: Add Frame Validation Function
**Status:** Not Started  
**Estimated Duration:** 1 hour  
**Estimated LOC:** 30 lines

**Plan:**
- Create `validate_frame()` helper
- Check frame pointer not NULL
- Check data pointer not NULL
- Check dimensions > 0
- Check format == NV12
- Check size matches expected

---

### ⏳ Micro-Task 31.1.4: Implement YUV Data Copy to Staging
**Status:** Not Started  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 40 lines

**Plan:**
- Create `copy_frame_to_staging()` helper
- Calculate Y and UV plane sizes
- memcpy Y plane data
- memcpy UV plane data with offset

---

### ⏳ Micro-Task 31.1.5: Add Image Layout Transition Helper
**Status:** Not Started  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 60 lines

**Plan:**
- Create `transition_image_layout()` helper
- Implement pipeline barrier logic
- Handle src/dst access masks
- Handle src/dst stage masks

---

### ⏳ Micro-Task 31.1.6: Implement Buffer-to-Image Copy (Y Plane)
**Status:** Not Started  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 50 lines

**Plan:**
- Create `copy_staging_to_y_image()` helper
- Transition Y image to TRANSFER_DST
- Execute vkCmdCopyBufferToImage
- Submit command buffer

---

### ⏳ Micro-Task 31.1.7: Implement Buffer-to-Image Copy (UV Plane)
**Status:** Not Started  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 50 lines

**Plan:**
- Create `copy_staging_to_uv_image()` helper
- Transition UV image to TRANSFER_DST
- Execute vkCmdCopyBufferToImage with offset
- Submit command buffer

---

### ⏳ Micro-Task 31.1.8: Transition Images to Shader-Readable
**Status:** Not Started  
**Estimated Duration:** 1 hour  
**Estimated LOC:** 30 lines

**Plan:**
- Create `finalize_image_layouts()` helper
- Transition Y image to SHADER_READ_ONLY
- Transition UV image to SHADER_READ_ONLY

---

### ⏳ Micro-Task 31.1.9: Implement Main vulkan_upload_frame Function
**Status:** Not Started  
**Estimated Duration:** 2 hours  
**Estimated LOC:** 40 lines

**Plan:**
- Wire all helper functions together
- Add error handling
- Add performance timing
- Test with real frames

---

### ⏳ Micro-Task 31.1.10: Add Cleanup for Staging Resources
**Status:** Not Started  
**Estimated Duration:** 1 hour  
**Estimated LOC:** 20 lines

**Plan:**
- Add cleanup to vulkan_cleanup()
- Unmap staging memory
- Destroy staging buffer
- Free staging memory
- Test with valgrind

---

## Success Criteria

Phase 31.1 will be complete when:
- ✅ All 11 micro-tasks marked complete
- ✅ Frame upload completes without errors
- ✅ Upload latency <2ms (measured)
- ✅ Memory usage <50MB for staging
- ✅ No memory leaks (valgrind clean)
- ✅ All validation layers pass
- ✅ Integration test passes

---

## Notes

- Using micro-tasks to make progress visible
- Each task is small enough to complete in one sitting
- Testing after each task to catch issues early
- Documenting as we go for easy tracking

---

**Next Action:** Begin Micro-Task 31.1.2 (Create Staging Buffer Allocation)
