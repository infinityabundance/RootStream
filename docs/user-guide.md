# RootStream User Guide

A comprehensive guide to using RootStream for secure peer-to-peer game streaming.

## Table of Contents

1. [Installation](#installation)
2. [Quick Start](#quick-start)
3. [Host Setup](#host-setup)
4. [Client Setup](#client-setup)
5. [Configuration](#configuration)
6. [Recording & Playback](#recording--playback)
7. [Troubleshooting](#troubleshooting)
8. [Settings Reference](#settings-reference)

---

## Installation

### Ubuntu / Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential pkg-config \
    libdrm-dev libva-dev libsodium-dev \
    libopus-dev libasound2-dev libsdl2-dev \
    libgtk-3-dev libavahi-client-dev \
    libqrencode-dev libpng-dev

# Build and install
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream
make
sudo make install
```

### Fedora / RHEL

```bash
# Install dependencies
sudo dnf install -y \
    gcc make pkg-config \
    libdrm-devel libva-devel libsodium-devel \
    opus-devel alsa-lib-devel SDL2-devel \
    gtk3-devel avahi-devel \
    qrencode-devel libpng-devel

# Build and install
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream
make
sudo make install
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S --needed \
    base-devel pkg-config \
    libdrm libva libsodium \
    opus alsa-lib sdl2 \
    gtk3 avahi \
    qrencode libpng

# Build and install
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream
make
sudo make install

# Or use the PKGBUILD
makepkg -si
```

---

## Quick Start

### 1. Generate Your Identity

First time setup generates your cryptographic identity:

```bash
rootstream --qr
```

This displays:
- Your **RootStream Code** (share this with peers)
- A **QR code** for easy mobile scanning

### 2. Start Hosting

On the computer you want to stream FROM:

```bash
rootstream host
```

### 3. Connect as Client

On the computer you want to stream TO:

```bash
rootstream client <rootstream-code>
```

Replace `<rootstream-code>` with the code from step 1 (e.g., `kXx7Y...@gaming-pc`).

---

## Host Setup

The host is the computer running the game or application you want to stream.

### Basic Hosting

```bash
# Start hosting with default settings
rootstream host

# Host on a specific port
rootstream host --port 9876

# Host with custom bitrate (10 Mbps)
rootstream host --bitrate 10000000

# Host with H.265/HEVC codec
rootstream host --codec h265
```

### Background Service

Run RootStream as a system service for automatic startup:

```bash
# Enable and start the service
systemctl --user enable rootstream.service
systemctl --user start rootstream.service

# Check status
systemctl --user status rootstream.service

# View logs
journalctl --user -u rootstream.service -f
```

### Recording While Streaming

Record your stream to a file:

```bash
# Record to file while streaming
rootstream host --record gameplay.rstr

# Play back the recording
rstr-player gameplay.rstr
```

### Multi-Monitor Setup

If you have multiple displays:

```bash
# List available displays
rootstream --list-displays

# Stream a specific display
rootstream host --display 1
```

---

## Client Setup

The client receives and displays the stream from a host.

### Connecting to a Host

```bash
# Connect using RootStream code
rootstream client kXx7Y...@gaming-pc

# Connect using IP address
rootstream client kXx7Y...@192.168.1.100

# Connect with custom port
rootstream client kXx7Y...@gaming-pc --port 9876
```

### Client Controls

While connected:
- **Escape**: Disconnect and exit
- **F11**: Toggle fullscreen
- **Mouse/Keyboard**: Input is sent to host

### Audio

Audio is streamed automatically using Opus codec. Ensure your audio output device is configured correctly:

```bash
# List audio devices
aplay -l

# Test audio playback
speaker-test -c 2
```

---

## Configuration

RootStream stores configuration in `~/.config/rootstream/`.

### Configuration File

Edit `~/.config/rootstream/config.ini`:

```ini
[video]
bitrate = 10000000
framerate = 60
codec = h264

[audio]
enabled = true
bitrate = 128000

[network]
port = 9876
discovery = true
```

### Key Storage

Your cryptographic keys are stored in `~/.config/rootstream/keys/`:
- `private.key` - Your private key (keep secret!)
- `public.key` - Your public key

**Warning**: Never share your `private.key` file.

---

## Recording & Playback

### Recording a Stream

```bash
# Record while hosting
rootstream host --record session.rstr

# Recording is saved in RootStream format (.rstr)
```

### Playing Recordings

```bash
# Play a recording
rstr-player recording.rstr

# Get recording info
rstr-player --info recording.rstr
```

### Recording Format

The `.rstr` format contains:
- H.264/H.265 encoded video frames
- Opus encoded audio
- Frame timestamps
- Keyframe markers

---

## Troubleshooting

### "Cannot open DRM device"

**Cause**: No permission to access GPU.

**Fix**:
```bash
# Add user to video group
sudo usermod -aG video $USER

# Log out and back in, or:
newgrp video
```

### "VA-API initialization failed"

**Cause**: Missing or misconfigured VA-API drivers.

**Fix**:
```bash
# Intel GPUs
sudo apt install intel-media-va-driver

# AMD GPUs
sudo apt install mesa-va-drivers

# Verify VA-API works
vainfo
```

### "Cannot bind to port"

**Cause**: Port already in use or permission denied.

**Fix**:
```bash
# Check what's using the port
sudo lsof -i :9876

# Use a different port
rootstream host --port 9877
```

### "Connection timeout"

**Cause**: Network or firewall issues.

**Fix**:
```bash
# Check firewall
sudo ufw allow 9876/udp

# Or with firewalld
sudo firewall-cmd --add-port=9876/udp --permanent
sudo firewall-cmd --reload
```

### High Latency

**Symptoms**: Noticeable delay between input and response.

**Fixes**:
1. Use wired Ethernet instead of WiFi
2. Reduce bitrate: `--bitrate 5000000`
3. Enable low-latency mode (default)
4. Check for network congestion

### Video Artifacts / Glitches

**Symptoms**: Visual corruption, green frames, blocky video.

**Fixes**:
1. Request a keyframe (client will auto-request on errors)
2. Increase bitrate for better quality
3. Check GPU temperature (throttling)
4. Update GPU drivers

### No Audio

**Symptoms**: Video works but no sound.

**Fixes**:
```bash
# Check ALSA configuration
aplay -l

# Verify PulseAudio/PipeWire
pactl list sinks

# Check RootStream audio settings
# In config.ini: audio.enabled = true
```

---

## Settings Reference

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--port PORT` | UDP port for streaming | 9876 |
| `--bitrate BPS` | Video bitrate in bits/sec | 10000000 |
| `--fps FPS` | Target framerate | 60 |
| `--codec CODEC` | Video codec (h264, h265) | h264 |
| `--display N` | Display index to capture | 0 |
| `--record FILE` | Record stream to file | - |
| `--qr` | Show QR code and exit | - |
| `--latency` | Enable latency logging | false |

### Config.ini Options

#### [video]
| Option | Description | Default |
|--------|-------------|---------|
| `bitrate` | Target video bitrate | 10000000 |
| `framerate` | Target FPS | 60 |
| `codec` | h264 or h265 | h264 |

#### [audio]
| Option | Description | Default |
|--------|-------------|---------|
| `enabled` | Enable audio streaming | true |
| `bitrate` | Opus audio bitrate | 128000 |

#### [network]
| Option | Description | Default |
|--------|-------------|---------|
| `port` | Default UDP port | 9876 |
| `discovery` | Enable mDNS discovery | true |

### Environment Variables

| Variable | Description |
|----------|-------------|
| `XDG_CONFIG_HOME` | Config directory (default: ~/.config) |
| `ROOTSTREAM_DEBUG` | Enable debug output |

---

## Getting Help

- **GitHub Issues**: https://github.com/infinityabundance/RootStream/issues
- **Documentation**: https://github.com/infinityabundance/RootStream/tree/main/docs

---

*RootStream - Secure Peer-to-Peer Game Streaming*
