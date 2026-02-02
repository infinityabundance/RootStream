# RootStream API Reference

Complete API documentation for RootStream's C library.

## Table of Contents

1. [Overview](#overview)
2. [Data Types](#data-types)
3. [Initialization](#initialization)
4. [Cryptography](#cryptography)
5. [Capture](#capture)
6. [Encoding](#encoding)
7. [Decoding](#decoding)
8. [Network](#network)
9. [Audio](#audio)
10. [Input](#input)
11. [Recording](#recording)
12. [Utilities](#utilities)

---

## Overview

Include the main header in your application:

```c
#include <rootstream.h>
```

Link with: `-lrootstream` (or compile source files directly)

### Thread Safety

- Most functions are **not thread-safe**
- Use a single `rootstream_ctx_t` per thread
- Network operations use internal locking

---

## Data Types

### rootstream_ctx_t

Main application context containing all state.

```c
typedef struct {
    keypair_t keypair;           // Device identity
    settings_t settings;         // Configuration
    display_info_t display;      // Capture display info
    frame_buffer_t current_frame;// Current video frame
    encoder_ctx_t encoder;       // Encoder state
    peer_t peers[MAX_PEERS];     // Connected peers
    int num_peers;               // Peer count
    recording_ctx_t recording;   // Recording state
    latency_stats_t latency;     // Performance stats
    bool running;                // Main loop flag
} rootstream_ctx_t;
```

### frame_buffer_t

Video frame data.

```c
typedef struct {
    uint8_t *data;      // Pixel data (RGBA or encoded)
    uint32_t size;      // Data size in bytes
    uint32_t width;     // Frame width
    uint32_t height;    // Frame height
    uint32_t pitch;     // Bytes per row
    uint32_t format;    // Pixel format (DRM fourcc)
    uint64_t timestamp; // Capture time (microseconds)
    bool is_keyframe;   // True for I-frames
} frame_buffer_t;
```

### peer_t

Connected peer information.

```c
typedef struct {
    char rootstream_code[128];              // Peer's code
    uint8_t public_key[32];                 // Peer's public key
    struct sockaddr_storage addr;           // Network address
    crypto_session_t session;               // Encryption session
    peer_state_t state;                     // Connection state
    uint64_t last_seen;                     // Last packet time
    char hostname[64];                      // Peer hostname
    bool is_streaming;                      // Stream active?
} peer_t;
```

### encoder_ctx_t

Video encoder configuration.

```c
typedef struct {
    encoder_type_t type;    // ENCODER_VAAPI, ENCODER_NVENC
    codec_type_t codec;     // CODEC_H264, CODEC_H265
    int device_fd;          // Encoder device
    void *hw_ctx;           // Hardware context
    uint32_t bitrate;       // Target bitrate (bps)
    uint32_t framerate;     // Target FPS
    uint8_t quality;        // Quality level 0-100
    bool low_latency;       // Low-latency mode
    bool force_keyframe;    // Force next keyframe
} encoder_ctx_t;
```

---

## Initialization

### rootstream_init

Initialize the RootStream context.

```c
int rootstream_init(rootstream_ctx_t *ctx);
```

**Parameters:**
- `ctx`: Pointer to context structure (caller allocated)

**Returns:** 0 on success, -1 on error

**Example:**
```c
rootstream_ctx_t ctx = {0};
if (rootstream_init(&ctx) < 0) {
    fprintf(stderr, "Failed to initialize\n");
    return 1;
}
```

### rootstream_cleanup

Clean up and release all resources.

```c
void rootstream_cleanup(rootstream_ctx_t *ctx);
```

**Parameters:**
- `ctx`: Initialized context

---

## Cryptography

### crypto_init

Initialize the cryptographic subsystem.

```c
int crypto_init(void);
```

**Returns:** 0 on success, -1 on error

**Note:** Call once at program startup before other crypto functions.

### crypto_generate_keypair

Generate a new Ed25519 keypair.

```c
int crypto_generate_keypair(keypair_t *kp, const char *hostname);
```

**Parameters:**
- `kp`: Output keypair structure
- `hostname`: Device hostname for identity

**Returns:** 0 on success, -1 on error

### crypto_load_keypair / crypto_save_keypair

Load or save keypair to disk.

```c
int crypto_load_keypair(keypair_t *kp, const char *config_dir);
int crypto_save_keypair(const keypair_t *kp, const char *config_dir);
```

**Parameters:**
- `kp`: Keypair structure
- `config_dir`: Configuration directory path

### crypto_create_session

Create encrypted session with a peer.

```c
int crypto_create_session(crypto_session_t *session,
                          const uint8_t *my_secret,
                          const uint8_t *peer_public);
```

**Parameters:**
- `session`: Output session structure
- `my_secret`: Our 32-byte private key
- `peer_public`: Peer's 32-byte public key

**Returns:** 0 on success, -1 on error

### crypto_encrypt_packet / crypto_decrypt_packet

Encrypt or decrypt a packet.

```c
int crypto_encrypt_packet(const crypto_session_t *session,
                         const void *plaintext, size_t plain_len,
                         void *ciphertext, size_t *cipher_len,
                         uint64_t nonce);

int crypto_decrypt_packet(const crypto_session_t *session,
                         const void *ciphertext, size_t cipher_len,
                         void *plaintext, size_t *plain_len,
                         uint64_t nonce);
```

**Parameters:**
- `session`: Established session
- `plaintext/ciphertext`: Data buffers
- `*_len`: Data lengths
- `nonce`: Unique nonce (must never repeat)

**Returns:** 0 on success, -1 on error

### crypto_format_fingerprint

Format a public key as a short fingerprint.

```c
int crypto_format_fingerprint(const uint8_t *public_key, size_t key_len,
                              char *output, size_t output_len);
```

---

## Capture

### rootstream_detect_displays

Detect available displays.

```c
int rootstream_detect_displays(display_info_t *displays, int max_displays);
```

**Returns:** Number of displays found

### rootstream_select_display

Select a display for capture.

```c
int rootstream_select_display(rootstream_ctx_t *ctx, int display_index);
```

### rootstream_capture_init

Initialize capture subsystem.

```c
int rootstream_capture_init(rootstream_ctx_t *ctx);
```

### rootstream_capture_frame

Capture a single frame.

```c
int rootstream_capture_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame);
```

**Parameters:**
- `ctx`: Context with display selected
- `frame`: Output frame buffer

**Returns:** 0 on success, -1 on error

### rootstream_capture_cleanup

Clean up capture resources.

```c
void rootstream_capture_cleanup(rootstream_ctx_t *ctx);
```

---

## Encoding

### rootstream_encoder_init

Initialize hardware encoder.

```c
int rootstream_encoder_init(rootstream_ctx_t *ctx,
                           encoder_type_t type,
                           codec_type_t codec);
```

**Parameters:**
- `ctx`: Context
- `type`: `ENCODER_VAAPI` or `ENCODER_NVENC`
- `codec`: `CODEC_H264` or `CODEC_H265`

### rootstream_encode_frame

Encode a captured frame.

```c
int rootstream_encode_frame(rootstream_ctx_t *ctx, frame_buffer_t *in,
                           uint8_t *out, size_t *out_size);
```

**Parameters:**
- `ctx`: Initialized encoder context
- `in`: Input RGBA frame
- `out`: Output buffer for encoded data
- `out_size`: Output encoded size

### rootstream_encode_frame_ex

Extended encode with keyframe detection.

```c
int rootstream_encode_frame_ex(rootstream_ctx_t *ctx, frame_buffer_t *in,
                              uint8_t *out, size_t *out_size,
                              bool *is_keyframe);
```

**Additional Parameter:**
- `is_keyframe`: Output: true if encoded as keyframe

### rootstream_encoder_cleanup

Clean up encoder.

```c
void rootstream_encoder_cleanup(rootstream_ctx_t *ctx);
```

---

## Decoding

### rootstream_decoder_init

Initialize hardware decoder.

```c
int rootstream_decoder_init(rootstream_ctx_t *ctx);
```

### rootstream_decode_frame

Decode an encoded frame.

```c
int rootstream_decode_frame(rootstream_ctx_t *ctx,
                           const uint8_t *in, size_t in_size,
                           frame_buffer_t *out);
```

### rootstream_decoder_cleanup

Clean up decoder.

```c
void rootstream_decoder_cleanup(rootstream_ctx_t *ctx);
```

---

## Network

### rootstream_net_init

Initialize networking.

```c
int rootstream_net_init(rootstream_ctx_t *ctx, uint16_t port);
```

### rootstream_net_send_encrypted

Send encrypted packet to peer.

```c
int rootstream_net_send_encrypted(rootstream_ctx_t *ctx, peer_t *peer,
                                  uint8_t type, const void *data,
                                  size_t size);
```

**Packet Types:**
- `PKT_VIDEO` (0x02): Video frame
- `PKT_AUDIO` (0x03): Audio frame
- `PKT_INPUT` (0x04): Input events
- `PKT_CONTROL` (0x05): Control commands

### rootstream_net_recv

Receive and process packets.

```c
int rootstream_net_recv(rootstream_ctx_t *ctx, int timeout_ms);
```

**Parameters:**
- `timeout_ms`: Poll timeout (0 = non-blocking)

### rootstream_connect_to_peer

Connect to a peer by RootStream code.

```c
int rootstream_connect_to_peer(rootstream_ctx_t *ctx, const char *code);
```

### Control Commands

```c
int rootstream_send_control(rootstream_ctx_t *ctx, peer_t *peer,
                           control_cmd_t cmd, uint32_t value);

int rootstream_pause_stream(rootstream_ctx_t *ctx, peer_t *peer);
int rootstream_resume_stream(rootstream_ctx_t *ctx, peer_t *peer);
int rootstream_request_keyframe(rootstream_ctx_t *ctx, peer_t *peer);
```

---

## Audio

### Opus Codec

```c
int rootstream_opus_encoder_init(rootstream_ctx_t *ctx);
int rootstream_opus_decoder_init(rootstream_ctx_t *ctx);

int rootstream_opus_encode(rootstream_ctx_t *ctx, const int16_t *pcm,
                          uint8_t *out, size_t *out_len);

int rootstream_opus_decode(rootstream_ctx_t *ctx, const uint8_t *in,
                          size_t in_len, int16_t *pcm, size_t *pcm_len);

void rootstream_opus_cleanup(rootstream_ctx_t *ctx);

int rootstream_opus_get_frame_size(void);   // Returns 960
int rootstream_opus_get_sample_rate(void);  // Returns 48000
int rootstream_opus_get_channels(void);     // Returns 2
```

### Audio Capture (ALSA)

```c
int audio_capture_init(rootstream_ctx_t *ctx);
int audio_capture_frame(rootstream_ctx_t *ctx, int16_t *samples,
                       size_t *num_samples);
void audio_capture_cleanup(rootstream_ctx_t *ctx);
```

### Audio Playback (ALSA)

```c
int audio_playback_init(rootstream_ctx_t *ctx);
int audio_playback_write(rootstream_ctx_t *ctx, int16_t *samples,
                        size_t num_samples);
void audio_playback_cleanup(rootstream_ctx_t *ctx);
```

---

## Input

### rootstream_input_init

Initialize virtual input devices.

```c
int rootstream_input_init(rootstream_ctx_t *ctx);
```

### rootstream_input_process

Process received input event.

```c
int rootstream_input_process(rootstream_ctx_t *ctx, input_event_pkt_t *event);
```

### rootstream_input_cleanup

Clean up input devices.

```c
void rootstream_input_cleanup(rootstream_ctx_t *ctx);
```

---

## Recording

### recording_init

Start recording to file.

```c
int recording_init(rootstream_ctx_t *ctx, const char *filename);
```

### recording_write_frame

Write encoded frame to recording.

```c
int recording_write_frame(rootstream_ctx_t *ctx, const uint8_t *data,
                          size_t size, bool is_keyframe);
```

### recording_cleanup

Close recording file.

```c
void recording_cleanup(rootstream_ctx_t *ctx);
```

---

## Utilities

### Timestamps

```c
uint64_t get_timestamp_ms(void);  // Milliseconds (monotonic)
uint64_t get_timestamp_us(void);  // Microseconds (monotonic)
```

### Latency Instrumentation

```c
int latency_init(latency_stats_t *stats, size_t capacity,
                 uint64_t report_interval_ms, bool enabled);
void latency_cleanup(latency_stats_t *stats);
void latency_record(latency_stats_t *stats, const latency_sample_t *sample);
```

### Configuration

```c
const char* config_get_dir(void);
int config_load(rootstream_ctx_t *ctx);
int config_save(rootstream_ctx_t *ctx);
void config_add_peer_to_history(rootstream_ctx_t *ctx, const char *code);
```

### Error Handling

```c
const char* rootstream_get_error(void);  // Get last error message
void rootstream_print_stats(rootstream_ctx_t *ctx);  // Print statistics
```

---

## Example: Minimal Host

```c
#include <rootstream.h>

int main(void) {
    rootstream_ctx_t ctx = {0};

    // Initialize
    crypto_init();
    rootstream_init(&ctx);
    rootstream_net_init(&ctx, 9876);

    // Setup capture and encoding
    rootstream_capture_init(&ctx);
    rootstream_encoder_init(&ctx, ENCODER_VAAPI, CODEC_H264);

    // Allocate encode buffer
    uint8_t *enc_buf = malloc(ctx.display.width * ctx.display.height);

    // Main loop
    ctx.running = true;
    while (ctx.running) {
        // Capture
        rootstream_capture_frame(&ctx, &ctx.current_frame);

        // Encode
        size_t enc_size = 0;
        rootstream_encode_frame(&ctx, &ctx.current_frame, enc_buf, &enc_size);

        // Send to peers
        for (int i = 0; i < ctx.num_peers; i++) {
            if (ctx.peers[i].is_streaming) {
                rootstream_net_send_encrypted(&ctx, &ctx.peers[i],
                                             PKT_VIDEO, enc_buf, enc_size);
            }
        }

        // Process incoming
        rootstream_net_recv(&ctx, 1);
    }

    // Cleanup
    free(enc_buf);
    rootstream_encoder_cleanup(&ctx);
    rootstream_capture_cleanup(&ctx);
    rootstream_cleanup(&ctx);

    return 0;
}
```

---

*RootStream API Reference - Version 1.0*
