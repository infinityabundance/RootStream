# RootStream Android Client

Native Android application for secure peer-to-peer game streaming.

## Overview

This is the Android implementation of RootStream, providing a complete streaming client with:

- **Vulkan/OpenGL ES rendering** for hardware-accelerated video
- **MediaCodec video decoding** (H.264, VP9, AV1)
- **Low-latency audio** with Opus codec support
- **mDNS peer discovery** for automatic host detection
- **Touch controls** with on-screen joystick and buttons
- **Gamepad support** for Xbox/PlayStation controllers
- **Sensor fusion** using gyroscope and accelerometer
- **Battery optimization** with adaptive quality
- **Picture-in-Picture** mode for background streaming
- **Modern Material Design 3** UI with Jetpack Compose

## Architecture

### Tech Stack

- **Language**: Kotlin 1.9.20
- **UI**: Jetpack Compose with Material Design 3
- **Architecture**: MVVM with Hilt dependency injection
- **Async**: Kotlin Coroutines and Flow
- **Build**: Gradle 8.2.0 with Kotlin DSL
- **Min SDK**: API 24 (Android 7.0)
- **Target SDK**: API 34 (Android 14)

### Project Structure

```
android/RootStream/
├── app/
│   ├── src/
│   │   ├── main/
│   │   │   ├── kotlin/com/rootstream/
│   │   │   │   ├── MainActivity.kt              # App entry point
│   │   │   │   ├── RootStreamApplication.kt     # Application class
│   │   │   │   ├── ui/                          # UI layer
│   │   │   │   │   ├── Navigation.kt            # Compose navigation
│   │   │   │   │   ├── screens/                 # Screen composables
│   │   │   │   │   │   ├── LoginScreen.kt
│   │   │   │   │   │   ├── PeerDiscoveryScreen.kt
│   │   │   │   │   │   ├── StreamScreen.kt
│   │   │   │   │   │   └── SettingsScreen.kt
│   │   │   │   │   ├── components/              # Reusable components
│   │   │   │   │   │   └── StatusOverlay.kt
│   │   │   │   │   └── theme/                   # Material Design theme
│   │   │   │   ├── viewmodel/                   # MVVM ViewModels
│   │   │   │   │   ├── LoginViewModel.kt
│   │   │   │   │   ├── PeerDiscoveryViewModel.kt
│   │   │   │   │   ├── StreamViewModel.kt
│   │   │   │   │   └── SettingsViewModel.kt
│   │   │   │   ├── network/                     # Network layer
│   │   │   │   │   ├── StreamingClient.kt       # TCP/TLS client
│   │   │   │   │   ├── PeerDiscovery.kt         # mDNS discovery
│   │   │   │   │   └── StreamingService.kt      # Foreground service
│   │   │   │   ├── rendering/                   # Rendering layer
│   │   │   │   │   ├── VulkanRenderer.kt        # Vulkan renderer
│   │   │   │   │   ├── OpenGLRenderer.kt        # OpenGL ES fallback
│   │   │   │   │   └── VideoDecoder.kt          # MediaCodec decoder
│   │   │   │   ├── audio/                       # Audio layer
│   │   │   │   │   ├── AudioEngine.kt           # AAudio/OpenSL ES
│   │   │   │   │   └── OpusDecoder.kt           # Opus decoder
│   │   │   │   ├── input/                       # Input layer
│   │   │   │   │   └── InputController.kt       # Touch/gamepad/sensors
│   │   │   │   ├── data/                        # Data layer
│   │   │   │   │   └── models/                  # Data models
│   │   │   │   ├── util/                        # Utilities
│   │   │   │   └── di/                          # Hilt DI modules
│   │   │   ├── res/                             # Android resources
│   │   │   │   ├── values/
│   │   │   │   │   ├── strings.xml
│   │   │   │   │   └── themes.xml
│   │   │   │   └── xml/
│   │   │   │       ├── network_security_config.xml
│   │   │   │       ├── backup_rules.xml
│   │   │   │       └── data_extraction_rules.xml
│   │   │   ├── cpp/                             # Native C++ code
│   │   │   │   ├── CMakeLists.txt
│   │   │   │   ├── vulkan_renderer.cpp
│   │   │   │   ├── opus_decoder.cpp
│   │   │   │   └── gles_utils.cpp
│   │   │   └── AndroidManifest.xml
│   │   ├── test/                                # Unit tests
│   │   └── androidTest/                         # Instrumented tests
│   ├── build.gradle.kts
│   └── proguard-rules.pro
├── build.gradle.kts
├── settings.gradle.kts
└── gradle/
```

## Implementation Status

### ✅ Phase 22.2.1: Project Setup (Complete)
- Android project structure created
- Gradle configuration with Kotlin DSL
- Hilt dependency injection setup
- ProGuard rules configured
- AndroidManifest with all permissions
- Material Design 3 theming

### ✅ Phase 22.2.2: UI Layer (Complete)
- Jetpack Compose navigation
- LoginScreen with authentication UI
- PeerDiscoveryScreen with peer list
- StreamScreen with video container
- SettingsScreen with configurations
- StatusOverlay component
- Material Design 3 implementation

### 🚧 Phase 22.2.3: Vulkan Rendering (Stub)
- VulkanRenderer class structure
- JNI bridge scaffolding
- Native C++ stub code
- **TODO**: Full Vulkan implementation

### 🚧 Phase 22.2.4: OpenGL ES Rendering (Stub)
- OpenGLRenderer class structure
- GLSurfaceView.Renderer implementation
- **TODO**: Complete shader and rendering pipeline

