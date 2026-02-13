# Proton Support in RootStream KDE Client

## Overview

**PHASE 13** adds Proton/Wine compatibility to RootStream, enabling seamless streaming of Windows games running on Linux via Steam Proton. The Proton renderer automatically detects DXVK (DirectX 11) and VKD3D (DirectX 12) environments and provides optimized performance for gaming workloads.

## Features

✅ **Automatic Proton Detection** - Detects Proton/Wine environment automatically  
✅ **DXVK Support** - Full support for DirectX 11 games via DXVK  
✅ **VKD3D Support** - Full support for DirectX 12 games via VKD3D  
✅ **Game Database** - Built-in workarounds for 5+ popular games  
✅ **Shader Cache** - Optimized shader compilation and caching  
✅ **Settings** - User-configurable Proton settings  

## Quick Start

### Check if Proton is Available

```bash
# Set environment (if not already in Proton):
export PROTON_VERSION=8.3
export WINEPREFIX=~/.steam/steam/steamapps/compatdata/570/pfx
export SteamAppId=570

# Run the client
rootstream-kde-client
```

The client will automatically detect Proton and use the appropriate renderer.

### Test Proton Detection

```bash
cd clients/kde-plasma-client/src/renderer
gcc -o proton_demo proton_test_demo.c proton_detector.c proton_game_db.c proton_settings.c -I.

# Test without Proton:
./proton_demo

# Test with Proton:
PROTON_VERSION=8.3 DXVK_VERSION=1.10.3 SteamAppId=570 ./proton_demo
```

## Environment Variables

### Proton Detection
- `PROTON_VERSION` - Proton version (e.g., "8.3", "9.0-GE")
- `WINEPREFIX` - Wine prefix path
- `SteamAppId` - Steam application ID

### DXVK (DirectX 11)
- `DXVK_HUD` - Enable DXVK HUD (e.g., "fps,frametimes")
- `DXVK_ASYNC` - Enable async shader compilation (set to "1")
- `DXVK_VERSION` - DXVK version string
- `DXVK_STATE_CACHE` - Enable state cache

### VKD3D (DirectX 12)
- `VKD3D_SHADER_DEBUG` - Enable shader debug mode
- `VKD3D_VERSION` - VKD3D version string
- `VKD3D_CONFIG` - VKD3D configuration options

## Supported Games

The game database includes workarounds for:
- **Dota 2** (570) - Shader compilation optimization
- **CS:GO** (730) - Frame pacing fixes
- **GTA V** (271590) - Memory optimization
- **Fallout 4** (377160) - D3D11 performance
- **Red Dead Redemption 2** (1174180) - VKD3D/D3D12 support

## Configuration

Settings are stored in `~/.rootstream_proton.conf`:

```ini
# RootStream Proton Settings
enable_dxvk=true
enable_vkd3d=true
enable_async_shader_compile=true
enable_dxvk_hud=false
shader_cache_max_mb=1024
preferred_directx_version=auto
```

## Build Options

```bash
cmake .. -DENABLE_RENDERER_PROTON=ON
```

Note: Proton renderer requires Vulkan renderer to be enabled (it will be automatically enabled).

## Performance Tips

### Enable Async Shader Compilation
```bash
export DXVK_ASYNC=1
```

### Monitor Shader Cache
```bash
du -sh ~/.cache/dxvk-cache/
```

### Enable DXVK HUD
```bash
export DXVK_HUD=fps,frametimes,gpuload
```

## Troubleshooting

### Proton Not Detected
1. Check environment variables: `printenv | grep -E "PROTON|WINE|DXVK"`
2. Verify Wine prefix exists: `ls -la $WINEPREFIX`
3. Check for DXVK/VKD3D DLLs: `ls $WINEPREFIX/drive_c/windows/system32/*.dll`

### Performance Issues
1. Enable async shader compilation: `DXVK_ASYNC=1`
2. Clear shader cache: `rm -rf ~/.cache/dxvk-cache/*`
3. Check workarounds for your game in the database

### Visual Artifacts
1. Try different DirectX version preference
2. Check DXVK/VKD3D version compatibility
3. Update Proton to latest version

## Architecture

```
┌──────────────────────────────────┐
│    RootStream KDE Client         │
└────────────┬─────────────────────┘
             │
             ▼
┌──────────────────────────────────┐
│     Proton Renderer              │
│  - Proton Detection              │
│  - Game Database                 │
│  - Settings Management           │
└────────────┬─────────────────────┘
             │
      ┌──────┴──────┐
      ▼             ▼
┌──────────┐  ┌──────────┐
│   DXVK   │  │  VKD3D   │
│ (D3D11)  │  │ (D3D12)  │
└─────┬────┘  └────┬─────┘
      │            │
      └──────┬─────┘
             ▼
    ┌──────────────┐
    │    Vulkan    │
    │   Backend    │
    └──────────────┘
```

## API Reference

See [README_PROTON.md](../src/renderer/README_PROTON.md) for detailed API documentation.

## Contributing

To add new game workarounds, edit:
- `clients/kde-plasma-client/src/renderer/proton_game_db.c`

To improve Proton detection, edit:
- `clients/kde-plasma-client/src/renderer/proton_detector.c`

## License

Same as RootStream main project (MIT).

---

**Stream Windows games on Linux via Proton!** 🎮🐧
