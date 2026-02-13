# RootStream KDE Plasma Client - User Guide

## Installation

### Arch Linux / CachyOS

```bash
# Install dependencies
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 \
               libsodium opus libva libpulse

# Clone repository
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream/clients/kde-plasma-client

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Install
sudo make install
```

### From PKGBUILD

```bash
cd RootStream/clients/kde-plasma-client/packaging
makepkg -si
```

## Quick Start

### Launching the Client

```bash
# Standard launch
rootstream-kde-client

# With AI logging enabled
rootstream-kde-client --ai-logging

# Auto-connect to peer
rootstream-kde-client --connect "kXx7YqZ3...@hostname"
```

### Connecting to a Host

1. **Launch the client**
2. **Start peer discovery** - Click "Start Discovery" to find RootStream hosts on your local network
3. **Connect** - Select a discovered peer and click "Connect", or manually enter a RootStream code
4. **Stream** - Video/audio will begin streaming automatically

### Keyboard Shortcuts

- **F11** - Toggle fullscreen mode
- **Escape** - Exit fullscreen
- **Ctrl+Q** - Quit application
- **Ctrl+D** - Disconnect from peer

## Settings

### Video Settings

- **Codec** - Select video codec (H.264, H.265, VP9, VP8)
- **Bitrate** - Adjust stream quality (1-50 Mbps)

### Audio Settings

- **Enable audio** - Toggle audio playback
- **Audio device** - Select audio output device

### Input Settings

- **Input mode** - Choose input injection method (uinput or xdotool)

### Advanced Settings

- **AI logging** - Enable debug logging for troubleshooting

## Troubleshooting

### Black Screen

1. Check that the host is streaming
2. Verify video codec is supported
3. Try lowering bitrate
4. Check "Diagnostics" in Help menu

### No Audio

1. Verify audio is enabled in settings
2. Check PulseAudio/PipeWire is running
3. Try different audio device
4. Restart the application

### Connection Failed

1. Ensure both devices are on the same network
2. Check firewall settings (UDP port 9876)
3. Verify RootStream code is correct
4. Try manual IP:port connection

### High Latency

1. Use wired ethernet instead of WiFi
2. Lower bitrate in settings
3. Close other network-intensive applications
4. Check router QoS settings

## Advanced Usage

### AI Logging Mode

Enable detailed logging for debugging:

```bash
rootstream-kde-client --ai-logging
```

Logs will be output to stderr in structured format:
```
[AICODING][timestamp][module] message
```

### Configuration Files

Settings are stored in:
```
~/.config/RootStream/KDE-Client.conf
```

Backup your settings:
```bash
cp ~/.config/RootStream/KDE-Client.conf ~/KDE-Client.conf.backup
```

## Support

- **GitHub**: https://github.com/infinityabundance/RootStream
- **Documentation**: See docs/ folder in repository
- **Issues**: Report bugs on GitHub Issues