### 🚧 Phase 22.2.5: Video Decoding (Stub)
- VideoDecoder with MediaCodec structure
- Codec capability detection
- **TODO**: Full decoding pipeline

### 🚧 Phase 22.2.6: Audio Engine (Stub)
- AudioEngine class structure
- OpusDecoder JNI scaffolding
- **TODO**: AAudio/OpenSL ES implementation
- **TODO**: libopus integration

### 🚧 Phase 22.2.7-8: Input System (Stub)
- InputController structure
- Sensor listener implementation
- **TODO**: On-screen controls
- **TODO**: Gamepad integration
- **TODO**: Sensor fusion algorithm

### 🚧 Phase 22.2.9: Network Stack (Stub)
- StreamingClient structure
- Connection state management
- **TODO**: TLS/SSL implementation
- **TODO**: Protocol Buffers serialization

### 🚧 Phase 22.2.10: Peer Discovery (Stub)
- PeerDiscovery with NsdManager structure
- **TODO**: Complete mDNS resolution
- **TODO**: Peer list management

### 🚧 Phase 22.2.11-13: Security & Optimization (Not Started)
- **TODO**: SecurityManager integration (Phase 21)
- **TODO**: Battery optimization
- **TODO**: Picture-in-Picture mode

### ✅ Phase 22.2.14: Testing (Basic)
- JUnit4 test infrastructure
- Basic unit tests for models
- **TODO**: Comprehensive test suite

## Building

### Prerequisites

- Android Studio Hedgehog (2023.1.1) or later
- JDK 17
- Android SDK API 34
- NDK r25c or later (for native code)
- Gradle 8.2+

### Build Commands

```bash
# Debug build
./gradlew assembleDebug

# Release build (with ProGuard/R8)
./gradlew assembleRelease

# Run tests
./gradlew test

# Run instrumented tests
./gradlew connectedAndroidTest

# Install on device
./gradlew installDebug
```

## Running

1. Open project in Android Studio
2. Sync Gradle files
3. Connect Android device or start emulator (API 24+)
4. Click Run or use `./gradlew installDebug`

## Configuration

### Network Security

Edit `res/xml/network_security_config.xml` for certificate pinning:

```xml
<pin-set expiration="2025-01-01">
    <pin digest="SHA-256">base64encodedPin==</pin>
</pin-set>
```

### Permissions

Required permissions in AndroidManifest.xml:
- `INTERNET` - Network communication
- `ACCESS_NETWORK_STATE` - Network status
- `CHANGE_WIFI_MULTICAST_STATE` - mDNS discovery
- `RECORD_AUDIO` - Audio streaming
- `VIBRATE` - Haptic feedback
- `CAMERA` - Optional camera access
- `FOREGROUND_SERVICE` - Background streaming

## Testing

### Unit Tests

```bash
./gradlew test
```

### UI Tests

```bash
./gradlew connectedAndroidTest
```

### Manual Testing

1. Start RootStream host on local network
2. Launch Android app
3. Login with credentials
4. Discover peers via mDNS
5. Connect and start streaming

## Next Steps

### Immediate TODOs

1. **Vulkan Renderer** (Phase 22.2.3)
   - Implement native Vulkan initialization
   - Create render pipeline
   - Add frame rendering loop

2. **Video Decoding** (Phase 22.2.5)
   - Complete MediaCodec integration
   - Add frame buffer management
   - Implement sync with audio

3. **Network Client** (Phase 22.2.9)
   - TLS/SSL socket connection
   - Protocol Buffers serialization
   - Receive/send loops

4. **Peer Discovery** (Phase 22.2.10)
   - Complete mDNS resolution
   - Peer list updates via Flow
   - Timeout and cleanup

5. **Audio Engine** (Phase 22.2.6)
   - AAudio implementation
   - libopus integration via NDK
   - Low-latency buffer configuration

### Future Enhancements

- Hardware video encoding for peer-to-peer
- WebRTC integration for NAT traversal
- Cloud relay for remote connections
- Multi-monitor support
- Recording and playback

## Dependencies

### Kotlin/Android
- androidx.core:core-ktx:1.12.0
- androidx.compose.* (BOM 2023.10.01)
- androidx.navigation:navigation-compose:2.7.5
- androidx.hilt:hilt-navigation-compose:1.1.0
- com.google.dagger:hilt-android:2.48.1

### Network
- com.squareup.okhttp3:okhttp:4.12.0
- com.google.protobuf:protobuf-kotlin:3.24.4

### Security
- androidx.security:security-crypto:1.1.0-alpha06
- androidx.biometric:biometric:1.1.0

### Media
- androidx.media3:media3-exoplayer:1.2.0

### Testing
- junit:junit:4.13.2
- org.mockito:mockito-core:5.7.0
- androidx.test.espresso:espresso-core:3.5.1

### Native Libraries (TODO)
- libopus (Opus audio codec)
- libvulkan (Vulkan rendering)
- OpenSSL/BoringSSL (TLS encryption)

## Contributing

This is Phase 22.2 of the RootStream project. See main repository for contribution guidelines.

## License

MIT License - See repository root for details.

## Related Documentation

- [RootStream Main README](../../README.md)
- [iOS Client Implementation](../../ios/PHASE22_1_SUMMARY.md)
- [Phase 21 Security](../../PHASE21_SUMMARY.md)
- [Protocol Documentation](../../PROTOCOL.md)
- [Architecture Overview](../../ARCHITECTURE.md)
