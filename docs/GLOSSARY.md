# RootStream Glossary

This glossary defines the canonical terms used across RootStream documentation.
Consistent terminology reduces ambiguity and makes the codebase easier to
navigate for new contributors.

If you encounter a term used inconsistently, file an issue referencing this file.

---

## A

**adaptive bitrate (ABR)**  
A control system that adjusts the encoded bitrate based on network conditions.
In RootStream, this is implemented in `src/abr/` and `src/network/adaptive_bitrate.c`.

**audio backend**  
An implementation of the audio capture or playback interface
(`audio_capture_backend_t` / `audio_playback_backend_t`). Available backends:
ALSA (primary), PulseAudio, PipeWire, Dummy (silent).

---

## B

**backend**  
A concrete implementation of a capability interface (capture, encode, audio, display,
discovery, input). Backends are selected at runtime via a fallback chain and stored
as vtable pointers in `rootstream_ctx_t`.

**backend verbose** (`--backend-verbose`)  
A CLI flag that causes RootStream to print which backend was selected at each
tier and what fallback attempts were made.

**bitrate**  
The number of bits transmitted per second for the encoded video stream.
Configurable via `--bitrate KBPS`.

---

## C

**capture backend**  
An implementation of the display capture interface (`capture_backend_t`).
Available backends: DRM/KMS (primary), X11 SHM (fallback), Dummy test pattern.

**ChaCha20-Poly1305**  
The authenticated encryption cipher used to protect all stream packets.
Provided by libsodium. Never implemented from scratch.

**client**  
The peer that receives the stream. Invoked with `rootstream connect <code>`.
Also referred to as "peer" in some contexts.

**codec**  
The algorithm used to compress video frames. RootStream supports H.264 (primary)
and H.265 (where supported). Audio uses the Opus codec.

---

## D

**DRM/KMS**  
Direct Rendering Manager / Kernel Mode Setting. The Linux kernel subsystem
used for display capture. RootStream reads framebuffers directly via
`/dev/dri/card0` without involving the compositor.

**discovery**  
The mechanism by which peers find each other. Tiers: mDNS/Avahi (primary),
UDP broadcast (fallback), manual peer entry (ultimate fallback).

**dummy backend**  
A no-op backend that produces silence (audio) or a test pattern (capture).
Used as the ultimate fallback and for testing.

---

## E

**Ed25519**  
The elliptic-curve signature algorithm used for device identity.
Each device generates a keypair on first run. The public key is shared
via the peer code; the private key never leaves the device.

**encoder backend**  
An implementation of the video encode interface (`encoder_backend_t`).
Available backends: NVENC (CUDA), VA-API, FFmpeg software, Raw (passthrough).

---

## F

**fallback chain**  
The ordered list of backends tried in sequence. If the primary backend fails,
the next one in the chain is tried. The dummy backend is always the final tier.

**FEC (Forward Error Correction)**  
A technique for recovering from packet loss by sending redundant data.
Implemented in `src/fec/`.

**fingerprint**  
A short human-readable representation of a public key, used to visually
verify peer identity. Format: `xxxx-xxxx-xxxx-xxxx`.

---

## H

**handshake**  
The initial exchange of messages between host and peer that establishes
mutual authentication and derives the session key. Implemented in
`src/session_hs/`.

**headless mode** (`HEADLESS=1`)  
A build flag that disables the GTK3 system tray. Produces a binary with
CLI-only interaction. Required for server and CI builds.

**host**  
The machine that captures and streams its display. Invoked with `rootstream host`.

---

## I

**IDR frame** (also: keyframe, I-frame)  
An independently decodable video frame that does not reference prior frames.
Required at stream start and after connection interruptions.

---

## K

**keypair**  
An Ed25519 public/private key pair representing a device's identity.
Stored in `~/.config/rootstream/` as `identity.key` (private, mode 600)
and `identity.pub` (public).

**keyframe**  
See IDR frame.

---

## L

**latency**  
The delay from screen pixel change on the host to the same pixel appearing
on the peer's display. Measured in milliseconds. Target for the supported
path: see `benchmarks/README.md`.

---

## M

**mDNS**  
Multicast DNS, used for local network peer discovery without a central server.
Implemented via Avahi on Linux.

---

## N

**nonce**  
A per-packet number that prevents replay attacks. Monotonically increasing
per session. Used as the ChaCha20-Poly1305 IV.

**NV12**  
A YUV 4:2:0 pixel format with a Y plane followed by an interleaved UV plane.
The primary format for hardware-decoded video frames.

---

## O

**Opus**  
The audio codec used for low-latency audio streaming. Target frame size:
2.5–5ms.

---

## P

**peer**  
Either machine in a RootStream session. More specifically, the connecting
client (receiving the stream). See also: host.

**peer code**  
A human-readable string encoding the host's public key and hostname.
Format: `base64(pubkey)@hostname`. Shared via `--qr` or printed to stdout.

**PipeWire**  
A Linux multimedia framework. RootStream includes a PipeWire audio backend
(`src/audio_capture_pipewire.c`, `src/audio_playback_pipewire.c`) that is
used when available.

**PLC (Packet Loss Concealment)**  
A technique to mask brief audio interruptions caused by packet loss.
Implemented in `src/plc/`.

---

## Q

**QR code**  
A machine-readable visual encoding of the peer code. Generated by `--qr`
using the `libqrencode` library.

---

## R

**RGBA**  
A 32-bit pixel format with red, green, blue, and alpha channels.
Used by the X11 capture and dummy backends.

**relay / TURN**  
An intermediate server that relays packets between peers that cannot
connect directly. Implemented in `src/relay/`. Not part of the supported
LAN path.

---

## S

**session**  
An encrypted streaming connection between host and peer. Each session
has a unique derived key and nonce space.

**session key**  
A symmetric key derived from the X25519 ECDH exchange of the two device
keypairs. Used for ChaCha20-Poly1305 encryption of all packets.

**supported path**  
The canonical RootStream product path: Linux host → Linux peer on a LAN.
See `docs/PRODUCT_CORE.md`.

---

## T

**tray**  
The system notification area UI (GTK3). Provides a quick-access menu for
host/client operations. Disabled in headless builds.

---

## U

**uinput**  
A Linux kernel facility for creating virtual input devices. RootStream
uses uinput to inject keyboard and mouse events received from the peer.

---

## V

**VA-API**  
Video Acceleration API. The Linux hardware-accelerated video encode/decode
interface. RootStream uses VA-API for hardware encoding on Intel and AMD GPUs.

---

## W

**wire protocol**  
The binary message format exchanged between host and peer. Versioned via
`PROTOCOL_VERSION` in `include/rootstream.h`. See `docs/PROTOCOL.md`.

---

## X

**X25519**  
The elliptic Diffie-Hellman function used to derive the session shared
secret from the two devices' Ed25519 keypairs.

**XDG_CONFIG_HOME**  
The environment variable controlling where RootStream stores its config and
keys (default: `~/.config/rootstream/`). Used in testing to redirect to a
temporary directory.
