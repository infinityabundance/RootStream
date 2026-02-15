# Phase 31.2: YUV to RGB Shader System - Progress Tracker

## Overview
Implement shader loading and descriptor set management for YUV→RGB conversion in Vulkan renderer.

## Status: 2/9 Tasks Complete (22%)

**Time Used:** 1h / 8.5h (12%)  
**Code Written:** 0 / 370 LOC (0% - compilation only so far)

---

## Completed Tasks ✅

### Task 31.2.0: Planning & Analysis
**Duration:** 30 minutes  
**Status:** ✅ Complete

- Reviewed existing shader source files
- Found shaders already written (saves time!)
- Created 9 micro-task breakdown
- Identified integration points

### Task 31.2.1: Compile Shaders to SPIR-V
**Duration:** 30 minutes  
**Status:** ✅ Complete  
**Files Generated:** 2 (.spv binaries)

**What was done:**
- Installed glslang-tools
- Compiled fullscreen.vert → fullscreen.vert.spv (1.4 KB)
- Compiled nv12_to_rgb.frag → nv12_to_rgb.frag.spv (1.9 KB)
- Verified compilation successful

**Note:** .spv files are gitignored (generated files, built from source)

---

## Remaining Tasks ⏳

### Task 31.2.2: Add Shader Module Loading Function
**Status:** ⏳ Next  
**Duration:** ~1.5 hours  
**LOC:** ~80 lines

**Goal:** Create `load_shader_module()` helper function

**Implementation plan:**
```c
static VkShaderModule load_shader_module(vulkan_context_t *ctx, 
                                         const char *filepath) {
    // 1. Open .spv file (binary mode)
    // 2. Get file size with fseek/ftell
    // 3. Allocate buffer (malloc)
    // 4. Read entire file (fread)
    // 5. Create VkShaderModuleCreateInfo
    // 6. vkCreateShaderModule
    // 7. Free buffer
    // 8. Return module or VK_NULL_HANDLE
}
```

**Error cases:**
- File not found → set error, return VK_NULL_HANDLE
- Read failure → free buffer, set error, return VK_NULL_HANDLE
- Creation failure → free buffer, set error, return VK_NULL_HANDLE

---

### Task 31.2.3: Add Shader Fields to Context
**Status:** ⏳ Planned  
**Duration:** ~30 minutes  
**LOC:** ~10 lines

**Fields to add to vulkan_context_s:**
```c
VkShaderModule vert_shader_module;
VkShaderModule frag_shader_module;
bool shaders_loaded;
```

**Initialization:**
- Set to NULL/false in vulkan_init()
- Add cleanup in vulkan_cleanup()

---

### Task 31.2.4: Load Shaders in vulkan_init()
**Status:** ⏳ Planned  
**Duration:** ~1 hour  
**LOC:** ~40 lines

**Integration:**
- Call load_shader_module() twice (vert + frag)
- Check for errors
- Set shaders_loaded = true on success

---

### Task 31.2.5: Create Descriptor Set Layout
**Status:** ⏳ Planned  
**Duration:** ~1.5 hours  
**LOC:** ~70 lines

**Goal:** Define shader texture bindings

**Bindings:**
- Binding 0: Y texture (sampler2D)
- Binding 1: UV texture (sampler2D)

---

### Task 31.2.6: Create Descriptor Pool and Sets
**Status:** ⏳ Planned  
**Duration:** ~1.5 hours  
**LOC:** ~80 lines

**Goal:** Allocate descriptor sets for each swapchain image

---

### Task 31.2.7: Update Descriptor Sets with Images
**Status:** ⏳ Planned  
**Duration:** ~1 hour  
**LOC:** ~60 lines

**Goal:** Bind Y/UV images to descriptor sets

---

### Task 31.2.8: Cleanup Shader Resources
**Status:** ⏳ Planned  
**Duration:** ~30 minutes  
**LOC:** ~30 lines

**Goal:** Destroy shader modules and descriptor resources

---

## Progress Summary

| # | Task | Status | Time | LOC | 
|---|------|--------|------|-----|
| 31.2.0 | Planning | ✅ | 30m | 0 |
| 31.2.1 | Compile | ✅ | 30m | 0 |
| 31.2.2 | Loader | ⏳ | 1.5h | 80 |
| 31.2.3 | Fields | ⏳ | 30m | 10 |
| 31.2.4 | Init | ⏳ | 1h | 40 |
| 31.2.5 | Layout | ⏳ | 1.5h | 70 |
| 31.2.6 | Pool | ⏳ | 1.5h | 80 |
| 31.2.7 | Update | ⏳ | 1h | 60 |
| 31.2.8 | Cleanup | ⏳ | 30m | 30 |
| **TOTAL** | | **22%** | **1h/8.5h** | **0/370** |

---

## Files Modified

**Shader sources (already exist):**
- ✅ `clients/kde-plasma-client/src/renderer/shader/fullscreen.vert`
- ✅ `clients/kde-plasma-client/src/renderer/shader/nv12_to_rgb.frag`
- ✅ `clients/kde-plasma-client/src/renderer/shader/compile_shaders.sh`

**Generated (gitignored):**
- ✅ `clients/kde-plasma-client/src/renderer/shader/fullscreen.vert.spv`
- ✅ `clients/kde-plasma-client/src/renderer/shader/nv12_to_rgb.frag.spv`

**To modify:**
- ⏳ `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

---

## Next Step

Begin Task 31.2.2: Implement shader module loading function

**Updated:** February 15, 2026
