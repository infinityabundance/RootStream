# Proton Renderer - PHASE 13

## Overview

The Proton renderer provides compatibility for streaming games running under Proton/Wine with DXVK (DirectX 11) or VKD3D (DirectX 12) translation layers. This allows RootStream to stream Windows games running on Linux via Steam Proton.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│         VideoRenderer Abstraction Layer             │
│  (renderer.h - OpenGL, Vulkan, Proton)              │
└──────────────┬──────────────────────────────────────┘
               │
      ┌────────┼────────┐
      │        │        │
      ▼        ▼        ▼
 ┌─────────┐ ┌───────┐ ┌──────────┐
 │ OpenGL  │ │Vulkan │ │  Proton  │
 │(Phase11)│ │(Phase12)│(Phase 13)│
 └─────────┘ └───────┘ └──────────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
              ▼            ▼            ▼
          ┌─────────┐  ┌─────────┐  ┌─────────┐
          │  DXVK   │  │  VKD3D  │  │ Vulkan  │
          │ (D3D11) │  │ (D3D12) │  │ Backend │
          └─────────┘  └─────────┘  └─────────┘
```

## Components

### 1. Proton Detector (`proton_detector.h/c`)

Detects Proton environment by checking:
- `PROTON_VERSION` - Proton version string
- `WINE_PREFIX` - Wine prefix location
- `DXVK_HUD` - DXVK is active
- `VKD3D_SHADER_DEBUG` - VKD3D is active
- `STEAM_COMPAT_TOOL_PATHS` - Steam compatibility tool paths
- `SteamAppId` - Steam application ID

```c
proton_info_t info;
if (proton_detect(&info)) {
    printf("Running under Proton %s\n", info.proton_version);
    if (info.has_dxvk) {
        printf("DXVK %u.%u.%u detected\n", 
               info.dxvk_version.major,
               info.dxvk_version.minor,
               info.dxvk_version.patch);
    }
}
```

### 2. Proton Renderer (`proton_renderer.h/c`)

Main renderer implementation that:
- Initializes based on detected Proton environment
- Uses Vulkan as the underlying backend (DXVK/VKD3D both use Vulkan)
- Provides same API as OpenGL and Vulkan renderers
- Transparently handles frame upload and presentation

```c
proton_context_t *ctx = proton_init(native_window);
if (ctx) {
    const proton_info_t *info = proton_get_info(ctx);
    printf("Compatibility layer: %s\n", proton_get_compatibility_layer(ctx));
}
```

### 3. DXVK Interop (`dxvk_interop.h/c`)

Provides DXVK-specific functionality:
- Version detection
- Async shader compilation control
- Shader cache statistics
- GPU utilization monitoring

### 4. VKD3D Interop (`vkd3d_interop.h/c`)

Provides VKD3D-specific functionality:
- Version detection
- Shader debug mode control
- Compilation statistics
- GPU synchronization

### 5. Game Database (`proton_game_db.h/c`)

Database of known games with compatibility workarounds:
- Dota 2 (570)
- CS:GO (730)
- GTA V (271590)
- Fallout 4 (377160)
- Red Dead Redemption 2 (1174180)

```c
const game_workaround_t *workarounds[10];
int count = proton_game_db_lookup(570, workarounds, 10);
if (count > 0) {
    proton_game_db_apply_workaround(workarounds[0]);
}
```

### 6. Settings (`proton_settings.h/c`)

User-configurable settings:
- Enable/disable DXVK
- Enable/disable VKD3D
- Async shader compilation
- DXVK HUD
- Shader cache size limit
- Preferred DirectX version

## Build Configuration

Enable Proton renderer in CMake:

```cmake
cmake -DENABLE_RENDERER_PROTON=ON ..
```

This will automatically enable Vulkan renderer as well, since Proton depends on it.

## Environment Variables

The Proton renderer respects these environment variables:

### Detection Variables
- `PROTON_VERSION` - Proton version (e.g., "8.3", "9.0-GE")
- `WINEPREFIX` or `WINE_PREFIX` - Wine prefix path
- `SteamAppId` - Steam application ID

### DXVK Variables
- `DXVK_HUD` - Enable DXVK HUD (e.g., "fps,frametimes,gpuload")
- `DXVK_ASYNC` - Enable async shader compilation (set to "1")
- `DXVK_VERSION` - DXVK version string
- `DXVK_STATE_CACHE` - Enable state cache

### VKD3D Variables
- `VKD3D_SHADER_DEBUG` - Enable shader debug mode
- `VKD3D_VERSION` - VKD3D version string
- `VKD3D_CONFIG` - VKD3D configuration options

## Usage

### Automatic Detection

The renderer automatically detects Proton when using `RENDERER_AUTO`:

```c
renderer_t *renderer = renderer_create(RENDERER_AUTO, 1920, 1080);
renderer_init(renderer, native_window);
// Will use Proton renderer if running under Proton
```

### Explicit Selection

```c
renderer_t *renderer = renderer_create(RENDERER_PROTON, 1920, 1080);
if (renderer_init(renderer, native_window) == 0) {
    // Successfully initialized Proton renderer
}
```

### Configuration File

Settings are stored in `~/.rootstream_proton.conf`:

```
# RootStream Proton Settings
enable_dxvk=true
enable_vkd3d=true
enable_async_shader_compile=true
enable_dxvk_hud=false
shader_cache_max_mb=1024
preferred_directx_version=auto
```

## Performance Tuning

### Async Shader Compilation

Enable async compilation to avoid stuttering:

```bash
export DXVK_ASYNC=1
```

Or in settings:
```c
proton_settings_t settings;
proton_settings_load(&settings);
settings.enable_async_shader_compile = true;
proton_settings_save(&settings);
proton_settings_apply(&settings);
```

### Shader Cache

DXVK shader cache is stored in `~/.cache/dxvk-cache/`. Monitor size:

```c
uint32_t cache_mb = proton_get_shader_cache_size(ctx);
printf("Shader cache: %u MB\n", cache_mb);
```

## Troubleshooting

### Proton Not Detected

Check environment variables:
```bash
printenv | grep -E "PROTON|WINE|DXVK|VKD3D|Steam"
```

### Performance Issues

1. Enable async shader compilation: `DXVK_ASYNC=1`
2. Check shader cache size
3. Verify DirectX version matches game requirements
4. Apply game-specific workarounds from database

### Visual Artifacts

Check compatibility layer:
```c
const char *layer = proton_get_compatibility_layer(ctx);
bool is_d3d11 = proton_is_d3d11_game(ctx);
bool is_d3d12 = proton_is_d3d12_game(ctx);
```

## Limitations

1. **Detection Only**: Currently detects Proton environment but doesn't capture frames directly from DXVK/VKD3D
2. **Vulkan Backend**: Requires Vulkan renderer to be available
3. **Environment-Based**: Relies on environment variables set by Proton
4. **No Direct Interception**: Doesn't hook into DirectX calls directly

## Future Enhancements

- Direct frame capture from DXVK/VKD3D backbuffers
- VkInterop for zero-copy frame sharing
- D3D11/D3D12 API hooking for better integration
- Per-game performance profiles
- Automatic workaround application based on Steam App ID

## References

- [Proton GitHub](https://github.com/ValveSoftware/Proton)
- [DXVK GitHub](https://github.com/doitsujin/dxvk)
- [VKD3D-Proton GitHub](https://github.com/HansKristian-Work/vkd3d-proton)
- [Wine HQ](https://www.winehq.org/)

## License

Same as RootStream main project.
