# RootStream KDE Plasma Client

<p align="center">
  <img src="packaging/icon.svg" alt="RootStream KDE Client" width="128"/>
</p>

**Native KDE Plasma / Qt 6 client for RootStream secure P2P game streaming**

---

## Features

- ✅ **Native KDE Plasma integration** - Built with Qt 6 and QML
- ✅ **Secure P2P streaming** - Ed25519 + ChaCha20-Poly1305 encryption
- ✅ **Peer discovery** - Automatic mDNS discovery and manual peer entry
- ✅ **Low latency** - Hardware-accelerated decoding with VA-API
- ✅ **Audio streaming** - Opus codec with PulseAudio/PipeWire support
- ✅ **Settings persistence** - KConfig integration for settings
- ✅ **AI logging mode** - Debug logging for troubleshooting
- ✅ **Multiple codecs** - H.264, H.265, VP9, VP8 support
- ✅ **Fullscreen mode** - Optimized for gaming

## Quick Start

### Installation

#### Arch Linux / CachyOS

```bash
# Install dependencies
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 \
               libsodium opus libva libpulse

# Clone and build
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream/clients/kde-plasma-client
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

#### From PKGBUILD

```bash
cd packaging
makepkg -si
```

### Usage

```bash
# Launch the client
rootstream-kde-client

# With AI logging (for debugging)
rootstream-kde-client --ai-logging

# Auto-connect to peer
rootstream-kde-client --connect "kXx7YqZ3...@hostname"
```

## Documentation

- **[User Guide](docs/USER_GUIDE.md)** - Installation, usage, and troubleshooting
- **[Developer Guide](docs/DEVELOPER_GUIDE.md)** - Architecture and development
- **[Building](docs/BUILDING.md)** - Build instructions for all platforms
- **[Troubleshooting](docs/TROUBLESHOOTING.md)** - Common issues and solutions

## Architecture

```
┌─────────────────────────────────────────┐
│         QML UI Layer                     │
│  - Peer selection and discovery          │
│  - Video streaming view                  │
│  - Settings and configuration            │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│      C++ Qt Wrapper Classes              │
│  - RootStreamClient (API wrapper)        │
│  - PeerManager (discovery)               │
│  - VideoRenderer (OpenGL)                │
│  - AudioPlayer (PulseAudio/PipeWire)     │
│  - InputManager (uinput)                 │
│  - SettingsManager (KConfig)             │
└─────────────┬───────────────────────────┘
              │
┌─────────────▼───────────────────────────┐
│      RootStream C API (libRootStream)    │
│  - Network, Crypto, Codecs               │
│  - Discovery, Peer management            │
└──────────────────────────────────────────┘
```

## Development Status

### ✅ Implemented
- [x] Basic UI structure and navigation
- [x] Connection management
- [x] Peer discovery (partial)
- [x] Settings persistence
- [x] AI logging integration
- [x] Multiple codec support
- [x] Fullscreen mode

### 🚧 In Progress
- [ ] Video rendering (OpenGL integration)
- [ ] Audio playback (Opus decoding)
- [ ] Input injection (uinput/xdotool)
- [ ] Performance metrics display
- [ ] mDNS peer discovery

### 📋 Planned
- [ ] Virtual input overlay
- [ ] Gamepad support
- [ ] Recording playback
- [ ] HDR support
- [ ] Multi-monitor support

## Building from Source

### Dependencies

**Required:**
- Qt 6.4+
- libsodium
- Opus
- libRootStream

**Optional:**
- KDE Frameworks 6 (KConfig, CoreAddons)
- VA-API (hardware decoding)
- PulseAudio or PipeWire

See [BUILDING.md](docs/BUILDING.md) for detailed instructions.

### Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Test

```bash
ctest --verbose
```

## Contributing

We welcome contributions! See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

### Areas Needing Help

1. **Video rendering** - OpenGL texture upload and rendering
2. **Audio playback** - PulseAudio/PipeWire integration
3. **Input injection** - Mouse/keyboard event handling
4. **Performance optimization** - Reduce latency, improve FPS
5. **Testing** - Unit tests, integration tests, stress tests
6. **Documentation** - Tutorials, examples, API docs

## License

MIT License - see [LICENSE](../../LICENSE)

## Support

- **GitHub**: https://github.com/infinityabundance/RootStream
- **Issues**: https://github.com/infinityabundance/RootStream/issues
- **Documentation**: See docs/ folder

## Acknowledgments

Built on:
- [Qt 6](https://www.qt.io/) - UI framework
- [RootStream](https://github.com/infinityabundance/RootStream) - Streaming engine
- [libsodium](https://libsodium.org/) - Cryptography
- [Opus](https://opus-codec.org/) - Audio codec
- [VA-API](https://github.com/intel/libva) - Hardware decoding

---

**Secure P2P streaming for everyone** 🎮🔐
