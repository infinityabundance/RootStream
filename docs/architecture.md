# RootStream Architecture

Technical architecture documentation for developers.

## Table of Contents

1. [System Overview](#system-overview)
2. [Data Flow](#data-flow)
3. [Protocol Specification](#protocol-specification)
4. [Component Details](#component-details)
5. [Security Model](#security-model)
6. [Adding New Encoders](#adding-new-encoders)
7. [Performance Considerations](#performance-considerations)

---

## System Overview

RootStream is a peer-to-peer game streaming application with the following architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                          HOST                                   │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐     │
│  │   DRM    │──▶│  VA-API  │──▶│  ChaCha  │──▶│   UDP    │─────┼──▶
│  │ Capture  │   │  Encode  │   │ Encrypt  │   │  Send    │     │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘     │
│       │                                                         │
│       ▼                                                         │
│  ┌──────────┐   ┌──────────┐                                   │
│  │   ALSA   │──▶│   Opus   │──────────────────────────────────┼──▶
│  │ Capture  │   │  Encode  │                                   │
│  └──────────┘   └──────────┘                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Network (UDP)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                          CLIENT                                 │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐     │
│  │   UDP    │──▶│  ChaCha  │──▶│  VA-API  │──▶│   SDL2   │     │
│  │ Receive  │   │ Decrypt  │   │  Decode  │   │ Display  │     │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘     │
│                                                    │            │
│  ┌──────────┐   ┌──────────┐                      │            │
│  │   Opus   │──▶│   ALSA   │                      ▼            │
│  │  Decode  │   │ Playback │              ┌──────────┐         │
│  └──────────┘   └──────────┘              │  uinput  │─────────┼──▶
│                                           │  Input   │         │
│                                           └──────────┘         │
└─────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

### Host Pipeline

```
Frame Capture (DRM/KMS)
         │
         ▼ RGBA pixels
    ┌─────────┐
    │ Convert │ RGBA → NV12 (YUV 4:2:0)
    │ to NV12 │ BT.709 color matrix
    └─────────┘
         │
         ▼ NV12 surface
    ┌─────────┐
    │ VA-API  │ Hardware H.264/H.265
    │ Encode  │ encoding
    └─────────┘
         │
         ▼ NAL units
    ┌─────────┐
    │ ChaCha  │ ChaCha20-Poly1305
    │ Encrypt │ AEAD encryption
    └─────────┘
         │
         ▼ Ciphertext + MAC
    ┌─────────┐
    │  UDP    │ Fragmented if
    │  Send   │ > MTU
    └─────────┘
```

### Client Pipeline

```
    ┌─────────┐
    │  UDP    │ Reassemble
    │ Receive │ fragments
    └─────────┘
         │
         ▼ Ciphertext
    ┌─────────┐
    │ ChaCha  │ Verify MAC &
    │ Decrypt │ decrypt
    └─────────┘
         │
         ▼ NAL units
    ┌─────────┐
    │ VA-API  │ Hardware decode
    │ Decode  │ to NV12
    └─────────┘
         │
         ▼ NV12 surface
    ┌─────────┐
    │  SDL2   │ NV12 → RGB
    │ Display │ via texture
    └─────────┘
```

### Latency Budget (Design Target: <30ms)

> These are design targets. Actual latency varies by hardware and network conditions.

| Stage | Target | Notes |
|-------|--------|-------|
| Capture | 1-2ms | DRM atomic commit timing |
| Colorspace | ~1ms | SIMD-optimized |
| Encode | 2-5ms | Hardware encoder (varies by GPU) |
| Encrypt | <1ms | ChaCha20 is fast |
| Network | 5-15ms | LAN latency (varies by network) |
| Decrypt | <1ms | - |
| Decode | 2-5ms | Hardware decoder (varies by GPU) |
| Display | 1-2ms | GPU texture upload |
| **Total** | **15-30ms** | End-to-end (example range) |

---

## Protocol Specification

### Packet Format

All packets follow this structure:

```
┌─────────────────────────────────────────────────────────────┐
│                    Packet Header (18 bytes)                 │
├───────────┬─────────┬──────┬───────┬────────┬──────────────┤
│   Magic   │ Version │ Type │ Flags │ Nonce  │ Payload Size │
│  4 bytes  │ 1 byte  │1 byte│2 bytes│8 bytes │   2 bytes    │
├───────────┴─────────┴──────┴───────┴────────┴──────────────┤
│                  Encrypted Payload (variable)               │
├─────────────────────────────────────────────────────────────┤
│                       MAC (16 bytes)                        │
└─────────────────────────────────────────────────────────────┘
```

### Header Fields

| Field | Size | Description |
|-------|------|-------------|
| Magic | 4 | `0x524F4F54` ("ROOT") |
| Version | 1 | Protocol version (1) |
| Type | 1 | Packet type (see below) |
| Flags | 2 | Reserved |
| Nonce | 8 | Encryption nonce (counter) |
| Payload Size | 2 | Encrypted payload length |

### Packet Types

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| PKT_HANDSHAKE | 0x01 | Both | Key exchange |
| PKT_VIDEO | 0x02 | Host→Client | Video frame |
| PKT_AUDIO | 0x03 | Host→Client | Audio frame |
| PKT_INPUT | 0x04 | Client→Host | Input events |
| PKT_CONTROL | 0x05 | Both | Control commands |
| PKT_PING | 0x06 | Both | Keepalive |
| PKT_PONG | 0x07 | Both | Keepalive response |

### Handshake Protocol

```
Client                              Host
   │                                  │
   │  PKT_HANDSHAKE                  │
   │  [pubkey_client][hostname]      │
   │────────────────────────────────▶│
   │                                  │
   │                                  │ Derive shared secret
   │                                  │
   │         PKT_HANDSHAKE           │
   │   [pubkey_host][hostname]       │
   │◀────────────────────────────────│
   │                                  │
   │ Derive shared secret            │
   │                                  │
   │     (All future packets         │
   │      are encrypted)             │
```

### Control Commands

```c
typedef struct __attribute__((packed)) {
    uint8_t cmd;       // Command type
    uint32_t value;    // Command parameter
} control_packet_t;  // 5 bytes
```

| Command | Value | Description |
|---------|-------|-------------|
| CTRL_PAUSE | 0x01 | Pause streaming |
| CTRL_RESUME | 0x02 | Resume streaming |
| CTRL_SET_BITRATE | 0x03 | Change bitrate (bps) |
| CTRL_SET_FPS | 0x04 | Change framerate |
| CTRL_REQUEST_KEYFRAME | 0x05 | Request I-frame |
| CTRL_SET_QUALITY | 0x06 | Set quality 0-100 |
| CTRL_DISCONNECT | 0x07 | Graceful disconnect |

### Input Event Format

```c
typedef struct __attribute__((packed)) {
    uint8_t type;      // EV_KEY, EV_REL, EV_ABS
    uint16_t code;     // Key/button code
    int32_t value;     // Value/delta
} input_event_pkt_t;  // 7 bytes
```

---

## Component Details

### Capture (drm_capture.c)

**Method:** Direct DRM/KMS framebuffer access

```c
// Open DRM device
fd = open("/dev/dri/card0", O_RDWR);

// Get framebuffer
drmModeGetFB(fd, fb_id);

// Map framebuffer memory
mmap(fb->handle, fb->size);
```

**Advantages:**
- Zero compositor latency
- Direct GPU memory access
- Works without X11/Wayland

**Fallback:** Memory-mapped framebuffer for legacy systems

### Encoding (vaapi_encoder.c)

**Pipeline:**
1. Upload RGBA to VA surface
2. Convert RGBA → NV12 (BT.709)
3. Configure H.264 parameters
4. Encode to NAL units
5. Parse NAL for keyframe detection

**Key Parameters:**
```c
seq_param.intra_period = fps;     // GOP size
seq_param.ip_period = 1;          // No B-frames
seq_param.bits_per_second = bitrate;
pic_param.pic_fields.bits.entropy_coding_mode_flag = 1;  // CABAC
```

### Decoding (vaapi_decoder.c)

**Pipeline:**
1. Parse NAL units
2. Submit to VA-API decoder
3. Wait for decoded surface
4. Present via SDL2 texture

### Cryptography (crypto.c)

**Algorithms:**
- Key exchange: X25519 (Curve25519 ECDH)
- Encryption: ChaCha20-Poly1305 (AEAD)
- Identity: Ed25519 signatures

**Session Establishment:**
```c
// Derive shared secret
shared_secret = X25519(my_secret, peer_public);

// Use for ChaCha20 key
crypto_aead_chacha20poly1305_ietf_encrypt(
    ciphertext, &ciphertext_len,
    plaintext, plaintext_len,
    NULL, 0,  // No additional data
    NULL,     // Nonce prefix
    nonce,    // 8-byte counter
    shared_secret
);
```

### Audio (opus_codec.c)

**Configuration:**
- Sample rate: 48000 Hz
- Channels: 2 (stereo)
- Frame size: 960 samples (20ms)
- Application: OPUS_APPLICATION_RESTRICTED_LOWDELAY

---

## Security Model

### Threat Model

RootStream protects against:
- **Eavesdropping**: All traffic encrypted
- **MITM**: Public key verification
- **Replay**: Nonce counter prevents reuse
- **Tampering**: Poly1305 MAC authentication

### Trust Establishment

1. Each device generates Ed25519 keypair
2. Public key shared via QR code or text
3. First connection verifies fingerprint
4. Subsequent connections auto-authenticate

### Key Storage

```
~/.config/rootstream/
├── keys/
│   ├── private.key    # Ed25519 secret key (32 bytes)
│   └── public.key     # Ed25519 public key (32 bytes)
└── config.ini
```

**Private key is never transmitted.**

---

## Adding New Encoders

To add a new encoder backend:

### 1. Create encoder file

```c
// src/myencoder.c

typedef struct {
    // Encoder-specific context
} myencoder_ctx_t;

int rootstream_encoder_init_myencoder(rootstream_ctx_t *ctx, codec_type_t codec) {
    // Initialize hardware
    // Allocate surfaces
    // Configure parameters
}

int rootstream_encode_frame_myencoder(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                      uint8_t *out, size_t *out_size) {
    // Upload frame
    // Encode
    // Detect keyframe
    // Copy output
}

void rootstream_encoder_cleanup_myencoder(rootstream_ctx_t *ctx) {
    // Free resources
}

bool rootstream_encoder_myencoder_available(void) {
    // Check if hardware is available
}
```

### 2. Add to routing

In `vaapi_encoder.c`:

```c
int rootstream_encoder_init(rootstream_ctx_t *ctx, encoder_type_t type, codec_type_t codec) {
    if (type == ENCODER_MYENCODER) {
        return rootstream_encoder_init_myencoder(ctx, codec);
    }
    // ...existing code...
}
```

### 3. Update Makefile

```makefile
SRCS += src/myencoder.c
```

### 4. Add detection

```makefile
# In Makefile, add hardware detection
MYENC_FOUND := $(shell ...)
ifeq ($(MYENC_FOUND),yes)
    CFLAGS += -DHAVE_MYENCODER
endif
```

---

## Performance Considerations

### Memory

| Component | Allocation | Notes |
|-----------|------------|-------|
| Frame buffer | width × height × 4 | RGBA pixels |
| Encode buffer | width × height | Worst case |
| VA surfaces | 4 × NV12 size | Ring buffer |
| Audio buffer | 960 × 2 × 2 | 20ms stereo |

### CPU Usage

| Stage | Usage | Optimization |
|-------|-------|--------------|
| Colorspace | 5-10% | SIMD (future) |
| Encryption | 1-2% | ChaCha20 is efficient |
| Encode/Decode | <1% | Hardware offload |

### GPU Usage

| Stage | GPU Load |
|-------|----------|
| Capture | 1-2% |
| Encode | 10-30% |
| Decode | 5-15% |
| Display | 1-2% |

### Network

| Parameter | Value | Notes |
|-----------|-------|-------|
| Bitrate | 5-50 Mbps | Configurable |
| Packet size | <1400 bytes | MTU safe |
| Framerate | 30-144 fps | Configurable |

### Latency Optimization

1. **Use CBR**: Constant bitrate for predictable latency
2. **No B-frames**: I and P frames only
3. **Low GOP**: Keyframe every 1 second
4. **Direct display**: Bypass compositor
5. **Hardware acceleration**: Offload to GPU

---

## File Structure

```
src/
├── main.c           # Entry point, argument parsing
├── drm_capture.c    # DRM/KMS screen capture
├── vaapi_encoder.c  # VA-API H.264/H.265 encoding
├── vaapi_decoder.c  # VA-API decoding
├── nvenc_encoder.c  # NVIDIA NVENC encoding
├── display_sdl2.c   # SDL2 video display
├── network.c        # UDP networking, protocol
├── crypto.c         # ChaCha20-Poly1305, X25519
├── discovery.c      # mDNS/Avahi service discovery
├── opus_codec.c     # Opus audio encode/decode
├── audio_capture.c  # ALSA audio capture
├── audio_playback.c # ALSA audio playback
├── input.c          # uinput virtual devices
├── service.c        # Host/client service loops
├── tray.c           # GTK3 system tray
├── qrcode.c         # QR code generation
├── config.c         # INI configuration
├── recording.c      # Stream recording
└── latency.c        # Performance instrumentation
```

---

*RootStream Architecture Documentation - Version 1.0*
