# RootStream iOS

Native iOS client for RootStream game streaming.

## Features

- ✅ Native iOS application with SwiftUI
- ✅ Metal-based hardware-accelerated rendering
- ✅ VideoToolbox hardware video decoding (H.264/H.265/VP9)
- ✅ Low-latency audio with Opus codec
- ✅ mDNS peer discovery
- ✅ On-screen controls and gamepad support
- ✅ End-to-end encryption
- ✅ Battery and thermal optimization
- ✅ Biometric authentication (Face ID/Touch ID)

## Requirements

- iOS 15.0+
- Xcode 14.0+
- CocoaPods
- Metal-capable device

## Installation

1. Clone the repository
2. Navigate to the iOS directory:
   ```bash
   cd ios/RootStream
   ```
3. Install dependencies:
   ```bash
   pod install
   ```
4. Open the workspace:
   ```bash
   open RootStream.xcworkspace
   ```
5. Build and run in Xcode

## Project Structure

```
RootStream/
├── App/                    # Application lifecycle and state
│   ├── RootStreamApp.swift
│   └── AppState.swift
├── UI/                     # SwiftUI views
│   ├── MainTabView.swift
│   ├── LoginView.swift
│   ├── PeerDiscoveryView.swift
│   ├── StreamView.swift
│   ├── SettingsView.swift
│   └── StatusBar.swift
├── Network/                # Networking and peer discovery
│   ├── StreamingClient.swift
│   └── PeerDiscovery.swift
├── Rendering/              # Metal rendering and video decoding
│   ├── MetalRenderer.swift
│   ├── VideoDecoder.swift
│   └── Shaders.metal
├── Audio/                  # Audio engine and Opus decoder
│   ├── AudioEngine.swift
│   └── OpusDecoder.swift
├── Input/                  # Input handling
│   ├── InputController.swift
│   ├── OnScreenJoystick.swift
│   └── SensorFusion.swift
├── Utils/                  # Utilities and managers
│   ├── KeychainManager.swift
│   ├── UserDefaultsManager.swift
│   ├── SecurityManager.swift
│   └── BatteryOptimizer.swift
├── Models/                 # Data models
│   ├── Peer.swift
│   └── StreamPacket.swift
└── Resources/
    └── Info.plist
```

## Architecture

### Rendering Pipeline
1. **StreamingClient** receives encoded video packets
2. **VideoDecoder** uses VideoToolbox for hardware decoding
3. **MetalRenderer** renders decoded frames using Metal API
4. Display at 60 FPS with low latency

### Audio Pipeline
1. **StreamingClient** receives Opus-encoded audio
2. **OpusDecoder** decodes to PCM
3. **AudioEngine** plays audio with AVAudioEngine
4. Low-latency audio output (<100ms)

### Input Pipeline
1. **OnScreenJoystick** and buttons capture touch input
2. **GameController** framework handles MFi gamepads
3. **SensorFusion** collects motion data
4. **InputController** sends events to host

### Network Architecture
- **NWConnection** for TCP/TLS streaming
- **NWBrowser** for mDNS peer discovery
- **TLS/SSL** encryption using Security framework
- **ChaCha20-Poly1305** for stream encryption

## Security

- Session token storage in Keychain
- Certificate pinning with TrustKit
- Biometric authentication (Face ID/Touch ID)
- End-to-end encryption with ChaCha20-Poly1305
- Integration with Phase 21 security system

## Performance Optimizations

- **Battery Optimization**: Adaptive FPS and resolution scaling
- **Thermal Management**: Automatic quality reduction on overheating
- **Memory Management**: Efficient texture caching and cleanup
- **Low Power Mode**: Reduced settings when battery is low

## Testing

Run tests in Xcode:
```bash
Command + U
```

Test coverage includes:
- Unit tests for core components
- Integration tests for end-to-end streaming
- Performance benchmarks for rendering and decoding
- UI tests with XCTest

## CI/CD

GitHub Actions workflow for:
- Building iOS app
- Running tests
- Performance benchmarks
- Code quality checks

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md)

## License

MIT License - see [LICENSE](../../LICENSE)
