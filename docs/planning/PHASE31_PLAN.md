# Phase 31: Vulkan Renderer Core Implementation

**Start Date:** February 15, 2026  
**Goal:** Implement all stubbed Vulkan renderer functionality to enable video frame rendering  
**Priority:** 🔴 CRITICAL - Blocks client functionality

---

## Overview

Phase 31 transforms the Vulkan renderer from a framework with stubs into a fully functional rendering pipeline capable of displaying video frames. This is the critical path to getting the KDE Plasma client working.

**Current State:** 
- Vulkan initialization ✅
- Device selection ✅
- Surface creation ✅
- Swapchain creation ✅
- Frame upload ❌ (stub)
- Rendering pipeline ❌ (stub)
- Shader system ❌ (missing)
- Present mode switching ❌ (stub)
- Window resize ❌ (stub)

**Target State:**
- All components functional ✅
- Video frames render on screen ✅
- VSync toggle works ✅
- Window resize works ✅

---

## Subphase Breakdown

### Phase 31.1: Frame Upload Infrastructure
**Status:** ⏳ Not Started  
**Estimated:** 2-3 days, 200-250 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c` (line 908-917)

#### Current Code (Stub):
```c
int vulkan_upload_frame(vulkan_context_t *ctx, const frame_t *frame) {
    if (!ctx || !frame) {
        return -1;
    }
    
    // TODO: Implement frame upload
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "Frame upload not yet implemented");
    return -1;
}
```

#### Tasks:
- [ ] Create staging buffer for frame data
  - Allocate VK_BUFFER_USAGE_TRANSFER_SRC_BIT buffer
  - Map memory for CPU write
  - Size: `frame->width * frame->height * 1.5` (YUV 4:2:0)
  
- [ ] Create device-local image for frame storage
  - VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
  - Format: VK_FORMAT_R8_UNORM (for Y plane), VK_FORMAT_R8G8_UNORM (for UV)
  - Or use VK_FORMAT_G8_B8R8_2PLANE_420_UNORM if available
  
- [ ] Implement buffer-to-image copy
  - Copy Y plane: `width * height` bytes
  - Copy U plane: `(width/2) * (height/2)` bytes
  - Copy V plane: `(width/2) * (height/2)` bytes
  
- [ ] Add memory barriers for layout transitions
  - UNDEFINED → TRANSFER_DST_OPTIMAL (before copy)
  - TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL (after copy)
  
- [ ] Implement frame data validation
  - Check frame dimensions
  - Check YUV format
  - Validate pointer
  
- [ ] Write unit tests
  - Test with mock frames
  - Test error cases
  - Test different resolutions

#### Implementation Notes:
```c
// Pseudo-code structure:
int vulkan_upload_frame(vulkan_context_t *ctx, const frame_t *frame) {
    // 1. Validate inputs
    // 2. Map staging buffer
    // 3. Copy YUV data to staging buffer
    // 4. Unmap staging buffer
    // 5. Begin command buffer
    // 6. Transition image layout (UNDEFINED → TRANSFER_DST)
    // 7. Copy buffer to image
    // 8. Transition image layout (TRANSFER_DST → SHADER_READ)
    // 9. End and submit command buffer
    // 10. Update frame counter
    return 0;
}
```

#### Success Criteria:
- [ ] Frames upload without errors
- [ ] Memory usage is reasonable (<50MB for staging)
- [ ] Upload latency <2ms
- [ ] No memory leaks (valgrind clean)

---

### Phase 31.2: YUV to RGB Shader System
**Status:** ⏳ Not Started  
**Estimated:** 2 days, 150-200 LOC  
**Files:** New shader directory + integration code

#### Tasks:
- [ ] Create shader directory structure
  ```
  clients/kde-plasma-client/src/renderer/shaders/
  ├── yuv_to_rgb.vert    (vertex shader)
  ├── yuv_to_rgb.frag    (fragment shader)
  ├── compile.sh         (SPIR-V compiler script)
  └── README.md          (shader documentation)
  ```

- [ ] Write vertex shader (`yuv_to_rgb.vert`)
  ```glsl
  #version 450
  
  layout(location = 0) out vec2 fragTexCoord;
  
  vec2 positions[4] = vec2[](
      vec2(-1.0, -1.0),
      vec2( 1.0, -1.0),
      vec2(-1.0,  1.0),
      vec2( 1.0,  1.0)
  );
  
  vec2 texCoords[4] = vec2[](
      vec2(0.0, 0.0),
      vec2(1.0, 0.0),
      vec2(0.0, 1.0),
      vec2(1.0, 1.0)
  );
  
  void main() {
      gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
      fragTexCoord = texCoords[gl_VertexIndex];
  }
  ```

- [ ] Write fragment shader (`yuv_to_rgb.frag`)
  ```glsl
  #version 450
  
  layout(binding = 0) uniform sampler2D samplerY;
  layout(binding = 1) uniform sampler2D samplerU;
  layout(binding = 2) uniform sampler2D samplerV;
  
  layout(location = 0) in vec2 fragTexCoord;
  layout(location = 0) out vec4 outColor;
  
  void main() {
      float y = texture(samplerY, fragTexCoord).r;
      float u = texture(samplerU, fragTexCoord).r - 0.5;
      float v = texture(samplerV, fragTexCoord).r - 0.5;
      
      // BT.709 color space conversion
      float r = y + 1.5748 * v;
      float g = y - 0.1873 * u - 0.4681 * v;
      float b = y + 1.8556 * u;
      
      outColor = vec4(r, g, b, 1.0);
  }
  ```

- [ ] Create compilation script
  ```bash
  #!/bin/bash
  glslangValidator -V yuv_to_rgb.vert -o yuv_to_rgb.vert.spv
  glslangValidator -V yuv_to_rgb.frag -o yuv_to_rgb.frag.spv
  ```

- [ ] Add shader loading to renderer
  - Load SPIR-V files
  - Create shader modules
  - Store in context

- [ ] Create descriptor set layout
  - 3 samplers (Y, U, V)
  - Uniform buffer (if needed for color matrix)

- [ ] Write shader tests
  - Test compilation
  - Test loading
  - Validate SPIR-V

#### Implementation Notes:
```c
// Add to vulkan_context_t:
typedef struct {
    VkShaderModule vert_shader;
    VkShaderModule frag_shader;
    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
} shader_system_t;

