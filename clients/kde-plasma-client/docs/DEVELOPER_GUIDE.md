# RootStream KDE Plasma Client - Developer Guide

## Architecture Overview

The KDE Plasma client is built using Qt 6 and QML, providing a native KDE experience for streaming from RootStream hosts.

### Component Structure

```
┌─────────────────────────────────────────┐
│         QML UI Layer                     │
│  (main.qml, views, dialogs)             │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│      C++ Qt Wrapper Classes              │
│  - RootStreamClient (main API wrapper)   │
│  - PeerManager (discovery/list)          │
│  - VideoRenderer (OpenGL rendering)      │
│  - AudioPlayer (playback)                │
│  - InputManager (event injection)        │
│  - SettingsManager (config)              │
│  - LogManager (AI logging)               │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│      RootStream C API                    │
│      (libRootStream)                     │
│  - Network, Crypto, Codecs               │
│  - Discovery, Peer management            │
└──────────────────────────────────────────┘
```

## Building from Source

### Dependencies

**Required:**
- Qt 6.4+ (Core, Gui, Qml, Quick, Widgets, OpenGL)
- libRootStream (from main repository)
- libsodium (crypto)
- Opus (audio codec)

**Optional:**
- KDE Frameworks 6 (KConfig, CoreAddons)
- VA-API (hardware decoding)
- PulseAudio or PipeWire (audio)

### Build Steps

```bash
cd RootStream/clients/kde-plasma-client
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_AI_LOGGING=ON \
      ..

# Build
make -j$(nproc)

# Run
./rootstream-kde-client
```

### Build Options

- `CMAKE_BUILD_TYPE` - Debug or Release
- `ENABLE_AI_LOGGING` - Enable AI logging support (default: ON)

## Code Structure

### RootStreamClient Class

Main wrapper around the RootStream C API.

**Key Methods:**
- `connectToPeer()` - Initiate connection to host
- `disconnect()` - Disconnect from host
- `setVideoCodec()` - Configure video codec
- `setBitrate()` - Set streaming bitrate
- `processEvents()` - Event loop for network I/O

**Signals:**
- `connected()` - Emitted when connection established
- `disconnected()` - Emitted when disconnected
- `videoFrameReceived()` - New video frame available
- `performanceMetrics()` - FPS/latency updates

### PeerManager Class

Manages peer discovery and list.

Inherits from `QAbstractListModel` for direct QML integration.

**Key Methods:**
- `startDiscovery()` - Begin peer discovery
- `addManualPeer()` - Add peer by code
- `removePeer()` - Remove peer from list

### VideoRenderer Class

(To be implemented)

Will handle:
- Video frame decoding via RootStream decoders
- OpenGL texture upload
- Rendering to Qt Quick Scene Graph

### AudioPlayer Class

(To be implemented)

Will handle:
- Opus audio decoding
- PulseAudio/PipeWire playback
- Audio/video synchronization

### InputManager Class

(To be implemented)

Will handle:
- Mouse/keyboard event capture from QML
- uinput injection (Linux)
- Gamepad support

## QML Integration

### Exposing C++ Classes to QML

Classes are registered in `main.cpp`:

```cpp
qmlRegisterType<RootStreamClient>("RootStream", 1, 0, "RootStreamClient");
```

Context properties are set for singleton instances:

```cpp
engine.rootContext()->setContextProperty("rootStreamClient", &client);
```

### QML Property Bindings

C++ properties are exposed via `Q_PROPERTY`:

```cpp
Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
```

QML can bind to these:

```qml
visible: rootStreamClient.connected
```

## Testing

### Unit Tests

```bash
cd build
ctest --verbose
```

### Manual Testing

1. Start a RootStream host
2. Launch the client with debug logging:
   ```bash
   ./rootstream-kde-client --ai-logging 2>&1 | tee client.log
   ```
3. Connect to host
4. Verify video/audio streaming
5. Test disconnection/reconnection

## Contributing

### Code Style

- C++: Follow Qt coding conventions
- QML: 4-space indentation
- Comments: Doxygen-style for public APIs

### Adding New Features

1. Create feature branch
2. Implement with tests
3. Update documentation
4. Submit pull request

### Debugging Tips

**Enable all logging:**
```bash
QT_LOGGING_RULES="*=true" ./rootstream-kde-client --ai-logging
```

**Debug Qt Quick:**
```bash
QT_QUICK_BACKEND=software ./rootstream-kde-client
```

**Memory leak detection:**
```bash
valgrind --leak-check=full ./rootstream-kde-client
```

## API Reference

See `API_BINDINGS.md` for complete C++ API documentation.

## Roadmap

### Phase 1 (Current)
- [x] Basic UI structure
- [x] Connection management
- [x] Settings persistence
- [ ] Video rendering
- [ ] Audio playback

### Phase 2
- [ ] Input injection
- [ ] Performance metrics
- [ ] Recording support

### Phase 3
- [ ] Virtual input overlay
- [ ] Gamepad support
- [ ] HDR support
