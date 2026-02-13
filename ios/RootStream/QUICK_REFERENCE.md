# iOS Implementation Quick Reference

## Getting Started

### Prerequisites
- macOS with Xcode 14.0+
- iOS 15.0+ target device or simulator
- CocoaPods installed

### Installation
```bash
# Navigate to iOS directory
cd ios/RootStream

# Install dependencies
pod install

# Open workspace
open RootStream.xcworkspace
```

## Architecture Overview

### Component Hierarchy
```
RootStreamApp (Main Entry)
    └── MainTabView
        ├── PeerDiscoveryView → PeerDiscovery (mDNS)
        ├── StreamView → StreamingClient → MetalRenderer + VideoDecoder
        └── SettingsView → SettingsViewModel
```

### Data Flow
```
Server → NWConnection → StreamingClient → VideoDecoder → MetalRenderer → Display
                                        ↓
                                   AudioEngine → Speakers
                      ↑
         InputController ← OnScreenJoystick/Gamepad
```

## Key Components

### 1. Metal Rendering Pipeline
```swift
// In StreamView
MetalRenderView(renderer: streamingClient.renderer)

// MetalRenderer handles:
// 1. CVPixelBuffer → CVMetalTexture conversion
// 2. Metal command buffer creation
// 3. Fragment shader execution
// 4. Frame presentation at 60 FPS
```

### 2. Video Decoding
```swift
// VideoDecoder uses VideoToolbox
decoder.decode(encodedData) { pixelBuffer in
    renderer.renderFrame(pixelBuffer)
}

// Supports: H.264, H.265 (HEVC), VP9
// Hardware accelerated on all Metal-capable devices
```

### 3. Network Streaming
```swift
// StreamingClient connects with TLS
await streamingClient.connect(to: peer)

// Packet format:
// [type:1][timestamp:8][sequence:4][data:n]
```

### 4. Input System
```swift
// On-screen controls
OnScreenJoystick(inputController: inputController)
ActionButtonsView(inputController: inputController)

// Gamepad support (automatic)
// MFi, Xbox, PlayStation controllers
```

### 5. Security
```swift
// Keychain for credentials
keychainManager.store(username: user, password: pass)

// ChaCha20-Poly1305 encryption
let encrypted = try securityManager.encrypt(data)

// Biometric authentication
LAContext().evaluatePolicy(.deviceOwnerAuthentication...)
```

## Configuration

### Info.plist Keys
- `NSLocalNetworkUsageDescription`: For mDNS discovery
- `NSBonjourServices`: `_rootstream._tcp`
- `NSFaceIDUsageDescription`: For biometric auth
- `GCSupportsGameControllers`: For gamepad support
- `UIBackgroundModes`: For audio streaming

### Settings
Managed via `UserDefaultsManager`:
- Video codec (H.264/H.265/VP9)
- Bitrate (Mbps)
- Resolution (720p/1080p/4K)
- Target FPS (30/45/60)
- Audio enabled
- Haptic feedback
- Battery optimization

## Testing

### Run Tests
```bash
# In Xcode
Command + U

# Or via xcodebuild
xcodebuild test \
  -workspace RootStream.xcworkspace \
  -scheme RootStream \
  -destination 'platform=iOS Simulator,name=iPhone 15 Pro'
```

### Test Coverage
- ✅ Keychain storage/retrieval
- ✅ Settings persistence
- ✅ Packet serialization
- ✅ Encryption/decryption
- ✅ Video decoder performance
- ✅ Renderer performance

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| Video FPS | 60 | Adaptive based on battery |
| Audio Latency | <100ms | 5ms buffer size |
| Network Latency | <50ms | LAN only |
| Memory Usage | <200MB | Typical streaming |
| Battery Life | 3+ hours | 1080p@60fps |

## Optimization Features

### Battery Optimizer
```swift
// Automatic quality adjustment
if batteryLevel < 0.2 {
    recommendedFPS = 30
    recommendedResolution = .hd720p
}
```

### Thermal Management
```swift
// Reduce quality on overheating
if thermalState == .serious {
    recommendedFPS = min(fps, 30)
}
```

## Troubleshooting

### Common Issues

**1. Build Errors**
```bash
# Clean derived data
rm -rf ~/Library/Developer/Xcode/DerivedData

# Reinstall pods
pod deintegrate
pod install
```

**2. Metal Not Available**
- Requires iOS device with Metal support (iOS 8+)
- All modern iPhones/iPads supported

**3. Gamepad Not Detected**
- Enable Bluetooth
- Pair controller in Settings
- GCController.controllers() should show connected devices

**4. mDNS Discovery Fails**
- Check Info.plist for NSLocalNetworkUsageDescription
- Verify NSBonjourServices includes _rootstream._tcp
- Ensure device is on same network

**5. Video Decoder Errors**
- Check codec compatibility
- Verify VideoToolbox support for codec
- H.264 is universally supported

## Development Tips

### Debug Logging
```swift
// Enable verbose logging
print("FPS: \(renderer.getCurrentFPS())")
print("Latency: \(streamingClient.currentLatency)ms")
```

### Instruments Profiling
- Time Profiler: CPU usage
- Allocations: Memory leaks
- Network: Bandwidth usage
- Energy Log: Battery impact

### SwiftUI Previews
```swift
#Preview {
    StreamView()
        .environmentObject(AppState.shared)
}
```

## Future Enhancements

### Planned Features
- [ ] HDR video support
- [ ] 4K streaming optimization
- [ ] Multi-peer streaming
- [ ] Cloud relay for remote access
- [ ] Recording to file
- [ ] Picture-in-picture mode

### Integration Points
- Phase 21 security (✅ compatible)
- Server-side NVENC support
- Desktop client protocol sync
- Authentication server

## Resources

### Documentation
- [iOS README](README.md)
- [Phase 22.1 Summary](../PHASE22_1_SUMMARY.md)
- [Main README](../../README.md)

### Apple Frameworks
- [Metal Documentation](https://developer.apple.com/metal/)
- [VideoToolbox Guide](https://developer.apple.com/documentation/videotoolbox)
- [Network Framework](https://developer.apple.com/documentation/network)
- [GameController](https://developer.apple.com/documentation/gamecontroller)

### Third-Party
- [libopus](https://opus-codec.org/)
- [TrustKit](https://github.com/datatheorem/TrustKit)