// Functions to add:
int load_shaders(vulkan_context_t *ctx);
int create_descriptor_sets(vulkan_context_t *ctx);
void cleanup_shaders(vulkan_context_t *ctx);
```

#### Success Criteria:
- [ ] Shaders compile to SPIR-V
- [ ] Shaders load without errors
- [ ] Descriptor sets created successfully
- [ ] YUV→RGB conversion is mathematically correct

---

### Phase 31.3: Graphics Pipeline Implementation
**Status:** ⏳ Not Started  
**Estimated:** 2 days, 150-200 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

#### Current Code (TODO):
```c
// Line 982-985
// TODO: Bind pipeline and draw when shaders are loaded
// For now, just clear to black
// vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);
// vkCmdDraw(command_buffer, 4, 1, 0, 0);  // Draw fullscreen quad
```

#### Tasks:
- [ ] Create pipeline layout
  - Bind descriptor set layout
  - Add push constants (if needed)
  
- [ ] Configure pipeline state
  - Vertex input: Empty (fullscreen quad from vertex shader)
  - Input assembly: Triangle strip
  - Viewport: Dynamic
  - Rasterizer: Fill mode, no culling
  - Multisampling: Disabled
  - Color blending: Replace mode
  
- [ ] Create graphics pipeline
  - Attach vertex shader
  - Attach fragment shader
  - Attach pipeline layout
  - Set render pass compatibility
  
- [ ] Bind pipeline in render loop
  - Replace TODO comment
  - Add vkCmdBindPipeline call
  - Bind descriptor sets
  
- [ ] Write pipeline tests
  - Test creation
  - Test binding
  - Mock render pass

#### Implementation Notes:
```c
// Add to vulkan_context_t:
VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;

