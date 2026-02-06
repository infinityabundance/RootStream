# RootStream Protocol (v1)

This document describes the on‑wire protocol used by RootStream for peer discovery, handshakes, and encrypted media/control transport.

## Overview

RootStream uses UDP with a small plaintext header and an encrypted payload. The encryption primitive is ChaCha20‑Poly1305 (IETF variant) and key agreement is X25519 derived from Ed25519 identity keys.

Key properties:
- UDP for low latency and drop‑tolerance
- Small plaintext header for routing and replay protection
- Authenticated encryption for all payloads after handshake

## Packet Header (plaintext)

All packets begin with a fixed header:

```
struct packet_header_t {
  uint32_t magic;         // 0x524F4F54 ("ROOT")
  uint8_t  version;       // 1
  uint8_t  type;          // PKT_*
  uint16_t flags;         // reserved (0)
  uint64_t nonce;         // per-peer increasing nonce
  uint16_t payload_size;  // encrypted payload size
  uint8_t  mac[16];        // Poly1305 tag (from ciphertext)
}
```

Notes:
- `payload_size` is the size of the encrypted payload (ciphertext).
- `mac` is produced by ChaCha20‑Poly1305 and validates ciphertext integrity.

## Packet Types

```
PKT_HANDSHAKE = 0x01
PKT_VIDEO     = 0x02
PKT_AUDIO     = 0x03
PKT_INPUT     = 0x04
PKT_CONTROL   = 0x05
PKT_PING      = 0x06
PKT_PONG      = 0x07
```

## Handshake

Handshake is unencrypted and establishes a shared session key.

Payload format:
```
[32 bytes] Ed25519 public key
[N bytes]  hostname (null‑terminated)
```

Flow:
1. Client sends PKT_HANDSHAKE with its public key + hostname.
2. Server derives X25519 shared secret from its Ed25519 private key and the client’s public key.
3. Server responds with its own PKT_HANDSHAKE.
4. Client derives the same shared secret.
5. Both sides set `session.authenticated = true` and start encrypted traffic.

## Encryption

For all non‑handshake packets:
- Payload is encrypted with ChaCha20‑Poly1305 (IETF).
- Nonce is a per‑peer monotonically increasing 64‑bit counter stored in the header.

## Video Payload (PKT_VIDEO)

Video frames are fragmented into UDP‑sized chunks. The encrypted payload begins with a chunk header:

```
struct video_chunk_header_t {
  uint32_t frame_id;
  uint32_t total_size;
  uint32_t offset;
  uint16_t chunk_size;
  uint16_t flags;         // reserved
  uint64_t timestamp_us;  // capture timestamp
}
[chunk_size bytes] encoded video data
```

Reassembly:
- Client reassembles chunks by `frame_id`.
- Once `received >= total_size`, the full frame is passed to the decoder.

## Audio Payload (PKT_AUDIO)

Audio packets carry Opus data preceded by a small header:

```
struct audio_packet_header_t {
  uint64_t timestamp_us;
  uint32_t sample_rate;
  uint16_t channels;
  uint16_t samples;       // per channel
}
[N bytes] Opus packet
```

The client can use `timestamp_us` for A/V sync.

## Input Payload (PKT_INPUT)

Input events are serialized as:

```
struct input_event_pkt_t {
  uint8_t  type;   // EV_KEY, EV_REL, etc.
  uint16_t code;   // key/button code
  int32_t  value;  // value/delta
}
```

## Control Payload (PKT_CONTROL)

Control packets carry a command and value:

```
struct control_packet_t {
  uint8_t  cmd;    // CTRL_*
  uint32_t value;  // command-specific
}
```

Commands:
```
CTRL_PAUSE            0x01
CTRL_RESUME           0x02
CTRL_SET_BITRATE      0x03
CTRL_SET_FPS          0x04
CTRL_REQUEST_KEYFRAME 0x05
CTRL_SET_QUALITY      0x06
CTRL_DISCONNECT       0x07
```

## Keepalive

- `PKT_PING` is sent periodically when connected.
- `PKT_PONG` is a response to `PKT_PING`.

## Versioning

Protocol version is in the header. Compatibility rules:
- Major version must match.
- New fields should be appended to payloads with safe defaults.
- Unknown packet types should be ignored.

## Limits

- Packet size targets MTU‑safe UDP payloads.
- Maximum reassembled video frame size is bounded in code.

## Reference

See:
- `include/rootstream.h` for struct definitions and constants.
- `src/network.c` for handshake, encryption, fragmentation, and reassembly.