// Function to add:
int create_graphics_pipeline(vulkan_context_t *ctx) {
    // Pipeline layout
    VkPipelineLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &ctx->shader_system.descriptor_layout;
    
    vkCreatePipelineLayout(ctx->device, &layout_info, NULL, &ctx->pipeline_layout);
    
    // Shader stages
    VkPipelineShaderStageCreateInfo vert_stage = {...};
    VkPipelineShaderStageCreateInfo frag_stage = {...};
    
    // Vertex input (empty)
    VkPipelineVertexInputStateCreateInfo vertex_input = {...};
    
    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {...};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    
    // ... more state configuration ...
    
    // Create pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {...};
    vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, 
                             &pipeline_info, NULL, &ctx->graphics_pipeline);
    
    return 0;
}
```

#### Success Criteria:
- [ ] Pipeline creates without errors
- [ ] Pipeline binds successfully
- [ ] Render pass compatibility correct
- [ ] No validation errors

---

### Phase 31.4: Rendering Loop Completion
**Status:** ⏳ Not Started  
**Estimated:** 1-2 days, 100-150 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

#### Tasks:
- [ ] Update descriptor sets with uploaded frame
  - Bind Y texture
  - Bind U texture
  - Bind V texture
  
- [ ] Add draw commands
  - vkCmdBindPipeline
  - vkCmdBindDescriptorSets
  - vkCmdDraw (4 vertices, triangle strip)
  
- [ ] Wire upload → shader → draw pipeline
  - Ensure frame upload happens before render
  - Synchronize with semaphores
  
- [ ] Add frame synchronization
  - Wait for upload to complete
  - Signal when render is done
  
- [ ] Add performance metrics
  - Frame time
  - Upload time
  - Render time

#### Implementation Notes:
```c
int vulkan_render(vulkan_context_t *ctx) {
    // ... existing swapchain acquire code ...
    
    // Update descriptor sets with current frame
    VkDescriptorImageInfo image_infos[3] = {
        {ctx->sampler, ctx->y_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {ctx->sampler, ctx->u_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {ctx->sampler, ctx->v_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };
    
    VkWriteDescriptorSet writes[3] = {...};
    vkUpdateDescriptorSets(ctx->device, 3, writes, 0, NULL);
    
    // Begin render pass
    vkCmdBeginRenderPass(...);
    
    // Bind pipeline and draw
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           ctx->pipeline_layout, 0, 1, &ctx->descriptor_set, 0, NULL);
    vkCmdDraw(command_buffer, 4, 1, 0, 0);  // 4 vertices = fullscreen quad
    
    vkCmdEndRenderPass(...);
    
    // ... existing submit code ...
}
```

#### Success Criteria:
- [ ] Video frames render on screen
- [ ] Frame rate ≥60 FPS
- [ ] No tearing (with VSync)
- [ ] GPU usage reasonable (<30% idle)

---

### Phase 31.5: Present Mode Switching
**Status:** ⏳ Not Started  
**Estimated:** 1 day, 80-100 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c` (line 1071-1077)

#### Current Code (Stub):
```c
int vulkan_set_present_mode(vulkan_context_t *ctx, VkPresentModeKHR mode) {
    if (!ctx) {
        return -1;
    }
    
    ctx->present_mode = mode;
    
    // TODO: Recreate swapchain with new present mode
    
    return 0;
}
```

#### Tasks:
- [ ] Implement swapchain recreation
  - Wait for device idle
  - Destroy old swapchain
  - Query new present modes
  - Create new swapchain with requested mode
  - Recreate image views
  - Recreate framebuffers
  
- [ ] Add present mode validation
  - Check if mode is supported
  - Fall back to FIFO if not
  
- [ ] Add mode constants
  - FIFO (VSync on)
  - MAILBOX (triple buffering)
  - IMMEDIATE (VSync off)
  
- [ ] Write tests
  - Test mode switching
  - Test fallback
  - Test multiple switches

#### Implementation Notes:
```c
int vulkan_set_present_mode(vulkan_context_t *ctx, VkPresentModeKHR mode) {
    // Validate mode is supported
    if (!is_present_mode_supported(ctx, mode)) {
        mode = VK_PRESENT_MODE_FIFO_KHR;  // Fallback
    }
    
    // Wait for device to finish
    vkDeviceWaitIdle(ctx->device);
    
    // Cleanup old swapchain resources
    cleanup_swapchain_resources(ctx);
    
    // Recreate swapchain with new mode
    ctx->present_mode = mode;
    if (create_swapchain(ctx) != 0) {
        return -1;
    }
    
    // Recreate dependent resources
    if (create_image_views(ctx) != 0) return -1;
    if (create_framebuffers(ctx) != 0) return -1;
    
    return 0;
}
```

#### Success Criteria:
- [ ] VSync toggle works
- [ ] No tearing in FIFO mode
- [ ] Low latency in IMMEDIATE mode
- [ ] No crashes during switch

---

### Phase 31.6: Window Resize Support
**Status:** ⏳ Not Started  
**Estimated:** 1 day, 100-120 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c` (line 1082-1090)

#### Current Code (Stub):
```c
int vulkan_resize(vulkan_context_t *ctx, int width, int height) {
    if (!ctx || width <= 0 || height <= 0) {
        return -1;
    }
    
    ctx->width = width;
    ctx->height = height;
    
    // TODO: Recreate swapchain
    
    return 0;
}
```

#### Tasks:
- [ ] Implement swapchain recreation for resize
  - Similar to present mode switching
  - Update swapchain extent
  
- [ ] Update viewport and scissor
  - Dynamic viewport in pipeline
  - Set viewport in command buffer
  
- [ ] Handle minimization (width=0, height=0)
  - Skip rendering
  - Don't crash
  
- [ ] Add resize throttling
  - Don't recreate on every pixel change
  - Debounce resize events
  
- [ ] Write tests
  - Test various sizes
  - Test minimization
  - Test rapid resizing

#### Implementation Notes:
```c
int vulkan_resize(vulkan_context_t *ctx, int width, int height) {
    // Handle minimization
    if (width == 0 || height == 0) {
        ctx->minimized = true;
        return 0;
    }
    
    ctx->minimized = false;
    
    // Skip if size hasn't changed
    if (ctx->width == width && ctx->height == height) {
        return 0;
    }
    
    ctx->width = width;
    ctx->height = height;
    
    // Wait for device idle
    vkDeviceWaitIdle(ctx->device);
    
    // Recreate swapchain
    cleanup_swapchain_resources(ctx);
    if (create_swapchain(ctx) != 0) return -1;
    if (create_image_views(ctx) != 0) return -1;
    if (create_framebuffers(ctx) != 0) return -1;
    
    return 0;
}
```

#### Success Criteria:
- [ ] Window resizes smoothly
- [ ] No crashes during resize
- [ ] Minimization works
- [ ] No memory leaks

---

### Phase 31.7: Cleanup and Error Handling
**Status:** ⏳ Not Started  
**Estimated:** 1 day, 80-100 LOC  
**File:** `clients/kde-plasma-client/src/renderer/vulkan_renderer.c`

#### Tasks:
- [ ] Complete `vulkan_cleanup()` function
  - Free all Vulkan resources
  - Destroy pipeline
  - Destroy shaders
  - Destroy descriptor sets
  - Free staging buffers
  
- [ ] Add comprehensive error handling
  - Check all Vulkan calls
  - Set ctx->last_error on failures
  - Return proper error codes
  
- [ ] Fix memory leaks
  - Run with valgrind
  - Fix all leaks
  - Add tests for cleanup
  
- [ ] Add validation layers (debug builds)
  - Enable Khronos validation
  - Log validation errors
  - Assert on errors in debug
  
- [ ] Write cleanup tests
  - Test cleanup after errors
  - Test double-cleanup safety
  - Test null context

#### Implementation Notes:
```c
void vulkan_cleanup(vulkan_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->device) {
        vkDeviceWaitIdle(ctx->device);
        
        // Cleanup shaders
        if (ctx->shader_system.vert_shader) {
            vkDestroyShaderModule(ctx->device, ctx->shader_system.vert_shader, NULL);
        }
        if (ctx->shader_system.frag_shader) {
            vkDestroyShaderModule(ctx->device, ctx->shader_system.frag_shader, NULL);
        }
        
        // Cleanup pipeline
        if (ctx->graphics_pipeline) {
            vkDestroyPipeline(ctx->device, ctx->graphics_pipeline, NULL);
        }
        if (ctx->pipeline_layout) {
            vkDestroyPipelineLayout(ctx->device, ctx->pipeline_layout, NULL);
        }
        
        // Cleanup descriptor sets
        if (ctx->descriptor_pool) {
            vkDestroyDescriptorPool(ctx->device, ctx->descriptor_pool, NULL);
        }
        if (ctx->descriptor_layout) {
            vkDestroyDescriptorSetLayout(ctx->device, ctx->descriptor_layout, NULL);
        }
        
        // ... cleanup all other resources ...
        
        vkDestroyDevice(ctx->device, NULL);
    }
    
    if (ctx->instance) {
        vkDestroyInstance(ctx->instance, NULL);
    }
    
    free(ctx);
}
```

#### Success Criteria:
- [ ] valgrind reports 0 leaks
- [ ] Cleanup is idempotent
- [ ] Validation layers pass
- [ ] No crashes on cleanup

---

## Integration Testing

After all subphases complete, run full integration tests:

### Test 1: Basic Rendering
```bash
# Start host
./rootstream-host -c drm -e vaapi --bitrate 20000

# Start client
./rootstream-client --connect <host-ip>

# Expected: Video renders on screen
```

### Test 2: VSync Toggle
```bash
# In client, press key to toggle VSync
# Expected: Tearing appears/disappears
```

### Test 3: Window Resize
```bash
# Resize client window
# Expected: Video scales, no crashes
```

### Test 4: Memory Test
```bash
# Run for 5 minutes
valgrind --leak-check=full ./rootstream-client

# Expected: No leaks reported
```

### Test 5: Performance Test
```bash
# Measure metrics
# Expected:
# - Frame rate ≥60 FPS
# - Latency <16ms
# - GPU usage <30% idle
```

---

## Dependencies

### Build Dependencies:
```bash
sudo apt install libvulkan-dev vulkan-tools
sudo apt install glslang-tools spirv-tools
```

### Runtime Dependencies:
- Vulkan driver (Mesa/NVIDIA/AMD)
- Vulkan ICD loader
- SPIR-V shaders (compiled)

---

## Success Metrics

| Metric | Target | Actual |
|--------|--------|--------|
| Frame Rate | ≥60 FPS | TBD |
| Upload Latency | <2ms | TBD |
| Render Latency | <1ms | TBD |
| GPU Usage (idle) | <30% | TBD |
| Memory Leaks | 0 | TBD |
| Validation Errors | 0 | TBD |

---

## Risk Mitigation

### Risk: Shader compilation failures
**Mitigation:** Test shaders early, provide precompiled SPIR-V

### Risk: Vulkan driver incompatibilities
**Mitigation:** Test on multiple GPUs (Intel, AMD, NVIDIA)

### Risk: Memory leaks
**Mitigation:** Regular valgrind tests, early leak detection

### Risk: Performance issues
**Mitigation:** Profile early, optimize hot paths

---

## Timeline

```
Week 1:
  Day 1-3: Phase 31.1 (Frame Upload)
  Day 4-5: Phase 31.2 (Shaders)
  
Week 2:
  Day 1-2: Phase 31.3 (Pipeline)
  Day 3-4: Phase 31.4 (Rendering)
  Day 5:   Phase 31.5 (Present Mode)
  
Week 3:
  Day 1:   Phase 31.6 (Resize)
  Day 2:   Phase 31.7 (Cleanup)
  Day 3-5: Integration testing, bug fixes
```

**Total Duration:** 10-12 days

---

## Notes

- Each subphase should be committed separately
- Write tests before implementation (TDD)
- Use validation layers during development
- Profile performance after each subphase
- Document all Vulkan calls with comments

---

**Last Updated:** February 15, 2026  
**Status:** Planning Complete, Ready to Begin Implementation  
**Next Step:** Begin Phase 31.1 (Frame Upload Infrastructure)
