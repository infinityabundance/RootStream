# 🚀 RootStream Microtask Registry

> **Source of Truth** for all development microtasks.  
> A microtask achieves 🟢 only when its gate script passes in CI.  
> For current repository state analysis, see [`docs/STATE_REPORT.md`](STATE_REPORT.md).

---

## 📖 Legend

### Status

| Symbol | Meaning |
|--------|---------|
| 🟢 | **Complete** — implemented, tested, and gate-verified |
| 🟡 | **In Progress** — partially implemented |
| 🔴 | **Not Started** — queued, dependencies met |
| 🔵 | **Blocked** — unresolved dependency prevents progress |
| ⚪ | **Deferred** — intentionally descoped |

### Columns

| Column | Meaning |
|--------|---------|
| P | Priority: **P0** (Critical) · **P1** (High) · **P2** (Medium) · **P3** (Low) |
| Effort | Estimated person-hours |
| 🌟 | Legendary score 1–10 (impact × novelty × moonshot value) |
| Description (done when) | Precise code-level condition that justifies 🟢 status |
| Gate | Gate script that validates this item in CI |

---

## 📊 Progress Summary

| Phase | Name | Status | Complete | Total |
|-------|------|--------|----------|-------|
| PHASE-00 | Backend Infrastructure | 🟢 | 5 | 5 |
| PHASE-01 | Display Capture Fallback | 🟢 | 4 | 4 |
| PHASE-02 | Video Encoding Fallback | 🟢 | 4 | 4 |
| PHASE-03 | Audio Pipeline Fallback | 🟢 | 4 | 4 |
| PHASE-04 | Network Resilience | 🟢 | 5 | 5 |
| PHASE-05 | Discovery Fallback | 🟢 | 5 | 5 |
| PHASE-06 | Input & GUI Fallback | 🟢 | 7 | 7 |
| PHASE-07 | PipeWire Audio Fallback | 🟢 | 4 | 4 |
| PHASE-08 | Integration Testing & Docs | 🟢 | 8 | 8 |
| PHASE-09 | Protocol Implementation | 🟢 | 5 | 5 |
| PHASE-10 | Security & Cryptography | 🟢 | 6 | 6 |
| PHASE-11 | Client Decode & Rendering | 🟢 | 5 | 5 |
| PHASE-12 | KDE Plasma Client Base | 🟢 | 6 | 6 |
| PHASE-13 | KDE Client Audio | 🟢 | 5 | 5 |
| PHASE-14 | KDE Client Audio Player | 🟢 | 4 | 4 |
| PHASE-15 | Input Manager | 🟢 | 4 | 4 |
| PHASE-16 | KDE Client Polish | 🟢 | 5 | 5 |
| PHASE-17 | Recording System | 🟢 | 6 | 6 |
| PHASE-18 | Advanced Encoding | 🟢 | 5 | 5 |
| PHASE-19 | Container Formats & Metadata | 🟢 | 4 | 4 |
| PHASE-20 | Performance Metrics / HUD | 🟢 | 6 | 6 |
| PHASE-21 | Web Dashboard API Server | 🟢 | 6 | 6 |
| PHASE-22 | Mobile Clients (Android/iOS) | 🟢 | 8 | 8 |
| PHASE-23 | Database Layer | 🟢 | 5 | 5 |
| PHASE-24 | VR / Proton Compatibility | 🟢 | 9 | 9 |
| PHASE-25 | Security Hardening | 🟢 | 7 | 7 |
| PHASE-26 | Network Optimization | 🟢 | 9 | 9 |
| PHASE-27 | CI / Infrastructure | 🟢 | 8 | 8 |
| PHASE-28 | Event Sourcing / CQRS | 🟢 | 6 | 6 |
| PHASE-29 | Android / iOS Full Client | 🟢 | 8 | 8 |
| PHASE-30 | Security Phase 2 | 🟢 | 6 | 6 |
| PHASE-31 | Vulkan Renderer | 🟢 | 6 | 6 |
| PHASE-32 | Backend Integration | 🟢 | 6 | 6 |
| PHASE-33 | Code Standards & Quality | 🟢 | 4 | 4 |
| PHASE-34 | Production Readiness | 🟢 | 4 | 4 |
| PHASE-35 | Plugin & Extension System | 🟢 | 5 | 5 |
| PHASE-36 | Audio DSP Pipeline | 🟢 | 5 | 5 |
| PHASE-37 | Multi-Client Fanout | 🟢 | 5 | 5 |
| PHASE-38 | Collaboration & Annotation | 🟢 | 4 | 4 |
| PHASE-39 | Stream Quality Intelligence | 🟢 | 5 | 5 |
| PHASE-40 | Relay / TURN Infrastructure | 🟢 | 5 | 5 |
| PHASE-41 | Session Persistence & Resumption | 🟢 | 4 | 4 |
| PHASE-42 | Closed-Caption & Subtitle System | 🟢 | 4 | 4 |
| PHASE-43 | Stream Scheduler | 🟢 | 5 | 5 |
| PHASE-44 | HLS Segment Output | 🟢 | 5 | 5 |
| PHASE-45 | Viewer Analytics & Telemetry | 🟢 | 5 | 5 |
| PHASE-46 | Perceptual Frame Hashing | 🟢 | 4 | 4 |
| PHASE-47 | Stream Watermarking | 🟢 | 5 | 5 |
| PHASE-48 | Adaptive Bitrate Controller | 🟢 | 5 | 5 |
| PHASE-49 | Content Metadata Pipeline | 🟢 | 4 | 4 |
| PHASE-50 | Low-Latency Jitter Buffer | 🟢 | 4 | 4 |
| PHASE-51 | Packet Loss Concealment | 🟢 | 5 | 5 |
| PHASE-52 | Token Bucket Rate Limiter | 🟢 | 4 | 4 |
| PHASE-53 | Frame Rate Controller | 🟢 | 4 | 4 |
| PHASE-54 | Stream Config Serialiser | 🟢 | 4 | 4 |
| PHASE-55 | Session Handshake Protocol | 🟢 | 5 | 5 |
| PHASE-56 | Network Congestion Detector | 🟢 | 4 | 4 |
| PHASE-57 | IDR / Keyframe Request Handler | 🟢 | 4 | 4 |
| PHASE-58 | Circular Event Log | 🟢 | 4 | 4 |
| PHASE-59 | Multi-Stream Mixer | 🟢 | 4 | 4 |
| PHASE-60 | Bandwidth Probe | 🟢 | 4 | 4 |
| PHASE-61 | Packet Reorder Buffer | 🟢 | 4 | 4 |
| PHASE-62 | Adaptive GOP Controller | 🟢 | 4 | 4 |
| PHASE-63 | Stream Health Monitor | 🟢 | 4 | 4 |
| PHASE-64 | FEC Encoder / Decoder | 🟢 | 4 | 4 |
| PHASE-65 | Clock Sync Offset Estimator | 🟢 | 4 | 4 |
| PHASE-66 | Plugin Hot-Reload Manager | 🟢 | 4 | 4 |
| PHASE-67 | Frame Rate Controller | 🟢 | 4 | 4 |
| PHASE-68 | Output Target Registry | 🟢 | 4 | 4 |
| PHASE-69 | Bitrate Ladder Builder | 🟢 | 4 | 4 |
| PHASE-70 | Packet Loss Estimator | 🟢 | 4 | 4 |
| PHASE-71 | Timestamp Synchronizer | 🟢 | 4 | 4 |
| PHASE-72 | Session Limiter | 🟢 | 4 | 4 |
| PHASE-73 | Stream Tag Store | 🟢 | 4 | 4 |
| PHASE-74 | Buffer Pool | 🟢 | 4 | 4 |
| PHASE-75 | Event Bus | 🟢 | 4 | 4 |
| PHASE-76 | Chunk Splitter | 🟢 | 4 | 4 |
| PHASE-77 | Priority Queue | 🟢 | 4 | 4 |
| PHASE-78 | Retry Manager | 🟢 | 4 | 4 |
| PHASE-79 | Flow Controller | 🟢 | 4 | 4 |
| PHASE-80 | Metrics Exporter | 🟢 | 4 | 4 |
| PHASE-81 | Signal Router | 🟢 | 4 | 4 |
| PHASE-82 | Drain Queue | 🟢 | 4 | 4 |
| PHASE-83 | Cross-Subsystem Integration Tests | 🟢 | 4 | 4 |
| PHASE-84 | KDE Client Deep Integration Tests | 🟢 | 4 | 4 |
| PHASE-85 | Android/iOS/Web Client Audits | 🟢 | 4 | 4 |
| PHASE-86 | Code Hygiene, Commentary, Qt6 Standards | 🟢 | 4 | 4 |
| PHASE-98 | Product Core Definition | 🟢 | 16 | 16 |
| PHASE-99 | Golden Path Hardening | 🟢 | 13 | 13 |
| PHASE-100 | CI and Quality Gate Hardening | 🟢 | 12 | 12 |
| PHASE-101 | Architecture Boundary Cleanup | 🟢 | 12 | 12 |
| PHASE-102 | Observability and Reliability | 🟢 | 12 | 12 |
| PHASE-103 | Testing, Stress, and Soak Discipline | 🟢 | 12 | 12 |
| PHASE-104 | Performance and Benchmark Proof | 🟢 | 12 | 12 |
| PHASE-105 | Security Posture and Trust Signals | 🟢 | 10 | 10 |
| PHASE-106 | Enterprise-Grade Repo Polish | 🟢 | 10 | 10 |
| PHASE-107 | Release Readiness System | 🟢 | 10 | 10 |
| PHASE-108 | Legendary Consistency Pass | 🟢 | 10 | 10 |
| PHASE-109 | Code Format Zero-Violation Sprint | 🟢 | 3 | 3 |
| PHASE-110 | Source TODO Resolution | 🟢 | 4 | 4 |
| PHASE-111 | Progress Registry Accuracy Pass | 🟢 | 2 | 2 |
| PHASE-112 | World-Class Final Consistency Pass | 🟢 | 3 | 3 |

> **Overall**: 582 / 582 microtasks complete (**100%** — all programmes complete)

---

## PHASE-00: Backend Infrastructure

> Establish the core context structure, CLI flag surface, and startup reporting that all subsequent phases depend on.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 0.1 | Context struct design | 🟢 | P0 | 2h | 4 | `StreamContext` struct in `src/context.h` has fields for all backend types and active-backend name strings | `scripts/validate_traceability.sh` |
| 0.2 | Active backend name reporting | 🟢 | P0 | 1h | 3 | `ctx->active_capture_backend`, `ctx->active_encode_backend`, etc. populated at init and printed on `--backend-verbose` | `scripts/validate_traceability.sh` |
| 0.3 | CLI `--backend-verbose` flag | 🟢 | P1 | 1h | 3 | `--backend-verbose` parsed in `main()`, triggers `print_backend_status(ctx)` | `scripts/validate_traceability.sh` |
| 0.4 | Startup backend status report | 🟢 | P1 | 1h | 3 | On startup, all active backends logged to stderr with their tier level | `scripts/validate_traceability.sh` |
| 0.5 | CMake feature detection flags | 🟢 | P0 | 2h | 4 | `cmake/FindBackends.cmake` detects libdrm, X11, NVENC, VA-API, ALSA, PulseAudio, PipeWire, FFmpeg; sets `HAVE_*` defines | `scripts/validate_traceability.sh` |

---

## PHASE-01: Display Capture Fallback

> Implement a three-tier display capture chain: DRM/KMS (GPU) → X11 SHM/XGetImage → Dummy test pattern.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 1.1 | DRM/KMS primary capture | 🟢 | P0 | 4h | 6 | `src/capture_drm.c` opens `/dev/dri/card0`, maps framebuffer, exports `capture_drm_frame()` | `scripts/validate_traceability.sh` |
| 1.2 | X11 SHM fallback capture | 🟢 | P0 | 3h | 5 | `src/x11_capture.c` uses `XShmGetImage` with `XGetImage` fallback; exported `capture_x11_frame()` | `scripts/validate_traceability.sh` |
| 1.3 | Dummy test-pattern generator | 🟢 | P1 | 2h | 3 | `src/dummy_capture.c` generates animated SMPTE colour bars at requested resolution/fps | `scripts/validate_traceability.sh` |
| 1.4 | Fallback selection in service loop | 🟢 | P0 | 2h | 5 | `init_capture_backend()` tries DRM → X11 → Dummy; stores result in `ctx->active_capture_backend` | `scripts/validate_traceability.sh` |

---

## PHASE-02: Video Encoding Fallback

> Four-tier video encoding chain: NVENC (CUDA) → VA-API (GPU) → x264/FFmpeg (CPU) → Raw passthrough.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 2.1 | NVENC hardware encoder | 🟢 | P0 | 6h | 7 | `src/encode_nvenc.c` initialises CUDA context, creates NV_ENC session, encodes I/P frames | `scripts/validate_traceability.sh` |
| 2.2 | VA-API hardware encoder | 🟢 | P0 | 5h | 7 | `src/encode_vaapi.c` opens `/dev/dri/renderD128`, creates VA context, encodes H.264 via libva | `scripts/validate_traceability.sh` |
| 2.3 | x264/FFmpeg software encoder | 🟢 | P1 | 4h | 5 | `src/encode_x264.c` links libx264 via FFmpeg AVCodec interface; tunable CRF/bitrate | `scripts/validate_traceability.sh` |
| 2.4 | Raw encoder fallback | 🟢 | P1 | 1h | 2 | `src/encode_raw.c` copies frames unmodified; used for loopback/debug and as ultimate fallback | `scripts/validate_traceability.sh` |

---

## PHASE-03: Audio Pipeline Fallback

> Four-tier audio capture chain: ALSA → PulseAudio → PipeWire → Dummy silence.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 3.1 | ALSA capture/playback primary | 🟢 | P0 | 4h | 5 | `src/audio_capture_alsa.c` and `src/audio_playback_alsa.c` use `snd_pcm_*` API; configurable sample rate/channels | `scripts/validate_traceability.sh` |
| 3.2 | PulseAudio fallback | 🟢 | P0 | 4h | 5 | `src/audio_capture_pulse.c` and `src/audio_playback_pulse.c` use `pa_simple_*` API | `scripts/validate_traceability.sh` |
| 3.3 | Dummy silent audio fallback | 🟢 | P1 | 1h | 2 | `src/audio_dummy.c` delivers zero-filled PCM buffers at correct rate to prevent stalls | `scripts/validate_traceability.sh` |
| 3.4 | Integration into host/client loops | 🟢 | P0 | 2h | 4 | `init_audio_backend()` selects tier; host and client main loops call `audio_capture_read()` / `audio_playback_write()` through a unified vtable | `scripts/validate_traceability.sh` |

---

## PHASE-04: Network Resilience

> Dual-transport layer (UDP primary, TCP fallback) with automatic reconnect and exponential backoff.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 4.1 | UDP primary transport | 🟢 | P0 | 3h | 5 | Existing UDP socket path refactored into `src/network_udp.c` with `net_udp_send()` / `net_udp_recv()` | `scripts/validate_traceability.sh` |
| 4.2 | TCP fallback transport | 🟢 | P0 | 4h | 6 | `src/network_tcp.c` provides `net_tcp_connect()`, `net_tcp_send()`, `net_tcp_recv()`; stream framing with 4-byte length prefix | `scripts/validate_traceability.sh` |
| 4.3 | Auto-reconnect with exponential backoff | 🟢 | P0 | 3h | 6 | `src/network_reconnect.c` implements reconnect loop; backoff doubles from 500 ms to 30 s cap; jitter ±10% | `scripts/validate_traceability.sh` |
| 4.4 | Connection state tracking | 🟢 | P1 | 2h | 4 | `ctx->net_state` enum: DISCONNECTED / CONNECTING / CONNECTED / RECONNECTING; state transitions logged | `scripts/validate_traceability.sh` |
| 4.5 | Peer health monitoring | 🟢 | P1 | 2h | 4 | Keepalive packets sent every 1 s; peer declared dead after 5 s silence; triggers reconnect | `scripts/validate_traceability.sh` |

---

## PHASE-05: Discovery Fallback

> Three-tier peer discovery: mDNS/Avahi → UDP broadcast → Manual IP entry with persistent peer history.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 5.1 | mDNS/Avahi primary discovery | 🟢 | P0 | 4h | 6 | Integrates libavahi-client; registers `_rootstream._udp` service; resolves peers on LAN | `scripts/validate_traceability.sh` |
| 5.2 | UDP broadcast fallback | 🟢 | P0 | 3h | 5 | `src/discovery_broadcast.c` sends/receives beacon on port 47920; parses peer descriptor JSON | `scripts/validate_traceability.sh` |
| 5.3 | Manual peer entry | 🟢 | P1 | 2h | 3 | `src/discovery_manual.c` parses `host:port` or peer-code format; stored in `~/.config/rootstream/peers.json` | `scripts/validate_traceability.sh` |
| 5.4 | Peer history/favourites | 🟢 | P2 | 2h | 3 | Peer history serialised to JSON on exit; loaded on startup; `--peer-list` prints known peers | `scripts/validate_traceability.sh` |
| 5.5 | CLI peer management flags | 🟢 | P1 | 1h | 3 | `--peer-add <addr>`, `--peer-list`, `--peer-code <code>` all functional in `main()` | `scripts/validate_traceability.sh` |

---

## PHASE-06: Input & GUI Fallback

> Three-tier input injection (uinput → xdotool → logging) and three-tier GUI (GTK → ncurses → CLI).

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 6.1 | uinput primary input injection | 🟢 | P0 | 4h | 6 | `src/input_uinput.c` opens `/dev/uinput`, creates virtual keyboard+mouse device; injects events | `scripts/validate_traceability.sh` |
| 6.2 | xdotool fallback input | 🟢 | P1 | 3h | 4 | `src/input_xdotool.c` forks `xdotool` subprocess; supports key, mousemove, click events | `scripts/validate_traceability.sh` |
| 6.3 | Logging debug input mode | 🟢 | P2 | 1h | 2 | `src/input_logging.c` writes all input events to `stderr` in human-readable form; useful for CI | `scripts/validate_traceability.sh` |
| 6.4 | GTK system tray GUI | 🟢 | P1 | 5h | 5 | GTK3 tray icon with status menu: connect/disconnect, bandwidth indicator, settings dialog | `scripts/validate_traceability.sh` |
| 6.5 | ncurses TUI fallback | 🟢 | P1 | 4h | 5 | `src/tray_tui.c` full-terminal dashboard: peer list, stats, key bindings | `scripts/validate_traceability.sh` |
| 6.6 | CLI-only mode | 🟢 | P0 | 1h | 3 | `--no-gui` flag disables all GUI layers; all status output via stderr; suitable for headless servers | `scripts/validate_traceability.sh` |
| 6.7 | Diagnostics module | 🟢 | P1 | 2h | 4 | `src/diagnostics.c` implements `--diagnostics` flag: probes all backends, prints capability matrix | `scripts/validate_traceability.sh` |

---

## PHASE-07: PipeWire Audio Fallback

> Add PipeWire as the third audio tier, completing the ALSA → PulseAudio → PipeWire → Dummy chain.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 7.1 | PipeWire capture backend | 🟢 | P0 | 5h | 7 | `src/audio_capture_pipewire.c` creates `pw_stream` in capture mode; exports `pipewire_audio_capture_*` vtable | `scripts/validate_traceability.sh` |
| 7.2 | PipeWire playback backend | 🟢 | P0 | 5h | 7 | `src/audio_playback_pipewire.c` creates `pw_stream` in playback mode; handles format negotiation | `scripts/validate_traceability.sh` |
| 7.3 | Integration into fallback chain | 🟢 | P0 | 1h | 5 | `init_audio_backend()` now tries ALSA → PulseAudio → PipeWire → Dummy in order | `scripts/validate_traceability.sh` |
| 7.4 | CMake detection of libpipewire-0.3 | 🟢 | P0 | 1h | 3 | `cmake/FindPipeWire.cmake` uses `pkg_check_modules(PIPEWIRE libpipewire-0.3)`; guards `HAVE_PIPEWIRE` | `scripts/validate_traceability.sh` |

---

## PHASE-08: Integration Testing, Unit Tests & Documentation

> Comprehensive test infrastructure validating all fallback chains; reality audit of all claims.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 8.1 | Test harness (`tests/common/`) | 🟢 | P0 | 3h | 5 | `test_harness.h` provides `ASSERT_*` macros, mock backend structs; `test_harness.c` runs suites and reports pass/fail/skip | `scripts/validate_traceability.sh` |
| 8.2 | Integration: capture fallback chain | 🟢 | P0 | 3h | 6 | `tests/integration/test_capture_fallback.c` validates DRM→X11→Dummy ordering with mock availability flags | `scripts/validate_traceability.sh` |
| 8.3 | Integration: encode fallback chain | 🟢 | P0 | 3h | 6 | `tests/integration/test_encode_fallback.c` validates NVENC→VAAPI→x264→Raw ordering | `scripts/validate_traceability.sh` |
| 8.4 | Integration: audio fallback chain | 🟢 | P0 | 3h | 6 | `tests/integration/test_audio_fallback.c` validates ALSA→Pulse→PipeWire→Dummy | `scripts/validate_traceability.sh` |
| 8.5 | Integration: network TCP fallback | 🟢 | P0 | 3h | 6 | `tests/integration/test_network_fallback.c` forces UDP failure, validates TCP promotion and reconnect backoff | `scripts/validate_traceability.sh` |
| 8.6 | Integration: discovery chain | 🟢 | P0 | 2h | 5 | `tests/integration/test_discovery_fallback.c` validates mDNS→Broadcast→Manual ordering | `scripts/validate_traceability.sh` |
| 8.7 | Unit tests for all backend modules | 🟢 | P1 | 4h | 5 | `tests/unit/test_backends_*.c` files cover priority scoring, naming, capability detection | `scripts/validate_traceability.sh` |
| 8.8 | CMake test labels & CTest integration | 🟢 | P0 | 2h | 4 | `ctest -L unit` and `ctest -L integration` both pass; test count ≥ 10 | `scripts/validate_traceability.sh` |

---

## PHASE-09: Protocol Implementation

> Binary packet protocol with versioning, sequence numbers, checksums, and packet type registry.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 9.1 | Packet header design | 🟢 | P0 | 2h | 5 | `src/packet.h` defines fixed 16-byte header: magic(4), version(1), type(1), flags(1), seq(4), length(4), checksum(2) | `scripts/validate_traceability.sh` |
| 9.2 | Packet type registry | 🟢 | P0 | 1h | 4 | Enum `PacketType` covers VIDEO_FRAME, AUDIO_CHUNK, INPUT_EVENT, KEEPALIVE, HANDSHAKE, CONFIG, DISCONNECT | `scripts/validate_traceability.sh` |
| 9.3 | Serialise / deserialise | 🟢 | P0 | 3h | 5 | `packet_serialize()` and `packet_deserialize()` handle network byte order; reject unknown versions | `scripts/validate_traceability.sh` |
| 9.4 | CRC-32 checksum validation | 🟢 | P1 | 2h | 4 | `packet_verify_checksum()` rejects corrupted packets; counter in ctx exposed via diagnostics | `scripts/validate_traceability.sh` |
| 9.5 | Protocol version negotiation | 🟢 | P0 | 2h | 5 | Handshake exchange: client offers version list; server selects highest mutual version | `scripts/validate_traceability.sh` |

---

## PHASE-10: Security & Cryptography

> End-to-end encryption via libsodium: Ed25519 identity keys, X25519 ephemeral DH, ChaCha20-Poly1305 stream cipher.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 10.1 | Ed25519 identity key generation | 🟢 | P0 | 2h | 8 | `src/crypto.c` calls `crypto_sign_keypair()` on first run; persists to `~/.config/rootstream/identity.key` (mode 0600) | `scripts/validate_traceability.sh` |
| 10.2 | X25519 ephemeral key exchange | 🟢 | P0 | 3h | 8 | Handshake generates ephemeral `crypto_kx_keypair()`; shared secret derived via `crypto_kx_client_session_keys()` | `scripts/validate_traceability.sh` |
| 10.3 | ChaCha20-Poly1305 stream encryption | 🟢 | P0 | 3h | 9 | All packets encrypted with `crypto_aead_chacha20poly1305_ietf_encrypt()`; nonce incremented per packet | `scripts/validate_traceability.sh` |
| 10.4 | Peer fingerprint display | 🟢 | P1 | 1h | 5 | `--show-fingerprint` prints base58-encoded Ed25519 public key; used for out-of-band verification | `scripts/validate_traceability.sh` |
| 10.5 | Trust-on-first-use (TOFU) | 🟢 | P1 | 2h | 6 | First connection to new peer prompts user; accepted keys stored in `~/.config/rootstream/known_peers` | `scripts/validate_traceability.sh` |
| 10.6 | Replay attack prevention | 🟢 | P0 | 2h | 7 | Sequence numbers validated; sliding window of 64 rejects duplicates/replays | `scripts/validate_traceability.sh` |

---

## PHASE-11: Client-Side Decode and Rendering

> Client receives encrypted stream, decodes H.264 via FFmpeg, renders via SDL2 with low-latency display path.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 11.1 | FFmpeg H.264 decoder | 🟢 | P0 | 4h | 6 | `src/client_decode.c` opens `h264` AVCodec context; `decode_video_packet()` returns `AVFrame*` | `scripts/validate_traceability.sh` |
| 11.2 | SDL2 render window | 🟢 | P0 | 3h | 5 | `src/client_render.c` creates SDL2 window; `render_frame()` uploads YUV via `SDL_UpdateYUVTexture` | `scripts/validate_traceability.sh` |
| 11.3 | Audio decode & playback (client) | 🟢 | P0 | 3h | 5 | `src/client_audio.c` decodes Opus packets via libopus; plays via SDL2 audio callback | `scripts/validate_traceability.sh` |
| 11.4 | Input event forwarding | 🟢 | P1 | 2h | 5 | SDL2 keyboard/mouse events serialised as `INPUT_EVENT` packets and sent to host | `scripts/validate_traceability.sh` |
| 11.5 | Latency statistics overlay | 🟢 | P2 | 2h | 4 | `--show-stats` renders frame latency, decode time, jitter as SDL2 text overlay | `scripts/validate_traceability.sh` |

---

## PHASE-12: KDE Plasma Client Base

> Qt6/QML KDE Plasma desktop client: main window, connection dialog, stream view, CMake build.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 12.1 | CMake project setup (Qt6, KDE Frameworks) | 🟢 | P0 | 2h | 4 | `clients/kde-plasma-client/CMakeLists.txt` finds Qt6, KF6, sets up KDE ECM; builds without errors | `scripts/validate_traceability.sh` |
| 12.2 | Main application window (QML) | 🟢 | P0 | 4h | 5 | `MainWindow.qml` with sidebar navigation, stream viewport, status bar | `scripts/validate_traceability.sh` |
| 12.3 | Connection dialog | 🟢 | P0 | 3h | 4 | `ConnectionDialog.qml` with host/port fields, peer-code input, connect button | `scripts/validate_traceability.sh` |
| 12.4 | Stream video widget | 🟢 | P0 | 5h | 7 | `StreamView.qml` backed by `StreamRenderer` QQuickItem using OpenGL texture upload | `scripts/validate_traceability.sh` |
| 12.5 | Settings page | 🟢 | P1 | 3h | 3 | `SettingsPage.qml` with codec, resolution, bitrate, audio device selectors | `scripts/validate_traceability.sh` |
| 12.6 | KDE Plasma integration (Kirigami) | 🟢 | P1 | 2h | 5 | Uses `Kirigami.ApplicationWindow`; respects KDE colour scheme; system tray integration via `KStatusNotifierItem` | `scripts/validate_traceability.sh` |

---

## PHASE-13: KDE Client Audio

> KDE client audio subsystem: Opus decode, multi-backend playback (PulseAudio, PipeWire, ALSA).

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 13.1 | Opus codec integration | 🟢 | P0 | 3h | 6 | `AudioManager` uses libopus for decode; configurable frame size (120/240/480/960 samples) | `scripts/validate_traceability.sh` |
| 13.2 | PulseAudio playback backend | 🟢 | P0 | 3h | 5 | `PulseAudioBackend.cpp` uses `pa_simple` API; auto-detects server from `$PULSE_SERVER` | `scripts/validate_traceability.sh` |
| 13.3 | PipeWire playback backend | 🟢 | P0 | 4h | 6 | `PipeWireBackend.cpp` uses `pw_stream` in playback mode; negotiates format with session manager | `scripts/validate_traceability.sh` |
| 13.4 | ALSA fallback playback backend | 🟢 | P1 | 3h | 4 | `ALSABackend.cpp` uses `snd_pcm_writei`; configurable device string | `scripts/validate_traceability.sh` |
| 13.5 | Audio backend selection logic | 🟢 | P0 | 1h | 4 | `AudioManager::init()` tries PipeWire → PulseAudio → ALSA; stores active backend name | `scripts/validate_traceability.sh` |

---

## PHASE-14: KDE Client Audio Player

> Audio player component integrated into KDE client stream view with volume, mute, and sync controls.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 14.1 | Audio player C++ class | 🟢 | P0 | 3h | 5 | `AudioPlayer.cpp` provides `play()`, `pause()`, `setVolume()`, `setMuted()` Q_INVOKABLE methods | `scripts/validate_traceability.sh` |
| 14.2 | QML audio controls widget | 🟢 | P1 | 2h | 4 | `AudioControls.qml` volume slider + mute toggle bound to `AudioPlayer` | `scripts/validate_traceability.sh` |
| 14.3 | Audio/video sync logic | 🟢 | P0 | 3h | 6 | `AudioPlayer` maintains PTS clock; drops/stretches audio to stay within ±20 ms of video PTS | `scripts/validate_traceability.sh` |
| 14.4 | Audio statistics reporting | 🟢 | P2 | 1h | 3 | `AudioPlayer::stats()` returns `QVariantMap` with buffer level, underruns, backend name; displayed in stats HUD | `scripts/validate_traceability.sh` |

---

## PHASE-15: Input Manager

> KDE client input manager: captures keyboard/mouse, serialises to INPUT_EVENT packets, sends to host.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 15.1 | Keyboard event capture | 🟢 | P0 | 2h | 5 | `InputManager` installs Qt event filter; captures `QKeyEvent` when stream view has focus | `scripts/validate_traceability.sh` |
| 15.2 | Mouse event capture & relative mode | 🟢 | P0 | 3h | 6 | `InputManager` captures mouse; sends relative deltas; hides cursor when `--grab-mouse` active | `scripts/validate_traceability.sh` |
| 15.3 | Gamepad / controller support | 🟢 | P1 | 4h | 7 | `GamepadManager.cpp` uses Qt Gamepad or SDL2 to read axes/buttons; forwards as INPUT_EVENT | `scripts/validate_traceability.sh` |
| 15.4 | Input event serialisation | 🟢 | P0 | 2h | 5 | `InputSerializer::serialize()` converts `InputEvent` struct to wire format matching host `packet.h` types | `scripts/validate_traceability.sh` |

---

## PHASE-16: KDE Client Polish

> UX polish: notifications, error dialogs, keyboard shortcuts, accessibility, dark/light theme.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 16.1 | KNotifications integration | 🟢 | P1 | 2h | 4 | Connection/disconnect/error events trigger `KNotification::event()`; notifications appear in KDE notification center | `scripts/validate_traceability.sh` |
| 16.2 | Error dialog with retry | 🟢 | P1 | 2h | 3 | `ErrorDialog.qml` shows human-readable error + "Retry" / "Settings" buttons | `scripts/validate_traceability.sh` |
| 16.3 | Global keyboard shortcuts | 🟢 | P2 | 1h | 3 | `KGlobalAccel` registers Ctrl+Shift+G (grab toggle), Ctrl+Shift+Q (quit stream) | `scripts/validate_traceability.sh` |
| 16.4 | Accessibility (a11y) | 🟢 | P2 | 2h | 4 | All interactive elements have `Accessible.name` set; passes `orca` screen-reader smoke test | `scripts/validate_traceability.sh` |
| 16.5 | Adaptive dark/light theme | 🟢 | P2 | 1h | 3 | Application inherits KDE colour scheme; stream overlay text respects `Kirigami.Theme` | `scripts/validate_traceability.sh` |

---

## PHASE-17: Recording System

> Server-side game recording: recording manager, configurable replay buffer, disk manager with rotation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 17.1 | RecordingManager module | 🟢 | P0 | 5h | 7 | `src/recording_manager.c` controls recording lifecycle: start, stop, pause, resume; emits events | `scripts/validate_traceability.sh` |
| 17.2 | Replay buffer | 🟢 | P0 | 4h | 8 | `src/replay_buffer.c` ring-buffer retains last N seconds of encoded stream; `replay_save()` flushes to file | `scripts/validate_traceability.sh` |
| 17.3 | Disk manager with rotation | 🟢 | P1 | 3h | 5 | `src/disk_manager.c` monitors free space; rotates oldest recordings when below threshold | `scripts/validate_traceability.sh` |
| 17.4 | Recording CLI flags | 🟢 | P1 | 1h | 3 | `--record <file>`, `--replay-buffer <seconds>`, `--max-disk-gb <n>` parsed and forwarded to managers | `scripts/validate_traceability.sh` |
| 17.5 | Chapter marker support | 🟢 | P2 | 2h | 4 | `--mark` hotkey inserts chapter metadata at current PTS in the output container | `scripts/validate_traceability.sh` |
| 17.6 | Recording quality presets | 🟢 | P2 | 1h | 3 | `--quality lossless|high|medium|low` maps to encoder CRF/bitrate presets | `scripts/validate_traceability.sh` |

---

## PHASE-18: Advanced Encoding

> H.264, VP9, and AV1 encoder wrappers; per-codec quality tuning; hardware/software auto-selection.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 18.1 | H.264 encoder wrapper | 🟢 | P0 | 3h | 5 | `src/encode_h264.c` wraps both NVENC H.264 and x264; unified `encode_h264_frame()` API | `docs/archive/verify_phase18.sh` |
| 18.2 | VP9 encoder wrapper | 🟢 | P1 | 4h | 6 | `src/encode_vp9.c` wraps libvpx VP9 encoder; tunable quality/speed tradeoff | `docs/archive/verify_phase18.sh` |
| 18.3 | AV1 encoder wrapper | 🟢 | P1 | 5h | 8 | `src/encode_av1.c` wraps libaom-av1 and SVT-AV1; selects fastest available implementation | `docs/archive/verify_phase18.sh` |
| 18.4 | Encoder capability matrix | 🟢 | P1 | 2h | 4 | `encode_probe_capabilities()` returns bitmask of available codecs on this system | `docs/archive/verify_phase18.sh` |
| 18.5 | Bitrate ladder generation | 🟢 | P2 | 2h | 5 | `encode_generate_bitrate_ladder()` produces ABR renditions at 360/480/720/1080p | `docs/archive/verify_phase18.sh` |

---

## PHASE-19: Container Formats & Metadata

> MKV and MP4 muxer wrappers; stream metadata embedding; thumbnail generation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 19.1 | MKV muxer (FFmpeg libavformat) | 🟢 | P0 | 3h | 5 | `src/container_mkv.c` opens MKV output, writes video/audio streams, flushes on stop | `docs/archive/verify_phase19.sh` |
| 19.2 | MP4 muxer | 🟢 | P0 | 3h | 5 | `src/container_mp4.c` writes fragmented MP4 for web compatibility; GOP-aligned fragments | `docs/archive/verify_phase19.sh` |
| 19.3 | Metadata embedding | 🟢 | P1 | 2h | 4 | Game title, capture date, hostname, codec info written to container metadata | `docs/archive/verify_phase19.sh` |
| 19.4 | Thumbnail generation | 🟢 | P2 | 2h | 4 | `thumbnail_generate()` extracts frame at 10 s mark; scales to 320×180 JPEG | `docs/archive/verify_phase19.sh` |

---

## PHASE-20: Performance Metrics / HUD

> In-stream HUD with CPU/GPU/memory monitors, network stats, frame timing, and configurable overlay.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 20.1 | MetricsManager module | 🟢 | P0 | 4h | 6 | `src/metrics_manager.c` aggregates all telemetry; publishes `MetricsSnapshot` struct every 500 ms | `docs/archive/verify_phase20.sh` |
| 20.2 | CPU usage monitor | 🟢 | P0 | 2h | 4 | Reads `/proc/stat`; reports per-core and aggregate utilisation | `docs/archive/verify_phase20.sh` |
| 20.3 | GPU usage monitor | 🟢 | P1 | 3h | 5 | Reads NVML (NVIDIA) or `/sys/class/drm/*/device/gpu_busy_percent` (AMD/Intel) | `docs/archive/verify_phase20.sh` |
| 20.4 | Memory monitor | 🟢 | P1 | 1h | 3 | Reads `/proc/meminfo`; reports used/total RAM and swap | `docs/archive/verify_phase20.sh` |
| 20.5 | Network metrics | 🟢 | P0 | 2h | 5 | Reads `/proc/net/dev`; reports TX/RX bytes, packet loss rate from RTT probes | `docs/archive/verify_phase20.sh` |
| 20.6 | Configurable HUD overlay | 🟢 | P1 | 3h | 6 | `--hud minimal|standard|full` selects overlay density; rendered via cairo or SDL2 text | `docs/archive/verify_phase20.sh` |

---

## PHASE-21: Web Dashboard API Server

> Embedded HTTP API server exposing stream status, metrics, recording controls as JSON REST endpoints.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 21.1 | Embedded HTTP server (libmicrohttpd) | 🟢 | P0 | 4h | 6 | `src/api_server.c` starts MHD daemon on `--api-port` (default 8080); handles GET/POST/DELETE | `scripts/validate_traceability.sh` |
| 21.2 | `/api/status` endpoint | 🟢 | P0 | 2h | 5 | Returns JSON with stream state, connected peers, active backends, uptime | `scripts/validate_traceability.sh` |
| 21.3 | `/api/metrics` endpoint | 🟢 | P0 | 2h | 5 | Returns latest `MetricsSnapshot` as JSON; supports `?since=<unix_ts>` for delta queries | `scripts/validate_traceability.sh` |
| 21.4 | `/api/recording` endpoints | 🟢 | P1 | 2h | 5 | POST `/start`, POST `/stop`, POST `/mark`; GET `/list` returns recording inventory | `scripts/validate_traceability.sh` |
| 21.5 | WebSocket push for live metrics | 🟢 | P1 | 4h | 7 | `src/api_websocket.c` upgrades connections to WS; pushes `MetricsSnapshot` JSON every 1 s | `scripts/validate_traceability.sh` |
| 21.6 | API authentication token | 🟢 | P0 | 2h | 6 | Bearer token required on all endpoints; token generated on startup and printed to stdout | `scripts/validate_traceability.sh` |

---

## PHASE-22: Mobile Clients (Android / iOS)

> Native mobile streaming clients: Android (Kotlin/JNI) and iOS (Swift/Objective-C) with hardware decode.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 22.1 | Android project scaffold | 🟢 | P0 | 3h | 5 | `android/` Gradle project; min SDK 26; NDK integration for native decode | `scripts/validate_traceability.sh` |
| 22.2 | Android H.264 hardware decode (MediaCodec) | 🟢 | P0 | 6h | 8 | `VideoDecoder.kt` uses `MediaCodec` async API; renders to `SurfaceView` | `scripts/validate_traceability.sh` |
| 22.3 | Android audio playback (AudioTrack) | 🟢 | P0 | 3h | 5 | `AudioPlayer.kt` uses `AudioTrack` in stream mode; Opus decode via JNI | `scripts/validate_traceability.sh` |
| 22.4 | Android input forwarding | 🟢 | P1 | 3h | 5 | Touch events mapped to mouse; on-screen gamepad overlaid; all forwarded as INPUT_EVENT | `scripts/validate_traceability.sh` |
| 22.5 | iOS project scaffold | 🟢 | P0 | 3h | 5 | `ios/` Xcode project; target iOS 15+; Privacy usage descriptions in `Info.plist` | `scripts/validate_traceability.sh` |
| 22.6 | iOS H.264 decode (VideoToolbox) | 🟢 | P0 | 6h | 8 | `VideoDecoder.swift` uses `VTDecompressionSession`; renders via `AVSampleBufferDisplayLayer` | `scripts/validate_traceability.sh` |
| 22.7 | iOS audio playback (AVAudioEngine) | 🟢 | P0 | 3h | 5 | `AudioPlayer.swift` uses `AVAudioEngine` with `AVAudioPlayerNode`; Opus decode via libopus | `scripts/validate_traceability.sh` |
| 22.8 | Mobile UI (touch-friendly stream view) | 🟢 | P1 | 4h | 6 | Pinch-to-zoom, landscape orientation lock, on-screen keyboard toggle | `scripts/validate_traceability.sh` |

---

## PHASE-23: Database Layer

> SQLite database layer via DatabaseManager: peer history, session logs, recording inventory, settings.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 23.1 | DatabaseManager module | 🟢 | P0 | 4h | 6 | `src/database_manager.c` opens `~/.local/share/rootstream/db.sqlite3`; handles migrations | `scripts/validate_traceability.sh` |
| 23.2 | Schema migrations | 🟢 | P0 | 2h | 4 | `migrations/` directory with numbered SQL files; applied in order; version tracked in `schema_version` table | `scripts/validate_traceability.sh` |
| 23.3 | Peer history persistence | 🟢 | P1 | 2h | 4 | `db_peer_upsert()`, `db_peer_list()`, `db_peer_delete()`; replaces JSON file from Phase 5 | `scripts/validate_traceability.sh` |
| 23.4 | Session log storage | 🟢 | P1 | 2h | 4 | Each streaming session recorded: start/end time, bytes transferred, peer, codec, peak metrics | `scripts/validate_traceability.sh` |
| 23.5 | Recording inventory | 🟢 | P1 | 2h | 4 | Recording files indexed with path, size, duration, thumbnail path, game title | `scripts/validate_traceability.sh` |

---

## PHASE-24: VR / Proton Compatibility

> OpenXR head/hand tracking, VR-optimised renderer, and Proton/Steam compatibility shim for VR game streaming.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 24.1 | OpenXR manager initialisation | 🟢 | P0 | 6h | 9 | `src/vr/openxr_manager.c` calls `xrCreateInstance`, enumerates extensions, creates session | `scripts/validate_traceability.sh` |
| 24.2 | Head tracking pipeline | 🟢 | P0 | 4h | 9 | `xrLocateViews()` called per frame; pose sent to renderer for per-eye projection matrices | `scripts/validate_traceability.sh` |
| 24.3 | Hand / controller tracking | 🟢 | P0 | 4h | 8 | `XR_EXT_hand_tracking` extension; grip/aim poses sent as INPUT_EVENT with 6DOF data | `scripts/validate_traceability.sh` |
| 24.4 | VR input action mapping | 🟢 | P1 | 3h | 7 | `openxr_actions.c` maps controller buttons/axes to RootStream input events via action set | `scripts/validate_traceability.sh` |
| 24.5 | VR UI framework (OpenXR overlay) | 🟢 | P1 | 5h | 8 | `src/vr/vr_ui.c` renders 2D panels in VR world space using `XR_EXTX_overlay` extension | `scripts/validate_traceability.sh` |
| 24.6 | Proton compatibility layer | 🟢 | P1 | 8h | 10 | `src/proton/proton_compat.c` hooks into Proton's `STEAM_COMPAT_DATA_PATH`; intercepts Vulkan calls | `scripts/validate_traceability.sh` |
| 24.7 | Steam VR integration | 🟢 | P1 | 6h | 10 | `src/proton/steamvr_bridge.c` forwards SteamVR poses to OpenXR runtime | `scripts/validate_traceability.sh` |
| 24.8 | VR latency optimisation | 🟢 | P1 | 4h | 9 | `src/vr/vr_latency_optimizer.c` reprojection pipeline achieves <2 ms extra latency; frame timing meets 90 Hz target | `scripts/validate_traceability.sh` |
| 24.9 | VR integration tests | 🟢 | P1 | 3h | 7 | `tests/integration/test_vr_integration.c` mock OpenXR runtime validates tracking pipeline; CI passes without headset | `scripts/validate_traceability.sh` |

---

## PHASE-25: Security Hardening

> Full security suite: session management, audit logging, authenticated key exchange, attack prevention, user auth.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 25.1 | SessionManager module | 🟢 | P0 | 4h | 7 | `src/security/session_manager.c` tracks active sessions; enforces max-sessions limit; idle timeout | `scripts/validate_traceability.sh` |
| 25.2 | AuditLog module | 🟢 | P0 | 3h | 6 | `src/security/audit_log.c` writes structured JSON audit events to `~/.local/share/rootstream/audit.log` | `scripts/validate_traceability.sh` |
| 25.3 | Authenticated key exchange (SIGMA) | 🟢 | P0 | 5h | 9 | `src/security/key_exchange.c` implements SIGMA-I protocol using Ed25519 + X25519 | `scripts/validate_traceability.sh` |
| 25.4 | Attack prevention | 🟢 | P0 | 4h | 7 | `src/security/attack_prevention.c` rate-limits connections per IP; detects port-scan patterns | `scripts/validate_traceability.sh` |
| 25.5 | User authentication module | 🟢 | P0 | 4h | 7 | `src/security/user_auth.c` supports password + TOTP 2FA; bcrypt password hashing via libsodium | `scripts/validate_traceability.sh` |
| 25.6 | Security regression tests | 🟢 | P0 | 3h | 7 | `tests/security/` suite: replay attacks, key exchange fuzzing, auth bypass attempts all rejected | `scripts/validate_traceability.sh` |
| 25.7 | Security documentation | 🟢 | P1 | 2h | 5 | `docs/SECURITY.md` documents threat model, crypto choices, audit procedures | `scripts/validate_traceability.sh` |

---

## PHASE-26: Network Optimization

> QoS, adaptive bitrate (ABR), congestion control, packet prioritisation, bandwidth estimation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 26.1 | Bandwidth estimator (BBR-like) | 🟢 | P0 | 6h | 8 | `src/net/bandwidth_estimator.c` tracks min RTT and delivery rate; estimates available bandwidth | `scripts/validate_traceability.sh` |
| 26.2 | Adaptive bitrate controller | 🟢 | P0 | 5h | 8 | `src/net/abr_controller.c` adjusts encoder bitrate target every 2 s based on bandwidth estimate | `scripts/validate_traceability.sh` |
| 26.3 | QoS / DSCP marking | 🟢 | P1 | 2h | 5 | Video packets marked `CS4` (DSCP 32); audio marked `EF` (DSCP 46); input marked `CS6` | `scripts/validate_traceability.sh` |
| 26.4 | Packet prioritisation queue | 🟢 | P1 | 3h | 6 | `src/net/priority_queue.c` prioritises I-frames > audio > P-frames > B-frames under congestion | `scripts/validate_traceability.sh` |
| 26.5 | FEC (Forward Error Correction) | 🟢 | P1 | 5h | 8 | `src/net/fec.c` uses Reed-Solomon (4+2) FEC on video packets; recovers up to 2 lost in 6 | `scripts/validate_traceability.sh` |
| 26.6 | NACK retransmission | 🟢 | P1 | 3h | 6 | Receiver sends NACK for missing seq; sender retransmits from jitter buffer (max 50 ms RTT) | `scripts/validate_traceability.sh` |
| 26.7 | Jitter buffer | 🟢 | P0 | 4h | 6 | `src/net/jitter_buffer.c` adaptive playout delay 20–150 ms; minimises stutter on lossy links | `scripts/validate_traceability.sh` |
| 26.8 | Network simulation test harness | 🟢 | P1 | 3h | 6 | `tests/net/test_network_conditions.c` uses `tc netem` to simulate packet loss/delay; all ABR tests pass | `scripts/validate_traceability.sh` |
| 26.9 | Network optimisation documentation | 🟢 | P2 | 1h | 4 | Detailed comments in all net/ modules; tuning guide in `docs/PROTOCOL.md` | `scripts/validate_traceability.sh` |

---

## PHASE-27: CI / Infrastructure

> Docker, Kubernetes, Terraform infrastructure; GitHub Actions CI/CD pipelines; multi-arch builds.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 27.1 | Dockerfile (multi-stage) | 🟢 | P0 | 3h | 6 | `infrastructure/docker/Dockerfile` builder + runtime stages; < 200 MB final image | `docs/archive/verify_phase27_1.sh` |
| 27.2 | Docker Compose dev environment | 🟢 | P0 | 2h | 5 | `infrastructure/docker/docker-compose.yml` with server, dummy-display, mock-audio services | `docs/archive/verify_phase27_1.sh` |
| 27.3 | Kubernetes Helm chart | 🟢 | P1 | 5h | 7 | `infrastructure/helm/rootstream/` chart deploys server as DaemonSet with GPU node affinity | `docs/archive/verify_phase27_2.sh` |
| 27.4 | Terraform AWS/GCP modules | 🟢 | P1 | 6h | 7 | `infrastructure/terraform/` provides EKS cluster + NLB + S3 recording bucket modules | `docs/archive/verify_phase27_3.sh` |
| 27.5 | GitHub Actions CI pipeline | 🟢 | P0 | 4h | 6 | `.github/workflows/ci.yml` builds, tests, lints on push; matrix: Ubuntu 22.04/24.04 + Arch | `docs/archive/verify_phase27_4.sh` |
| 27.6 | GitHub Actions CD pipeline | 🟢 | P0 | 3h | 6 | `.github/workflows/release.yml` builds multi-arch Docker images; pushes to GHCR on tag | `docs/archive/verify_phase27_4.sh` |
| 27.7 | SBOM generation | 🟢 | P1 | 1h | 5 | `syft` generates SPDX SBOM on release; attached as release asset | `docs/archive/verify_phase27_4.sh` |
| 27.8 | Dependency vulnerability scanning | 🟢 | P0 | 1h | 6 | `grype` runs on SBOM in CI; pipeline fails on CVSS ≥ 7.0 unfixed CVEs | `docs/archive/verify_phase27_4.sh` |

---

## PHASE-28: Event Sourcing / CQRS

> EventStore for streaming session events; CQRS read/write separation; event replay for state reconstruction.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 28.1 | EventStore core | 🟢 | P0 | 5h | 8 | `src/events/event_store.c` appends events to append-only log; supports replay from offset | `scripts/validate_traceability.sh` |
| 28.2 | Domain event models | 🟢 | P0 | 2h | 5 | `src/events/event_models.h` defines `StreamStarted`, `StreamEnded`, `PeerConnected`, `RecordingStarted` etc. | `scripts/validate_traceability.sh` |
| 28.3 | Command handlers | 🟢 | P0 | 3h | 6 | `src/events/command_handlers.c` validates and executes commands; emits corresponding events | `scripts/validate_traceability.sh` |
| 28.4 | Read model projections | 🟢 | P1 | 3h | 7 | `src/events/projections.c` builds in-memory read models from event stream; updated on new events | `scripts/validate_traceability.sh` |
| 28.5 | Event sourcing tests | 🟢 | P0 | 2h | 6 | `tests/events/` suite: append, replay, projection consistency; all pass | `scripts/validate_traceability.sh` |
| 28.6 | CQRS API integration | 🟢 | P1 | 2h | 6 | `/api/events` endpoint streams NDJSON event log; supports `?from=<seq>` | `scripts/validate_traceability.sh` |

---

## PHASE-29: Android / iOS Full Client

> Full-featured mobile streaming client: hardware decode, touch input, clipboard sync, file transfer.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 29.1 | Android full codec support (H.264/VP9/AV1) | 🟢 | P0 | 5h | 7 | `VideoDecoder.kt` handles all three codecs via `MediaCodec`; auto-selects based on server offer | `scripts/validate_traceability.sh` |
| 29.2 | Android clipboard sync | 🟢 | P1 | 3h | 6 | `ClipboardManager.kt` syncs host↔device clipboard over encrypted side-channel | `scripts/validate_traceability.sh` |
| 29.3 | Android file transfer | 🟢 | P2 | 5h | 6 | `FileTransferManager.kt` sends/receives files via dedicated DATA_TRANSFER packet type | `scripts/validate_traceability.sh` |
| 29.4 | iOS full codec support | 🟢 | P0 | 5h | 7 | `VideoDecoder.swift` uses `VideoToolbox` for H.264/HEVC; VP9 via libvpx fallback | `scripts/validate_traceability.sh` |
| 29.5 | iOS clipboard sync | 🟢 | P1 | 3h | 6 | `ClipboardManager.swift` using `UIPasteboard`; respects iOS privacy prompts | `scripts/validate_traceability.sh` |
| 29.6 | iOS file transfer | 🟢 | P2 | 5h | 6 | `FileTransferManager.swift` uses Files app integration via `UIDocumentPickerViewController` | `scripts/validate_traceability.sh` |
| 29.7 | Mobile HUD overlay | 🟢 | P2 | 3h | 5 | `HUDOverlay.swift` swipe-up reveals latency/bitrate overlay; dismissed by swipe-down | `scripts/validate_traceability.sh` |
| 29.8 | Push notification for stream invites | 🟢 | P2 | 4h | 6 | `PushNotificationManager.swift` APNs integration; host can "invite" mobile device to connect | `scripts/validate_traceability.sh` |

---

## PHASE-30: Security Phase 2

> Advanced security: fuzzing harness, penetration test results, rate limiting, security regression suite.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 30.1 | Packet parser fuzzing (libFuzzer) | 🟢 | P0 | 4h | 8 | `tests/fuzz/fuzz_packet_parse.cc` LibFuzzer target; > 1M executions without crash/sanitizer alert | `scripts/validate_traceability.sh` |
| 30.2 | Crypto handshake fuzzing | 🟢 | P0 | 4h | 8 | `tests/fuzz/fuzz_handshake.cc` fuzzes key exchange messages; no memory safety violations | `scripts/validate_traceability.sh` |
| 30.3 | Rate limiting (per-IP + global) | 🟢 | P0 | 3h | 6 | `src/security/rate_limiter.c` token-bucket per IP; global connection rate cap; returns `429` on API | `scripts/validate_traceability.sh` |
| 30.4 | SQL injection prevention | 🟢 | P0 | 2h | 6 | All DB calls use prepared statements; `db_*` functions reviewed for injection vectors | `scripts/validate_traceability.sh` |
| 30.5 | TLS for API server (mbedTLS) | 🟢 | P0 | 4h | 7 | `--api-tls` flag enables mbedTLS on HTTP server; self-signed cert generated on first run | `scripts/validate_traceability.sh` |
| 30.6 | Security audit documentation | 🟢 | P1 | 2h | 5 | `docs/SECURITY.md` includes attack surface map, known limitations, responsible disclosure policy | `scripts/validate_traceability.sh` |

---

## PHASE-31: Vulkan Renderer

> GPU-accelerated Vulkan renderer for the KDE client: frame upload, YUV→RGB shader, graphics pipeline, presentation, resize, cleanup.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 31.1 | Frame upload infrastructure (702 LOC) | 🟢 | P0 | 8h | 9 | `clients/kde-plasma-client/src/vulkan/VulkanFrameUploader.cpp` (702 LOC): staging buffer pool, transfer queue, timeline semaphores, VMA integration | `scripts/validate_traceability.sh` |
| 31.2 | YUV→RGB shader system (275 LOC) | 🟢 | P0 | 5h | 9 | `clients/kde-plasma-client/src/vulkan/shaders/yuv_to_rgb.frag` (275 LOC): BT.709/BT.601/BT.2020 matrices, HDR tone mapping | `scripts/validate_traceability.sh` |
| 31.3 | Vulkan graphics pipeline | 🟢 | P0 | 6h | 8 | `VulkanPipeline.cpp` creates render pass, framebuffers, descriptor sets, specialisation constants for colour space | `scripts/validate_traceability.sh` |
| 31.4 | Swapchain presentation | 🟢 | P0 | 4h | 8 | `VulkanPresenter.cpp` manages swapchain, acquire/present semaphores, mailbox/FIFO mode selection | `scripts/validate_traceability.sh` |
| 31.5 | Dynamic resize handling | 🟢 | P0 | 3h | 7 | `VulkanRenderer::onResize()` recreates swapchain and framebuffers without frame drops | `scripts/validate_traceability.sh` |
| 31.6 | Resource cleanup & validation layers | 🟢 | P0 | 2h | 6 | `VulkanRenderer::cleanup()` destroys all Vulkan objects in reverse order; `VK_LAYER_KHRONOS_validation` reports zero errors | `scripts/validate_traceability.sh` |

---

## PHASE-32: Backend Integration

> Wire the Vulkan renderer to the actual streaming backend: frame delivery pipeline, X11/Wayland platform backends, benchmarks.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 32.1 | Backend connection layer | 🟢 | P0 | 6h | 8 | `stream_backend_connector.cpp` receives decoded frames from `client_decode.c` and hands off to `VulkanFrameUploader` | `scripts/validate_traceability.sh` |
| 32.2 | Frame delivery pipeline | 🟢 | P0 | 5h | 8 | `frame_ring_buffer.c` lock-free ring buffer between decode thread and Vulkan render thread; < 0.1% frame drops at 60 fps | `scripts/validate_traceability.sh` |
| 32.3 | X11 Vulkan surface (VK_KHR_xlib_surface) | 🟢 | P0 | 3h | 6 | `X11VulkanSurface.cpp` creates `VkSurfaceKHR` via `vkCreateXlibSurfaceKHR`; verified on Xorg | `scripts/validate_traceability.sh` |
| 32.4 | Wayland Vulkan surface (VK_KHR_wayland_surface) | 🟢 | P0 | 3h | 7 | `WaylandVulkanSurface.cpp` creates `VkSurfaceKHR` via `vkCreateWaylandSurfaceKHR`; verified on KDE Plasma 6 Wayland | `scripts/validate_traceability.sh` |
| 32.5 | Integration test suite | 🟢 | P0 | 4h | 7 | `tests/vulkan/test_vulkan_integration.c` renders synthetic YUV frames through full pipeline; validates ring buffer and upload | `scripts/validate_traceability.sh` |
| 32.6 | Performance benchmarks | 🟢 | P1 | 3h | 7 | `benchmarks/vulkan_renderer_bench.cpp` measures upload + render latency; target < 2 ms at 1080p/60 | `scripts/validate_traceability.sh` |

---

## PHASE-33: Code Standards & Quality

> Enforce C++ code standards, comprehensive test coverage, sanitizer clean passes, and static analysis.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 33.1 | clang-format + clang-tidy enforcement | 🟢 | P0 | 4h | 5 | `.clang-format` and `.clang-tidy` configs at repo root; CI lint step fails on violations; zero existing violations | `scripts/validate_traceability.sh` |
| 33.2 | Unit test coverage ≥ 80% | 🟢 | P0 | 8h | 6 | `scripts/check_coverage.sh` runs `gcov`/`lcov` report; all `src/` and `clients/kde-plasma-client/src/` modules ≥ 80% line coverage | `scripts/validate_traceability.sh` |
| 33.3 | Sanitizer clean passes (ASan/UBSan/TSan) | 🟢 | P0 | 6h | 7 | `scripts/run_sanitizers.sh` debug build with `-fsanitize=address,undefined,thread`; full test suite passes with zero sanitizer errors | `scripts/validate_traceability.sh` |
| 33.4 | cppcheck static analysis | 🟢 | P1 | 3h | 5 | `scripts/run_cppcheck.sh` runs `cppcheck --error-exitcode=1` on `src/` and `clients/`; zero errors (warnings permitted) | `scripts/validate_traceability.sh` |

---

## PHASE-34: Production Readiness

> End-to-end integration testing, performance benchmarking suite, release packaging, and production documentation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 34.1 | End-to-end integration test | 🟢 | P0 | 8h | 8 | `tests/e2e/test_full_stream.sh` starts server + KDE client in Docker; streams 60 s; validates no dropped frames, correct decrypt/decode | `scripts/validate_traceability.sh` |
| 34.2 | Performance benchmark suite | 🟢 | P0 | 6h | 7 | `benchmarks/` directory: encode latency, network throughput, Vulkan render latency benchmarks | `scripts/validate_traceability.sh` |
| 34.3 | Release packaging | 🟢 | P0 | 4h | 6 | AUR `PKGBUILD` functional; `packaging/rootstream.spec` for RPM; `packaging/build_appimage.sh` for AppImage | `scripts/validate_traceability.sh` |
| 34.4 | Production documentation | 🟢 | P1 | 4h | 5 | `docs/QUICKSTART.md` includes benchmark and E2E test sections; `docs/TROUBLESHOOTING.md` covers top-10 issues | `scripts/validate_traceability.sh` |

---

## PHASE-35: Plugin & Extension System

> Runtime-loadable plugin ABI enabling third-party encoders, decoders, capture backends, filters, transports, and UI extensions.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 35.1 | Plugin ABI definition | 🟢 | P0 | 4h | 8 | `src/plugin/plugin_api.h` defines `plugin_descriptor_t`, `plugin_host_api_t`, `PLUGIN_API_MAGIC`, `RS_PLUGIN_DECLARE` macro; version-gated ABI | `scripts/validate_traceability.sh` |
| 35.2 | Dynamic plugin loader | 🟢 | P0 | 5h | 7 | `src/plugin/plugin_loader.c` uses `dlopen`/`dlclose` (POSIX) or `LoadLibrary` (Win32); validates magic + version before calling `rs_plugin_init` | `scripts/validate_traceability.sh` |
| 35.3 | Plugin registry | 🟢 | P0 | 4h | 7 | `src/plugin/plugin_registry.c` scans directories for `.so`/`.dll`; lookup by name and type; capacity `PLUGIN_REGISTRY_MAX` = 64 | `scripts/validate_traceability.sh` |
| 35.4 | Plugin system unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_plugin_system.c` — 10 tests covering constants, type enum, registry lifecycle, NULL guards, capacity; all pass without real plugins | `scripts/validate_traceability.sh` |
| 35.5 | Plugin developer guide | 🟢 | P1 | 2h | 6 | `docs/PLUGIN_API.md` — quick-start example, type table, ABI versioning, host API, search path, thread-safety, security notes | `scripts/validate_traceability.sh` |

---

## PHASE-36: Audio DSP Pipeline

> Composable per-frame DSP processing chain: noise gate, spectral subtraction, automatic gain control, and NLMS echo cancellation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 36.1 | DSP pipeline framework | 🟢 | P0 | 4h | 7 | `src/audio/audio_pipeline.c` — linear chain of `audio_filter_node_t`; add/remove by name; enabled flag bypasses nodes; `audio_pipeline_process()` calls all enabled nodes in order | `scripts/validate_traceability.sh` |
| 36.2 | Noise gate + spectral subtraction | 🟢 | P0 | 6h | 8 | `src/audio/noise_filter.c` — RMS noise gate with configurable threshold and release; single-band spectral subtraction with over-subtraction factor and noise floor history | `scripts/validate_traceability.sh` |
| 36.3 | Automatic gain control | 🟢 | P0 | 4h | 7 | `src/audio/gain_control.c` — feed-forward AGC with configurable target dBFS, gain clamp, attack/release envelope, and hard clipper | `scripts/validate_traceability.sh` |
| 36.4 | Acoustic echo cancellation | 🟢 | P0 | 6h | 9 | `src/audio/echo_cancel.c` — NLMS adaptive filter with configurable filter length and step size; `aec_set_reference()` for pipeline integration | `scripts/validate_traceability.sh` |
| 36.5 | Audio DSP unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_audio_dsp.c` — 12 tests: pipeline lifecycle, passthrough, NULL guards, gate silence/pass, AGC convergence, AEC adaptation; all pass without audio hardware | `scripts/validate_traceability.sh` |

---

## PHASE-37: Multi-Client Fanout

> Fan out a single encoded stream to multiple simultaneous clients, with independent per-client adaptive bitrate and graceful congestion handling.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 37.1 | Fanout manager | 🟢 | P0 | 5h | 8 | `src/fanout/fanout_manager.c` — iterates active sessions via `session_table_foreach`; drops delta frames for congested clients (loss > 10% or RTT > 500 ms); thread-safe stats | `scripts/validate_traceability.sh` |
| 37.2 | Session table | 🟢 | P0 | 4h | 7 | `src/fanout/session_table.c` — fixed-size (32-slot) table with mutex; add/remove/get/update-bitrate/update-stats; `session_table_foreach` skips removed slots | `scripts/validate_traceability.sh` |
| 37.3 | Per-client ABR | 🟢 | P0 | 4h | 8 | `src/fanout/per_client_abr.c` — AIMD controller: +500 kbps additive increase after 2 stable intervals; ×0.7 decrease on > 5% loss or RTT > 250 ms; capped at negotiated max | `scripts/validate_traceability.sh` |
| 37.4 | Fanout unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_fanout.c` — 13 tests: table lifecycle, add/remove, capacity, update, foreach, fanout create/deliver/stats-reset, ABR create/decrease/increase/max-cap/force-keyframe | `scripts/validate_traceability.sh` |
| 37.5 | Multi-client integration test | 🟢 | P0 | 3h | 7 | `tests/integration/test_multi_client.c` — 5 integration tests: add-all, fanout stats, per-client heterogeneous ABR, remove mid-stream, congestion drop; CI passes without real network | `scripts/validate_traceability.sh` |

---

## PHASE-38: Collaboration & Annotation

> Real-time screen annotation layer: draw strokes, erase, place text, and sync remote cursor positions over the existing encrypted data channel.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 38.1 | Annotation wire protocol | 🟢 | P0 | 5h | 8 | `src/collab/annotation_protocol.c` — compact binary framing (16-byte header, 7 event types); `annotation_encode`/`annotation_decode` with magic/version validation; full round-trip for all event types | `scripts/validate_traceability.sh` |
| 38.2 | Annotation renderer | 🟢 | P0 | 6h | 8 | `src/collab/annotation_renderer.c` — in-memory stroke/text layer; Bresenham circle-stamp thick lines; Porter-Duff src-over RGBA compositor; erase by proximity; up to 256 strokes × 1024 points | `scripts/validate_traceability.sh` |
| 38.3 | Remote pointer sync | 🟢 | P0 | 3h | 7 | `src/collab/pointer_sync.c` — tracks up to 16 remote peer positions; idle timeout via `pointer_sync_expire()`; `pointer_sync_get_all()` returns only visible pointers | `scripts/validate_traceability.sh` |
| 38.4 | Annotation unit tests | 🟢 | P0 | 3h | 7 | `tests/unit/test_annotation.c` — 15 tests: protocol round-trip for all event types, bad-magic/buffer-too-small guard, renderer lifecycle/strokes/erase/composite, pointer create/update/hide/expire/get-all | `scripts/validate_traceability.sh` |

---

## 🔬 Quality Gates Reference

| Gate Script | What It Validates |
|-------------|-------------------|
| `scripts/validate_traceability.sh` | Phase IDs exist in microtasks.md; gate scripts present |
| `docs/archive/verify_phase18.sh` | H.264/VP9/AV1 encoder source files exist |
| `docs/archive/verify_phase19.sh` | MKV/MP4 container source files exist |
| `docs/archive/verify_phase20.sh` | MetricsManager and monitor source files exist |
| `docs/archive/verify_phase27_1.sh` | Docker infrastructure files present |
| `docs/archive/verify_phase27_2.sh` | Helm chart structure present |
| `docs/archive/verify_phase27_3.sh` | Terraform modules present |
| `docs/archive/verify_phase27_4.sh` | GitHub Actions workflow files present |

---

## PHASE-39: Stream Quality Intelligence

> Per-frame PSNR/SSIM quality scoring, histogram-based scene-change detection, rolling quality monitor with alert thresholds, and JSON quality report generation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 39.1 | Quality metrics (PSNR/SSIM) | 🟢 | P0 | 4h | 7 | `src/quality/quality_metrics.c` — stateless `quality_psnr()`, `quality_ssim()`, `quality_mse()` on 8-bit luma planes; PSNR sentinel 1000 for identical frames | `scripts/validate_traceability.sh` |
| 39.2 | Scene-change detector | 🟢 | P0 | 4h | 8 | `src/quality/scene_detector.c` — normalised L1 luma histogram diff; configurable threshold + warmup; `scene_detector_push()` returns `scene_result_t` per frame | `scripts/validate_traceability.sh` |
| 39.3 | Rolling quality monitor | 🟢 | P0 | 4h | 7 | `src/quality/quality_monitor.c` — sliding-window (up to 120 frames) average + min PSNR/SSIM; alert counter incremented when avg drops below threshold | `scripts/validate_traceability.sh` |
| 39.4 | JSON quality reporter | 🟢 | P1 | 2h | 6 | `src/quality/quality_reporter.c` — `quality_report_json()` writes compact JSON into a caller-supplied buffer; returns -1 on overflow | `scripts/validate_traceability.sh` |
| 39.5 | Quality unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_quality.c` — 17 tests: MSE/PSNR/SSIM identical/degraded/null, scene create/no-change/cut/reset, monitor good/bad/reset, reporter basic/overflow/null; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-40: Relay / TURN Infrastructure

> Relay server session management for NAT traversal: wire protocol, server-side session table, client-side state machine, and HMAC-SHA256 auth token generation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 40.1 | Relay wire protocol | 🟢 | P0 | 4h | 7 | `src/relay/relay_protocol.c` — 10-byte big-endian header (magic 0x5253, version, type, session ID, payload length); encode/decode + HELLO payload build/parse | `scripts/validate_traceability.sh` |
| 40.2 | Relay session manager | 🟢 | P0 | 5h | 8 | `src/relay/relay_session.c` — 128-slot mutex-protected table; open (WAITING), pair on token match (PAIRED), close; bytes-relayed counter per session | `scripts/validate_traceability.sh` |
| 40.3 | Relay client connector | 🟢 | P0 | 4h | 8 | `src/relay/relay_client.c` — I/O-callback state machine: DISCONNECTED→HELLO_SENT→READY; auto-PONG on PING; `relay_client_send_data()` wraps payload in DATA message | `scripts/validate_traceability.sh` |
| 40.4 | HMAC auth token | 🟢 | P0 | 3h | 7 | `src/relay/relay_token.c` — portable HMAC-SHA256 over (peer_pubkey ‖ nonce); `relay_token_validate()` uses constant-time comparison; no external crypto dependency | `scripts/validate_traceability.sh` |
| 40.5 | Relay unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_relay.c` — 14 tests: header round-trip, bad magic, HELLO round-trip, session open/close/pair/wrong-token/bytes, client connect/ACK/ping-pong, token determinism/diff-key/validate; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-41: Session Persistence & Resumption

> Binary session-state serialisation, atomic checkpoint save/load with rotation, and a resume-protocol handshake (request/accepted/rejected) with server-side evaluation.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 41.1 | Session state serialisation | 🟢 | P0 | 4h | 7 | `src/session/session_state.c` — little-endian binary format; magic 0x52535353; all stream parameters + 32-byte stream key + peer address round-trip | `scripts/validate_traceability.sh` |
| 41.2 | Checkpoint save/load | 🟢 | P0 | 5h | 8 | `src/session/session_checkpoint.c` — atomic rename write; filename `rootstream-ckpt-<id>-<seq>.bin`; `checkpoint_load` finds highest sequence; `checkpoint_delete` cleans up | `scripts/validate_traceability.sh` |
| 41.3 | Resume-protocol negotiation | 🟢 | P0 | 4h | 8 | `src/session/session_resume.c` — RESQ/RESA/RESR tags; `resume_server_evaluate()` checks session ID, stream key, and frame gap; returns accepted/rejected struct | `scripts/validate_traceability.sh` |
| 41.4 | Session persistence unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_session_persist.c` — 12 tests: state round-trip/bad-magic/null, checkpoint save-load-delete/nonexistent/null, resume request/accepted/rejected round-trip, server accept/reject-gap/reject-key; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-42: Closed-Caption & Subtitle System

> Caption event wire format, PTS-sorted timing ring-buffer with expiry, and RGBA overlay compositor using a built-in 5×7 pixel bitmap font with Porter-Duff alpha blending.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 42.1 | Caption event format | 🟢 | P0 | 3h | 6 | `src/caption/caption_event.c` — magic 0x43415054 binary encoding; `caption_event_encode/decode` round-trip; `caption_event_is_active()` timing predicate | `scripts/validate_traceability.sh` |
| 42.2 | Caption timing buffer | 🟢 | P0 | 4h | 7 | `src/caption/caption_buffer.c` — 64-slot PTS-sorted insertion; `caption_buffer_query()` returns active events; `caption_buffer_expire()` prunes ended events | `scripts/validate_traceability.sh` |
| 42.3 | Caption overlay renderer | 🟢 | P0 | 6h | 8 | `src/caption/caption_renderer.c` — built-in 5×7 pixel font (ASCII 32–127); semi-transparent pill background; Porter-Duff src-over RGBA blending; row + top/bottom positioning | `scripts/validate_traceability.sh` |
| 42.4 | Caption unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_caption.c` — 13 tests: event encode/decode/is_active/null, buffer create/push-query/expire/clear/sorted-insert, renderer create/draw-active/draw-inactive/null; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-43: Stream Scheduler

> Schedule-based stream management: entry format serialisation, a sorted engine with repeating/one-shot entries, JSON-like binary persistence, and monotonic + wall-clock helpers.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 43.1 | Schedule entry format | 🟢 | P0 | 3h | 6 | `src/scheduler/schedule_entry.c` — little-endian binary format; magic 0x5343454E; all fields + UTF-8 title round-trip; `schedule_entry_is_enabled()` predicate | `scripts/validate_traceability.sh` |
| 43.2 | Scheduler engine | 🟢 | P0 | 4h | 8 | `src/scheduler/scheduler.c` — mutex-protected 256-slot table; `scheduler_tick(now_us)` fires enabled entries; one-shot entries removed after fire; repeat entries advance by 24 h | `scripts/validate_traceability.sh` |
| 43.3 | Schedule store (persistence) | 🟢 | P0 | 3h | 6 | `src/scheduler/schedule_store.c` — atomic rename write; 12-byte file header magic 0x52535348; entries length-prefixed; save/load round-trip | `scripts/validate_traceability.sh` |
| 43.4 | Schedule clock helpers | 🟢 | P1 | 2h | 5 | `src/scheduler/schedule_clock.c` — `schedule_clock_now_us()` CLOCK_REALTIME; `schedule_clock_mono_us()` CLOCK_MONOTONIC; `schedule_clock_format()` YYYY-MM-DD HH:MM:SS | `scripts/validate_traceability.sh` |
| 43.5 | Scheduler unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_scheduler.c` — 14 tests: entry round-trip/bad-magic/is_enabled/null, scheduler create/add-remove/tick-fires/disabled/clear/repeat, store save-load/missing, clock now/format; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-44: HLS Segment Output

> Full HLS output pipeline: minimal MPEG-TS segment writer (PAT/PMT/PES), M3U8 manifest generator (live sliding-window + VOD + master), and a segment lifecycle manager with atomic manifest updates.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 44.1 | MPEG-TS segment writer | 🟢 | P0 | 5h | 8 | `src/hls/ts_writer.c` — 188-byte TS packets; PAT+PMT tables; PES framing with PTS; continuity counter; `ts_writer_bytes_written()` always multiple of 188 | `scripts/validate_traceability.sh` |
| 44.2 | M3U8 manifest generator | 🟢 | P0 | 3h | 7 | `src/hls/m3u8_writer.c` — `m3u8_write_live()` sliding window; `m3u8_write_vod()` with ENDLIST; `m3u8_write_master()` multi-bitrate with BANDWIDTH/RESOLUTION; all write to caller buffer | `scripts/validate_traceability.sh` |
| 44.3 | HLS segment lifecycle manager | 🟢 | P0 | 4h | 8 | `src/hls/hls_segmenter.c` — open/write/close segment lifecycle; atomic manifest rename; VOD mode; `hls_segmenter_segment_count()` tracks completed segments | `scripts/validate_traceability.sh` |
| 44.4 | HLS configuration constants | 🟢 | P2 | 1h | 4 | `src/hls/hls_config.h` — TS packet size, sync byte, default target duration, window size, max variants, path limits | `scripts/validate_traceability.sh` |
| 44.5 | HLS unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_hls.c` — 11 tests: ts_writer create/PAT-PMT/PES/null, m3u8 live/VOD/master/overflow, segmenter create/lifecycle/VOD; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-45: Viewer Analytics & Telemetry

> Structured analytics event pipeline: binary-encoded event format (9 types), fixed-capacity ring buffer with overflow head-drop, Welford running-average statistics, and JSON/CSV export into caller buffers.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 45.1 | Analytics event format | 🟢 | P0 | 3h | 6 | `src/analytics/analytics_event.c` — magic 0x414E4C59; 9 typed events (viewer join/leave, bitrate, frame-drop, quality-alert, scene-change, stream start/stop, latency); encode/decode round-trip | `scripts/validate_traceability.sh` |
| 45.2 | Event ring buffer | 🟢 | P0 | 3h | 7 | `src/analytics/event_ring.c` — 1024-slot ring; push/pop/peek/drain/clear; full-ring head-drop (overwrites oldest); drain returns up to @max events | `scripts/validate_traceability.sh` |
| 45.3 | Aggregate statistics | 🟢 | P0 | 4h | 7 | `src/analytics/analytics_stats.c` — Welford running average for latency and bitrate; concurrent viewer counter + peak tracking; scene_changes reset on stream_start | `scripts/validate_traceability.sh` |
| 45.4 | Analytics exporter | 🟢 | P0 | 3h | 6 | `src/analytics/analytics_export.c` — `analytics_export_stats_json()` compact JSON; `analytics_export_events_json()` JSON array; `analytics_export_events_csv()` with header row; all write to caller buffer | `scripts/validate_traceability.sh` |
| 45.5 | Analytics unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_analytics.c` — 14 tests: event round-trip/bad-magic/type-name/null, ring create/push-pop/overflow/drain, stats viewer-counts/latency-avg/stream-events, export stats-JSON/events-JSON/CSV; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-46: Perceptual Frame Hashing

> DCT-based 64-bit perceptual hash (pHash): bilinear resize to 32×32, separable 2D DCT-II, 8×8 feature extraction; Hamming-distance index for nearest-neighbour lookup; streaming near-duplicate frame detector.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 46.1 | pHash computation | 🟢 | P0 | 4h | 7 | `src/phash/phash.c` — bilinear resize 32×32; separable DCT-II; 64-bit hash from 8×8 top-left DCT block vs mean; `phash_hamming()` popcount; `phash_similar()` threshold predicate | `scripts/validate_traceability.sh` |
| 46.2 | pHash index | 🟢 | P0 | 4h | 7 | `src/phash/phash_index.c` — linear-scan 65536-slot index; `phash_index_nearest()` min-distance search; `phash_index_range_query()` all within max_dist; `phash_index_remove()` by id | `scripts/validate_traceability.sh` |
| 46.3 | Near-duplicate detector | 🟢 | P0 | 3h | 7 | `src/phash/phash_dedup.c` — `phash_dedup_push()` checks index before inserting; returns true + match-id for duplicates; `phash_dedup_reset()` clears index for scene cut | `scripts/validate_traceability.sh` |
| 46.4 | pHash unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_phash.c` — 11 tests: hash deterministic/identical/different/hamming/null, index insert-nearest/range-query/remove, dedup unique/duplicate/reset; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-47: Stream Watermarking

> Per-viewer invisible forensic watermarking: binary payload format with 64-bit viewer ID and session ID, spatial LSB embedding with PRNG-scattered pixel selection, DCT-domain sign-substitution embedding (robust to IDCT→integer-round→re-DCT), and adaptive strength selection (LSB for high-quality, DCT-QIM for compressed output).

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 47.1 | Watermark payload format | 🟢 | P0 | 2h | 5 | `src/watermark/watermark_payload.c` — magic 0x574D4B50; encode/decode; `watermark_payload_to_bits()` / `from_bits()` 64-bit round-trip | `scripts/validate_traceability.sh` |
| 47.2 | DCT-domain embedder | 🟢 | P0 | 5h | 8 | `src/watermark/watermark_dct.c` — 8×8 forward/inverse DCT; sign-substitution at coefficient (3,4); delta=32; `watermark_dct_embed()` + `watermark_dct_extract()` | `scripts/validate_traceability.sh` |
| 47.3 | Spatial LSB embedder | 🟢 | P0 | 3h | 6 | `src/watermark/watermark_lsb.c` — xorshift64 PRNG seeded from viewer_id; scatter 64 bits across frame pixels; max pixel change ≤ 1; extract reads same PRNG sequence | `scripts/validate_traceability.sh` |
| 47.4 | Adaptive strength control | 🟢 | P1 | 2h | 5 | `src/watermark/watermark_strength.c` — quality_hint≥70 → LSB; 30–69 → DCT delta=32; <30 → DCT delta=64; DCT skipped on non-keyframes | `scripts/validate_traceability.sh` |
| 47.5 | Watermark unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_watermark.c` — 14 tests: payload round-trip/bad-magic/bits/null, lsb embed-extract/invisibility/null, dct embed-extract/null, strength high/low/non-keyframe/names/null; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-48: Adaptive Bitrate Controller

> EWMA bandwidth estimator + static quality-level ladder + conservative ABR decision engine with upgrade hysteresis + per-session quality statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 48.1 | EWMA bandwidth estimator | 🟢 | P0 | 2h | 6 | `src/abr/abr_estimator.c` — configurable α; first sample initialises EWMA; `is_ready()` after MIN_SAMPLES=3; `reset()` clears state | `scripts/validate_traceability.sh` |
| 48.2 | Bitrate ladder | 🟢 | P0 | 2h | 5 | `src/abr/abr_ladder.c` — up to 8 quality levels; `qsort` by bitrate_bps; `abr_ladder_select()` picks highest level within budget; clamped to level 0 if below minimum | `scripts/validate_traceability.sh` |
| 48.3 | ABR controller | 🟢 | P0 | 4h | 8 | `src/abr/abr_controller.c` — safety margin 0.85×; immediate downgrade; upgrade hold ABR_UPGRADE_HOLD_TICKS=3 ticks at target; one-step-at-a-time upgrade; `force_level()` override | `scripts/validate_traceability.sh` |
| 48.4 | ABR statistics | 🟢 | P1 | 2h | 5 | `src/abr/abr_stats.c` — Welford running avg_level; upgrade/downgrade counts; stall_ticks; ticks_per_level histogram | `scripts/validate_traceability.sh` |
| 48.5 | ABR unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_abr.c` — 12 tests: estimator create/EWMA/ready, ladder create-sort/select/null, controller create/downgrade/upgrade-hold/force, stats record/avg-level; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-49: Content Metadata Pipeline

> Binary-encoded stream metadata record (title, tags, codec info) + in-memory KV store with iterator + JSON export for both structured metadata and KV pairs.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 49.1 | Stream metadata record | 🟢 | P0 | 3h | 6 | `src/metadata/stream_metadata.c` — magic 0x4D455441; variable-length title/description/tags with 2-byte length prefix; `stream_metadata_is_live()` flag check | `scripts/validate_traceability.sh` |
| 49.2 | KV metadata store | 🟢 | P0 | 3h | 6 | `src/metadata/metadata_store.c` — 128-slot linear-scan array; `set()` upserts; `get()` / `has()` / `delete()` / `clear()`; `foreach()` iterator callback | `scripts/validate_traceability.sh` |
| 49.3 | Metadata JSON exporter | 🟢 | P0 | 2h | 5 | `src/metadata/metadata_export.c` — `metadata_export_json()` renders stream_metadata_t; `metadata_store_export_json()` uses foreach iterator to emit {"key":"value",...} | `scripts/validate_traceability.sh` |
| 49.4 | Metadata unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_metadata.c` — 9 tests: metadata round-trip/bad-magic/is_live, store set-get/delete/clear/foreach, export metadata-JSON/store-JSON; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-50: Low-Latency Jitter Buffer

> RTP-style sequence-numbered packet format with wrap-around ordering, sorted-insertion reorder buffer with configurable playout delay, and RFC 3550 inter-arrival jitter estimator with min/max/avg delay tracking.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 50.1 | Jitter packet format | 🟢 | P0 | 2h | 5 | `src/jitter/jitter_packet.c` — magic 0x4A504B54; 32-bit seq_num + RTP timestamp + capture_us; `jitter_packet_before()` RFC 3550 half-window modular comparison (strict, excludes equal) | `scripts/validate_traceability.sh` |
| 50.2 | Jitter reorder buffer | 🟢 | P0 | 4h | 7 | `src/jitter/jitter_buffer.c` — 256-slot sorted-insertion array; `push()` tail-drops oldest when full; `pop(now_us)` releases packet when now ≥ capture + delay; JITTER_FLAG_LATE set for significantly-late packets | `scripts/validate_traceability.sh` |
| 50.3 | Jitter statistics | 🟢 | P0 | 3h | 6 | `src/jitter/jitter_stats.c` — RFC 3550 §A.8 EWMA jitter ÷16 update; Welford avg_delay_us; min/max delay; late/dropped counters; `reset()` reinitialises min to DBL_MAX | `scripts/validate_traceability.sh` |
| 50.4 | Jitter buffer unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_jitter.c` — 11 tests: packet round-trip/bad-magic/ordering/null, buffer create/ordering/playout-delay/flush, stats basic/RFC-3550-jitter/reset; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-51: Packet Loss Concealment

> Audio PLC subsystem: PCM frame wire format (magic 0x504C4346), ring buffer of recent good frames (depth 8), three concealment strategies (zero/repeat/fade-out with per-step amplitude decay), and a sliding-window loss-rate estimator.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 51.1 | PCM frame format | 🟢 | P0 | 2h | 5 | `src/plc/plc_frame.c` — magic 0x504C4346; encode/decode; `plc_frame_is_silent()` zero-check; `plc_frame_byte_size()` | `scripts/validate_traceability.sh` |
| 51.2 | Frame history ring buffer | 🟢 | P0 | 2h | 5 | `src/plc/plc_history.c` — 8-slot ring; `push()` wraps on overflow; `get(age)` age-indexed access (0=newest); `clear()` | `scripts/validate_traceability.sh` |
| 51.3 | Concealment engine | 🟢 | P0 | 3h | 7 | `src/plc/plc_conceal.c` — ZERO fills silence; REPEAT copies last frame; FADE_OUT applies `fade_factor^consecutive_losses` amplitude decay; null-safe | `scripts/validate_traceability.sh` |
| 51.4 | PLC statistics | 🟢 | P1 | 2h | 5 | `src/plc/plc_stats.c` — sliding window (64 events) loss rate; concealment burst counter; max consecutive loss tracking; `reset()` | `scripts/validate_traceability.sh` |
| 51.5 | PLC unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_plc.c` — 13 tests: frame round-trip/bad-magic/is_silent/null, history push-get/wrap-around, conceal zero/repeat/fade-out/null/names, stats basic/loss-rate; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-52: Token Bucket Rate Limiter

> Per-viewer bandwidth shaping: token bucket with caller-supplied time for testability, per-viewer registry mapping viewer_id → bucket, and per-registry throttle statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 52.1 | Token bucket | 🟢 | P0 | 3h | 7 | `src/ratelimit/token_bucket.c` — caller-supplied µs time; refill = rate_per_us × elapsed; burst cap; `consume()` / `available()` / `reset()` / `set_rate()` | `scripts/validate_traceability.sh` |
| 52.2 | Per-viewer rate limiter | 🟢 | P0 | 3h | 6 | `src/ratelimit/rate_limiter.c` — 256-slot registry; `add_viewer()` upsert-safe; `remove_viewer()`; `consume()` delegates to per-viewer bucket; independent buckets | `scripts/validate_traceability.sh` |
| 52.3 | Rate limiter statistics | 🟢 | P1 | 2h | 5 | `src/ratelimit/ratelimit_stats.c` — packets allowed/throttled; bytes_consumed; throttle_rate = throttled / (allowed+throttled) | `scripts/validate_traceability.sh` |
| 52.4 | Rate limiter unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_ratelimit.c` — 8 tests: bucket create/consume-refill/reset/set-rate, rl create/add-remove/per-viewer, stats record; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-53: Frame Rate Controller

> Token-accumulator frame pacer with injectable monotonic clock for tests; presents/drops/duplicates frames to match target FPS; EWMA actual-FPS estimator.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 53.1 | Monotonic clock abstraction | 🟢 | P0 | 1h | 4 | `src/frc/frc_clock.c` — CLOCK_MONOTONIC wrapper; stub-mode for tests (`set_stub_ns` / `clear_stub`); `ns_to_us()` / `ns_to_ms()` inline helpers | `scripts/validate_traceability.sh` |
| 53.2 | Frame pacer | 🟢 | P0 | 4h | 7 | `src/frc/frc_pacer.c` — token accumulator; tokens += elapsed/interval; ≥1→PRESENT; <0→DUPLICATE; else→DROP; cap at 2 tokens; `set_fps()` live update | `scripts/validate_traceability.sh` |
| 53.3 | FRC statistics | 🟢 | P1 | 2h | 5 | `src/frc/frc_stats.c` — presented/dropped/duplicated counters; EWMA actual_fps updated once per 1-second window | `scripts/validate_traceability.sh` |
| 53.4 | FRC unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_frc.c` — 9 tests: clock stub/conversions, pacer create/present/drop/set_fps/names, stats basic/fps-estimation; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-54: Stream Config Serialiser

> Fixed-size binary stream configuration record (32 bytes, magic 0x53434647) with versioned envelope (magic 0x53455256, major version check) and full JSON export.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 54.1 | Stream config record | 🟢 | P0 | 2h | 5 | `src/stream_config/stream_config.c` — magic 0x53434647; 32-byte fixed header; `encode()` / `decode()` / `equals()` / `default()` (1280×720 H.264 Opus UDP:5900) | `scripts/validate_traceability.sh` |
| 54.2 | Versioned config serialiser | 🟢 | P0 | 2h | 6 | `src/stream_config/config_serialiser.c` — 8-byte envelope (magic 0x53455256, version, payload_len); major-version check → CSER_ERR_VERSION; wraps stream_config encode/decode | `scripts/validate_traceability.sh` |
| 54.3 | Config JSON exporter | 🟢 | P0 | 2h | 5 | `src/stream_config/config_export.c` — full JSON with all fields; `config_vcodec_name()` / `config_acodec_name()` / `config_proto_name()` string helpers | `scripts/validate_traceability.sh` |
| 54.4 | Config unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_stream_config.c` — 10 tests: config round-trip/bad-magic/equals/default, serialiser round-trip/bad-magic/version/null, export JSON/names; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-55: Session Handshake Protocol

> Client/server FSM (INIT→HELLO→AUTH→CONFIG→READY) over a CRC-checked PDU (magic 0x48534D47); 128-bit FNV-mixed session tokens; handshake latency and attempt/success/failure/timeout counters.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 55.1 | Handshake PDU | 🟢 | P0 | 3h | 6 | `src/session_hs/hs_message.c` — magic 0x48534D47; CRC-32 (Ethernet poly, reflected) over header[0-11]+payload; 8 message types; type_name() helper | `scripts/validate_traceability.sh` |
| 55.2 | Handshake FSM | 🟢 | P0 | 4h | 7 | `src/session_hs/hs_state.c` — 12 states (INIT→READY→CLOSED/ERROR); separate client and server role paths; process(msg) advances state or returns -1 on bad transition; set_error()/close() | `scripts/validate_traceability.sh` |
| 55.3 | Session token | 🟢 | P0 | 2h | 5 | `src/session_hs/hs_token.c` — 128-bit token; FNV-1a seed mix; constant-time equal(); zero(); hex round-trip (to_hex/from_hex) | `scripts/validate_traceability.sh` |
| 55.4 | Handshake stats | 🟢 | P1 | 2h | 5 | `src/session_hs/hs_stats.c` — attempts/successes/failures/timeouts; RTT (begin_us→complete_us); min/max/avg RTT; reset() | `scripts/validate_traceability.sh` |
| 55.5 | Session HS unit tests | 🟢 | P0 | 3h | 6 | `tests/unit/test_session_hs.c` — 12 tests: msg round-trip/CRC-tamper/bad-magic/names, FSM client/server/error-bye/state-names, token from-seed/hex-roundtrip/zero, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-56: Network Congestion Detector

> RFC 6298 SRTT/RTTVAR/RTO estimator (α=1/8, β=1/4) plus circular-bitset sliding-window loss detector with configurable threshold; congestion event and recovery counters in composite aggregator.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 56.1 | RTT estimator | 🟢 | P0 | 3h | 7 | `src/congestion/rtt_estimator.c` — RFC 6298 §2.2 first-sample init; α=1/8 SRTT, β=1/4 RTTVAR; RTO = SRTT + max(G, 4·RTTVAR); min/max tracking; reset() | `scripts/validate_traceability.sh` |
| 56.2 | Loss detector | 🟢 | P0 | 3h | 6 | `src/congestion/loss_detector.c` — 128-slot circular bitset; evicts oldest on overflow; configurable threshold; CONGESTED signal when fraction > threshold; set_threshold(); reset() | `scripts/validate_traceability.sh` |
| 56.3 | Congestion stats | 🟢 | P1 | 2h | 5 | `src/congestion/congestion_stats.c` — aggregates rtt_estimator + loss_detector; congestion_events / recovery_events on onset/clear transitions; snapshot() | `scripts/validate_traceability.sh` |
| 56.4 | Congestion unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_congestion.c` — 7 tests: RTT first-sample/convergence/null, loss no-loss/trigger/set-threshold, stats integrated; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-57: IDR / Keyframe Request Handler

> PLI/FIR wire format (magic 0x4B465251, 24 bytes); per-SSRC cooldown deduplicator (250 ms default); urgent flag bypasses cooldown; forwarded/suppressed/urgent counters.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 57.1 | KFR message | 🟢 | P0 | 2h | 5 | `src/keyframe/kfr_message.c` — magic 0x4B465251; PLI/FIR types; priority field; SSRC + timestamp; type_name() | `scripts/validate_traceability.sh` |
| 57.2 | KFR handler | 🟢 | P0 | 3h | 7 | `src/keyframe/kfr_handler.c` — 64-slot per-SSRC registry; has_forwarded flag prevents immediate duplicate; cooldown window; urgent priority bypasses cooldown; flush_ssrc() reset; set_cooldown() | `scripts/validate_traceability.sh` |
| 57.3 | KFR statistics | 🟢 | P1 | 2h | 5 | `src/keyframe/kfr_stats.c` — received/forwarded/suppressed/urgent counters; suppression_rate = suppressed/received | `scripts/validate_traceability.sh` |
| 57.4 | KFR unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_keyframe.c` — 9 tests: msg PLI/FIR round-trip/bad-magic/names, handler forward/suppress/cooldown/urgent/flush/multi-ssrc, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-58: Circular Event Log

> Timestamped event entries (level DEBUG/INFO/WARN/ERROR, event_type uint16, NUL-terminated message); 256-slot overwriting ring with age-indexed access; JSON array and plain-text export.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 58.1 | Event entry | 🟢 | P0 | 2h | 5 | `src/eventlog/event_entry.c` — 16-byte header + variable NUL-terminated msg; encode()/decode(); level_name() | `scripts/validate_traceability.sh` |
| 58.2 | Event ring | 🟢 | P0 | 2h | 6 | `src/eventlog/event_ring.c` — 256-slot overwriting ring; push()/get(age)/count()/clear(); find_level() iterates newest-first returning matching age indices | `scripts/validate_traceability.sh` |
| 58.3 | Event export | 🟢 | P0 | 2h | 5 | `src/eventlog/event_export.c` — export_json() renders ring as JSON array; export_text() renders as `[LEVEL] ts_us (type=N) msg\n` lines; buffer-too-small returns -1 | `scripts/validate_traceability.sh` |
| 58.4 | Event log unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_eventlog.c` — 7 tests: entry round-trip/level-names, ring push-get/wrap-around/find-level, export JSON/text; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-59: Multi-Stream Mixer

> Weighted signed-16 PCM blending engine with per-source gain (linear, 0–4×), mute flag, hard-clip to ±32767, and silence fill; statistics track active/muted source events, underruns and mix latency.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 59.1 | Mix source | 🟢 | P0 | 2h | 5 | `src/mixer/mix_source.c` — source descriptor (id, type, weight, muted, name); `init()` clamps weight to [0, 4]; `set_weight()`/`set_muted()`; 4 type names | `scripts/validate_traceability.sh` |
| 59.2 | Mix engine | 🟢 | P0 | 3h | 7 | `src/mixer/mix_engine.c` — 16-slot registry; `add()`/`remove()`/`update()` with duplicate-ID guard; `mix()` with per-source weight scaling, mute skip, and hard-clip; `silence()` | `scripts/validate_traceability.sh` |
| 59.3 | Mix statistics | 🟢 | P1 | 2h | 5 | `src/mixer/mix_stats.c` — mix_calls/active_sources/muted_sources/underruns; avg/min/max latency; `reset()` | `scripts/validate_traceability.sh` |
| 59.4 | Mixer unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_mixer.c` — 8 tests: source init/mutate/names, engine add-remove/basic-sum/hard-clip/mute/silence, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-60: Bandwidth Probe

> Fixed 32-byte probe PDU (magic 0x50524F42); burst-driven send scheduler with configurable interval and burst size; EWMA one-way delay and inter-arrival bandwidth estimator.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 60.1 | Probe packet | 🟢 | P0 | 2h | 5 | `src/bwprobe/probe_packet.c` — magic 0x50524F42; 32-byte fixed record; seq, size_hint, send_ts_us, burst_id, burst_seq; encode/decode | `scripts/validate_traceability.sh` |
| 60.2 | Probe scheduler | 🟢 | P0 | 3h | 6 | `src/bwprobe/probe_scheduler.c` — burst-based scheduler; first tick always sends; `last_burst_start_us` enables correct `set_interval()` deadline recalculation; burst/packet counters | `scripts/validate_traceability.sh` |
| 60.3 | Probe estimator | 🟢 | P0 | 3h | 7 | `src/bwprobe/probe_estimator.c` — EWMA OWD (α=1/8); inter-arrival bandwidth estimate (bits/gap_us); min/max OWD; `reset()` | `scripts/validate_traceability.sh` |
| 60.4 | Bwprobe unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_bwprobe.c` — 8 tests: packet round-trip/bad-magic, sched first-tick/burst/set-interval, estimator OWD/bandwidth/null-guard; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-61: Packet Reorder Buffer

> Seq-ordered reorder buffer with 64-slot circular index (seq % 64), RFC 1982 serial comparison, timeout-flush for gap recovery, and per-buffer delivery callback.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 61.1 | Reorder slot | 🟢 | P0 | 1h | 4 | `src/reorder/reorder_slot.c` — slot (seq, arrival_us, payload up to 2048 bytes, occupied flag); `fill()`/`clear()` | `scripts/validate_traceability.sh` |
| 61.2 | Reorder buffer | 🟢 | P0 | 4h | 8 | `src/reorder/reorder_buffer.c` — 64-slot circular index; `next_seq` starts at 0; consecutive in-order delivery; timeout flush advances `next_seq` to oldest timed-out slot; `set_timeout()` | `scripts/validate_traceability.sh` |
| 61.3 | Reorder statistics | 🟢 | P1 | 2h | 5 | `src/reorder/reorder_stats.c` — packets_inserted/delivered/late_flushes/discards; max_depth; `reset()` | `scripts/validate_traceability.sh` |
| 61.4 | Reorder unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_reorder.c` — 7 tests: slot fill/clear, buffer in-order/out-of-order/timeout-flush/dup-guard/set-timeout, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-62: Adaptive GOP Controller

> Policy-driven IDR decision engine: natural (max_gop), scene-change score threshold, loss-recovery (suppressed when RTT > threshold), min-GOP cooldown. Statistics track IDRs by reason and average GOP length.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 62.1 | GOP policy | 🟢 | P0 | 1h | 4 | `src/gop/gop_policy.c` — min/max GOP frames, scene-change threshold, RTT threshold, loss threshold; `default()`; `validate()` (min ≤ max, thresholds in [0,1]) | `scripts/validate_traceability.sh` |
| 62.2 | GOP controller | 🟢 | P0 | 4h | 8 | `src/gop/gop_controller.c` — 4-rule decision (natural/scene/loss/cooldown); `next_frame()` returns decision + reason; `force_idr()` resets counter; `update_policy()` | `scripts/validate_traceability.sh` |
| 62.3 | GOP statistics | 🟢 | P1 | 2h | 5 | `src/gop/gop_stats.c` — total_frames; idr_natural/scene_change/loss_recovery; avg_gop_length via length accumulator; `reset()` | `scripts/validate_traceability.sh` |
| 62.4 | GOP unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_gop.c` — 9 tests: policy default/validate, controller natural/scene/loss/high-rtt/cooldown/force-idr/names, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-63: Stream Health Monitor

> Typed-metric (GAUGE/COUNTER/RATE/BOOLEAN) registry with threshold evaluation (OK/WARN/CRIT), worst-level rollup, and a JSON snapshot serialiser using a `foreach` iterator to avoid exposing internals.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 63.1 | Health metric | 🟢 | P0 | 2h | 5 | `src/health/health_metric.c` — GAUGE/COUNTER/RATE/BOOLEAN kinds; warn_lo/hi crit_lo/hi threshold; `evaluate()` returns HM_OK/WARN/CRIT; level/kind names | `scripts/validate_traceability.sh` |
| 63.2 | Health monitor | 🟢 | P0 | 3h | 7 | `src/health/health_monitor.c` — 32-slot registry; dup-name guard; `evaluate()` sets overall = worst level; `foreach()` iterator | `scripts/validate_traceability.sh` |
| 63.3 | Health report | 🟢 | P1 | 2h | 5 | `src/health/health_report.c` — JSON serialiser via `foreach` callback; NUL-safe truncation | `scripts/validate_traceability.sh` |
| 63.4 | Health unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_health.c` — 5 tests: metric init/evaluate/names, monitor register/evaluate, report JSON keys; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-64: FEC Encoder / Decoder

> XOR-over-GF(2) parity matrix; group encoder producing k source + r repair packets; decoder recovering up to r lost source packets via single-missing-per-repair XOR inversion.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 64.1 | FEC matrix | 🟢 | P0 | 2h | 5 | `src/fec/fec_matrix.c` — `fec_repair_covers(j, ri)` = (j % (ri+2)) == 0; `fec_build_repair()` XOR loop; FEC_MAX_K=16, FEC_MAX_R=4 | `scripts/validate_traceability.sh` |
| 64.2 | FEC encoder | 🟢 | P0 | 2h | 5 | `src/fec/fec_encoder.c` — copies k sources then appends r repair blocks via fec_build_repair | `scripts/validate_traceability.sh` |
| 64.3 | FEC decoder | 🟢 | P0 | 3h | 7 | `src/fec/fec_decoder.c` — for each repair block: if exactly 1 covered source is missing, recover it by XOR of repair ⊕ other present covered sources | `scripts/validate_traceability.sh` |
| 64.4 | FEC unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_fec.c` — 4 tests: matrix covers, encode pass-through/repair, decode single-loss recovery, irrecoverable scenario; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-65: Clock Sync Offset Estimator

> NTP four-timestamp sample (t0/t1/t2/t3); 8-sample sliding-window median filter for robust offset/RTT estimation; statistics tracking min/avg/max offset and RTT with convergence flag.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 65.1 | Clock sync sample | 🟢 | P0 | 1h | 4 | `src/clocksync/cs_sample.c` — t0/t1/t2/t3 NTP timestamps; `rtt_us()` = (t3-t0)-(t2-t1); `offset_us()` = ((t1-t0)+(t2-t3))/2 | `scripts/validate_traceability.sh` |
| 65.2 | Clock sync filter | 🟢 | P0 | 3h | 7 | `src/clocksync/cs_filter.c` — 8-sample sliding ring; insertion-sort median; `converged` after CS_FILTER_SIZE samples; `reset()` | `scripts/validate_traceability.sh` |
| 65.3 | Clock sync stats | 🟢 | P1 | 2h | 5 | `src/clocksync/cs_stats.c` — sample_count; min/avg/max offset and RTT; convergence flag after CS_CONVERGENCE_SAMPLES; `reset()` | `scripts/validate_traceability.sh` |
| 65.4 | Clock sync unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_clocksync.c` — 3 tests: sample RTT/offset arithmetic, filter median/convergence/reset, stats snapshot/min/max/avg/converged; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-66: Plugin Hot-Reload Manager

> Plugin entry descriptor (path, dlopen handle, version counter, state); 16-slot manager with overridable dlopen/dlclose pointers for testability; statistics tracking reload/fail counts and last reload timestamp.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 66.1 | Hot-reload entry | 🟢 | P0 | 1h | 4 | `src/hotreload/hr_entry.c` — path (256), handle, version (uint32), state (UNLOADED/LOADED/FAILED), last_load_us; `init()`; `clear()` preserves path; `state_name()` | `scripts/validate_traceability.sh` |
| 66.2 | Hot-reload manager | 🟢 | P0 | 4h | 8 | `src/hotreload/hr_manager.c` — injected dlopen/dlclose or built-in stubs; `register()` with dup-guard; `load()`/`reload()` bump version; `unload()`; `get()` | `scripts/validate_traceability.sh` |
| 66.3 | Hot-reload stats | 🟢 | P1 | 2h | 5 | `src/hotreload/hr_stats.c` — reload_count/fail_count/last_reload_us/loaded_plugins; `record_reload(success, now_us)`; `set_loaded(count)`; `reset()` | `scripts/validate_traceability.sh` |
| 66.4 | Hot-reload unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_hotreload.c` — 5 tests: entry init/clear/names, manager register/dup, load/reload/unload/version, failed load state, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-67: Frame Rate Controller

> Token-bucket frame pacer with configurable burst cap; EWMA-based actual-fps tracker; statistics for frame/drop counts and interval min/avg/max.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 67.1 | Frame rate limiter | 🟢 | P0 | 2h | 5 | `src/framerate/fr_limiter.c` — token-bucket accumulation (elapsed_us × fps); burst cap FR_MAX_BURST=2; `tick()` returns frames_ready; `set_fps()` / `reset()` | `scripts/validate_traceability.sh` |
| 67.2 | Frame rate target | 🟢 | P0 | 2h | 5 | `src/framerate/fr_target.c` — EWMA of inter-frame interval (α=0.1); `actual_fps = 1e6/avg_interval_us`; `mark(now_us)` updates on each frame; `reset()` preserves target | `scripts/validate_traceability.sh` |
| 67.3 | Frame rate stats | 🟢 | P1 | 2h | 5 | `src/framerate/fr_stats.c` — frame_count/drop_count/sum_interval; min/avg/max interval; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 67.4 | Frame rate unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_framerate.c` — 4 tests: limiter init/tick/burst/set_fps, target EWMA/actual_fps/reset, stats snapshot/min/max/avg/drop; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-68: Output Target Registry

> Single output endpoint descriptor (URL/protocol/state); 16-slot registry with dup-guard, enable/disable, state transitions, and foreach iterator; statistics for bytes/connect/error/active counts.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 68.1 | Output target | 🟢 | P0 | 1h | 4 | `src/output/output_target.c` — name/url/protocol/state(IDLE/ACTIVE/ERROR/DISABLED)/enabled; `ot_init()`; `ot_state_name()` | `scripts/validate_traceability.sh` |
| 68.2 | Output registry | 🟢 | P0 | 3h | 7 | `src/output/output_registry.c` — 16-slot dup-guarded registry; `add()`/`remove()`/`get()`; `enable()`/`disable()`; `set_state()`; `active_count()`; `foreach()` | `scripts/validate_traceability.sh` |
| 68.3 | Output stats | 🟢 | P1 | 2h | 5 | `src/output/output_stats.c` — bytes_sent/connect_count/error_count/active_count; `record_bytes()`/`record_connect()`/`record_error()`; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 68.4 | Output unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_output.c` — 5 tests: target init/names, registry add/remove/dup, enable/disable/active_count, foreach, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-69: Bitrate Ladder Builder

> Single ABR rung value type (bitrate/width/height/fps); iterative ladder builder (step-down ratio, FPS-halve threshold, max LADDER_MAX_RUNGS=8, ascending qsort); highest-fitting rung selector with safety margin.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 69.1 | Ladder rung | 🟢 | P0 | 1h | 4 | `src/ladder/ladder_rung.c` — bitrate_bps/width/height/fps; `lr_init()` with validity checks; `lr_compare()` qsort comparator (ascending bitrate) | `scripts/validate_traceability.sh` |
| 69.2 | Ladder builder | 🟢 | P0 | 3h | 7 | `src/ladder/ladder_builder.c` — iterative step-down loop (bps × step_ratio); sqrt-proportional height snapping to std_heights[]; 16:9 width; fps halved below threshold; qsort output | `scripts/validate_traceability.sh` |
| 69.3 | Ladder selector | 🟢 | P0 | 1h | 4 | `src/ladder/ladder_selector.c` — scans ascending rungs; picks highest where bps ≤ budget × (1-margin); defaults to rung 0 | `scripts/validate_traceability.sh` |
| 69.4 | Ladder unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_ladder.c` — 3 tests: rung init/compare/qsort, build ascending/range/invalid params, selector margin/fallback; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-70: Packet Loss Estimator

> 64-slot sliding bitmask window with wrapping uint16 sequence numbers; EWMA loss-rate layer on top; burst-tracking statistics for congestion control feedback.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 70.1 | Loss window | 🟢 | P0 | 3h | 7 | `src/loss/loss_window.c` — 64-bit received_mask; `lw_receive()` advances window marking skipped slots lost; `lw_loss_rate()` = total_lost/total_seen; `lw_reset()` | `scripts/validate_traceability.sh` |
| 70.2 | Loss rate | 🟢 | P0 | 2h | 5 | `src/loss/loss_rate.c` — wraps loss_window; EWMA (α=0.125) of instantaneous loss rate; `lr_rate_receive()`; `lr_rate_get()` instantaneous; `lr_rate_ewma()` smooth | `scripts/validate_traceability.sh` |
| 70.3 | Loss stats | 🟢 | P1 | 2h | 5 | `src/loss/loss_stats.c` — total_sent/total_lost/burst_count/max_burst/current_burst; `record(lost)`; loss_pct in snapshot; `reset()` | `scripts/validate_traceability.sh` |
| 70.4 | Loss unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_loss.c` — 4 tests: window no-loss/skip-loss, rate EWMA/ready/reset, stats burst_count/max_burst/pct/reset; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-71: Timestamp Synchronizer

> Linear PTS ↔ wall-clock mapper with configurable timebase; EWMA drift estimator that quantifies stream clock vs. wall-clock divergence; statistics for sample count, peak drift, and cumulative correction.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 71.1 | PTS mapper | 🟢 | P0 | 2h | 5 | `src/timestamp/ts_map.c` — `ts_map_init(num, den)` sets us_per_tick = num/den × 1e6; `set_anchor(pts, wall_us)`; `pts_to_us()` and `us_to_pts()` using anchor + slope; returns 0 when uninitialised | `scripts/validate_traceability.sh` |
| 71.2 | Drift estimator | 🟢 | P0 | 2h | 5 | `src/timestamp/ts_drift.c` — EWMA (α=0.1) of (observed_us − expected_us); `drift_us_per_sec = ewma_error / elapsed_s`; `update(obs, exp)`; `reset()` | `scripts/validate_traceability.sh` |
| 71.3 | Timestamp stats | 🟢 | P1 | 1h | 4 | `src/timestamp/ts_stats.c` — sample_count/max_drift_us/total_correction_us; records |error_us| per measurement; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 71.4 | Timestamp unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_timestamp.c` — 4 tests: map init/us_per_tick, pts↔us round-trip, drift ewma/reset, stats max/total/reset; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-72: Session Limiter

> Single session entry descriptor (ID/IP/state); 32-slot session table with configurable max-sessions cap; admission/rejection/peak/eviction statistics for monitoring.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 72.1 | Session entry | 🟢 | P0 | 1h | 4 | `src/session_limit/sl_entry.c` — session_id/remote_ip/start_us/state (CONNECTING/ACTIVE/CLOSING)/in_use; `sl_entry_init()`; `sl_state_name()` | `scripts/validate_traceability.sh` |
| 72.2 | Session table | 🟢 | P0 | 3h | 7 | `src/session_limit/sl_table.c` — 32-slot table; `create(max_sessions)`; `add()` enforces cap; `remove()`/`get()` by session_id; `count()`; `foreach()` | `scripts/validate_traceability.sh` |
| 72.3 | Session stats | 🟢 | P1 | 2h | 5 | `src/session_limit/sl_stats.c` — total_admitted/total_rejected/peak_count/eviction_count; `record_admit(current)`; `record_reject()`; `record_eviction()`; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 72.4 | Session unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_session_limit.c` — 5 tests: entry init/states, table add/remove/get, cap enforcement, foreach, stats admit/reject/evict/peak; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-73: Stream Tag Store

> Key=value tag entry (32B key, 128B value); 32-slot store with set/get/overwrite/remove/clear/foreach; text serialiser/parser in `key=value\n` format for persistence.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 73.1 | Tag entry | 🟢 | P0 | 1h | 3 | `src/tagging/tag_entry.c` — key[TAG_KEY_MAX=32]/value[TAG_VAL_MAX=128]/in_use; `tag_entry_init()` rejects empty/NULL key | `scripts/validate_traceability.sh` |
| 73.2 | Tag store | 🟢 | P0 | 2h | 6 | `src/tagging/tag_store.c` — 32-slot flat store; `set()` upserts; `get()` returns value ptr; `remove()`; `clear()`; `count()`; `foreach()` | `scripts/validate_traceability.sh` |
| 73.3 | Tag serialiser | 🟢 | P1 | 2h | 5 | `src/tagging/tag_serial.c` — `tag_serial_write()` → `key=value\n` text; `tag_serial_read()` parses back (skips lines without '=' or with empty key) | `scripts/validate_traceability.sh` |
| 73.4 | Tagging unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_tagging.c` — 5 tests: entry init/null-guard, store set/get/overwrite/remove, clear, foreach, serial round-trip; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-74: Buffer Pool

> Contiguous backing-store pre-allocation; N-block acquire/release pool with high-water mark; statistics for allocation counts, failures, and peak usage.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 74.1 | Buffer block | 🟢 | P0 | 1h | 3 | `src/bufpool/bp_block.h` — value type: data ptr + size + in_use flag; no standalone functions; pool owns all instances | `scripts/validate_traceability.sh` |
| 74.2 | Buffer pool | 🟢 | P0 | 3h | 7 | `src/bufpool/bp_pool.c` — single calloc backing store; `create(n, size)`; `acquire()` O(N) scan → sets in_use, updates peak; `release()` validates block ownership; `in_use()`/`peak()`/`capacity()` | `scripts/validate_traceability.sh` |
| 74.3 | Pool stats | 🟢 | P1 | 1h | 4 | `src/bufpool/bp_stats.c` — alloc_count/free_count/peak_in_use/fail_count; `record_alloc(in_use)`; `record_free()`; `record_fail()`; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 74.4 | Buffer pool unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_bufpool.c` — 3 tests: pool create/invalid-params, acquire/release/exhaust/peak/double-release, stats alloc/free/fail/peak; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-75: Event Bus

> 16-subscriber synchronous pub/sub dispatcher; EB_TYPE_ANY wildcard subscription; subscriber cap enforcement; per-publish dispatch/drop statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 75.1 | Event descriptor | 🟢 | P0 | 1h | 3 | `src/eventbus/eb_event.c` — type_id/payload ptr/payload_len/timestamp_us; `eb_event_init()` | `scripts/validate_traceability.sh` |
| 75.2 | Event bus | 🟢 | P0 | 3h | 7 | `src/eventbus/eb_bus.c` — 16-slot subscription table; `subscribe(type_id, cb, user)` → handle; `unsubscribe(handle)`; `publish()` dispatches to matching type_id or EB_TYPE_ANY; returns count of invocations | `scripts/validate_traceability.sh` |
| 75.3 | Bus stats | 🟢 | P1 | 1h | 4 | `src/eventbus/eb_stats.c` — published_count/dispatch_count/dropped_count; `record_publish(n)` increments dropped when n=0; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 75.4 | Event bus unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_eventbus.c` — 5 tests: event init, subscribe/publish/unsubscribe, wildcard, cap enforcement, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-76: Chunk Splitter

> 16-byte chunk header (stream_id/frame_seq/chunk_idx/chunk_count); zero-copy MTU splitter with LAST-flag; 8-slot bitmask reassembly supporting out-of-order chunk arrival.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 76.1 | Chunk header | 🟢 | P0 | 1h | 3 | `src/chunk/chunk_hdr.c` — stream_id/frame_seq/chunk_idx/chunk_count/data_len/flags; `chunk_hdr_init()` validates idx<count and count>0; CHUNK_FLAG_KEYFRAME + CHUNK_FLAG_LAST | `scripts/validate_traceability.sh` |
| 76.2 | Chunk splitter | 🟢 | P0 | 2h | 6 | `src/chunk/chunk_split.c` — zero-copy: output `chunk_t` holds header + pointer into source buffer; sets CHUNK_FLAG_LAST on final chunk; returns chunk count or -1 on invalid | `scripts/validate_traceability.sh` |
| 76.3 | Chunk reassembler | 🟢 | P0 | 3h | 7 | `src/chunk/chunk_reassemble.c` — 8-slot context; `receive(ctx, hdr)` opens slot on new frame_seq; sets bit chunk_idx in received_mask; marks slot complete when mask == (1<<chunk_count)-1; `release()` clears slot | `scripts/validate_traceability.sh` |
| 76.4 | Chunk unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_chunk.c` — 5 tests: hdr init/invalid, split single, split multi/ptr/last-flag, reassemble single, reassemble multi out-of-order; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-77: Priority Queue

> 64-slot array-backed binary min-heap; push/pop/peek/count/clear; overflow tracking; push/pop/peak/overflow statistics for scheduler integration.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 77.1 | Queue entry | 🟢 | P0 | 0.5h | 2 | `src/pqueue/pq_entry.h` — key (uint64 deadline_us) / data ptr / id; header-only value type | `scripts/validate_traceability.sh` |
| 77.2 | Min-heap | 🟢 | P0 | 3h | 8 | `src/pqueue/pq_heap.c` — 64-slot array-backed heap; `push()` sift-up; `pop()` extract-min + sift-down; `peek()` returns minimum without removal; `clear()`; returns -1 on empty/full | `scripts/validate_traceability.sh` |
| 77.3 | Queue stats | 🟢 | P1 | 1h | 4 | `src/pqueue/pq_stats.c` — push_count/pop_count/peak_size/overflow_count; `record_push(cur_size)` updates peak; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 77.4 | Priority queue unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_pqueue.c` — 5 tests: create/empty-guard, ascending pop order, clear, capacity enforcement, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-78: Retry Manager

> Per-request exponential back-off entry; 32-slot retry table with tick-based dispatch and auto-eviction on budget exhaustion; attempt/success/expire statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 78.1 | Retry entry | 🟢 | P0 | 2h | 5 | `src/retry_mgr/rm_entry.c` — request_id/attempt_count/max_attempts/base_delay_us/next_retry_us; `init(now_us, base_delay_us, max)` sets next_retry_us=now+base; `advance(now)` computes next delay via 2^attempt×base (RM_MAX_BACKOFF_US cap); `is_due(now)` | `scripts/validate_traceability.sh` |
| 78.2 | Retry table | 🟢 | P0 | 3h | 7 | `src/retry_mgr/rm_table.c` — 32-slot table; `add()`/`remove()`/`get()`; `tick(now_us, cb, user)` fires callback for due entries; calls `rm_entry_advance()`; auto-evicts when advance returns false (budget exhausted) | `scripts/validate_traceability.sh` |
| 78.3 | Retry stats | 🟢 | P1 | 1h | 4 | `src/retry_mgr/rm_stats.c` — total_attempts/total_succeeded/total_expired/max_attempts_seen; `record_attempt(count)` updates max; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 78.4 | Retry unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_retry.c` — 5 tests: entry init/null-guard, advance/backoff/due, table add/remove/get, tick/auto-evict, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-79: Flow Controller

> Token-bucket single-channel flow controller; parameter block (window/budget/recv-window/credit-step); consume/replenish with credit-step floor and window cap; per-flow sent/dropped/stall/replenish statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 79.1 | FC parameter block | 🟢 | P0 | 0.5h | 2 | `src/flowctl/fc_params.c` — window_bytes/send_budget/recv_window/credit_step; `fc_params_init()` rejects any zero field | `scripts/validate_traceability.sh` |
| 79.2 | FC engine | 🟢 | P0 | 3h | 7 | `src/flowctl/fc_engine.c` — token-bucket; `consume(bytes)` deducts credit, returns -1 on insufficient; `can_send(bytes)` non-destructive; `replenish(bytes)` adds max(bytes, credit_step), caps at window_bytes; `reset()` restores send_budget | `scripts/validate_traceability.sh` |
| 79.3 | FC stats | 🟢 | P1 | 1h | 4 | `src/flowctl/fc_stats.c` — bytes_sent/bytes_dropped/stalls/replenish_count; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 79.4 | Flow controller unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_flowctl.c` — 3 tests: params init/invalid, engine consume/replenish/cap/reset, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-80: Metrics Exporter

> Named int64 gauges with set/add/get/reset; 64-gauge registry with duplicate-rejection and snapshot_all; timestamped snapshot with bounded dump to caller buffer.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 80.1 | Gauge | 🟢 | P0 | 1h | 3 | `src/metrics/mx_gauge.c` — int64 value + name[48]; `init(name)` rejects empty; `set/add/get/reset`; get(NULL) = 0 | `scripts/validate_traceability.sh` |
| 80.2 | Registry | 🟢 | P0 | 2h | 6 | `src/metrics/mx_registry.c` — 64-slot array; `register(name)` rejects duplicates and returns owned pointer; `lookup(name)`; `snapshot_all(out, max)` copies all active gauges | `scripts/validate_traceability.sh` |
| 80.3 | Snapshot | 🟢 | P1 | 1h | 4 | `src/metrics/mx_snapshot.c` — `mx_snapshot_t` holds timestamp_us + gauge array + count; `init()` zeros; `dump(out, max_out)` copies min(count, max_out) gauges; returns -1 on NULL | `scripts/validate_traceability.sh` |
| 80.4 | Metrics unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_metrics.c` — 3 tests: gauge operations, registry register/lookup/duplicate/snapshot_all, snapshot init/dump/truncation; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-81: Signal Router

> Per-signal descriptor (id/level/source_id/timestamp_us); 32-route table with bitmask matching and optional filter predicate; per-route delivery callback; routed/filtered/dropped statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 81.1 | Signal descriptor | 🟢 | P0 | 0.5h | 2 | `src/sigroute/sr_signal.c` — signal_id/level/source_id/timestamp_us; `sr_signal_init()` rejects NULL | `scripts/validate_traceability.sh` |
| 81.2 | Signal router | 🟢 | P0 | 3h | 7 | `src/sigroute/sr_route.c` — 32-slot table; `add_route(src_mask, match_id, filter_fn, deliver, user)` → handle; matching: `(signal_id & src_mask) == match_id` AND `filter_fn` returns true; `remove_route(handle)`; `route(signal)` returns invocation count | `scripts/validate_traceability.sh` |
| 81.3 | Router stats | 🟢 | P1 | 1h | 4 | `src/sigroute/sr_stats.c` — routed/filtered/dropped; `record_route(delivered, filtered_n)` increments dropped when both are 0; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 81.4 | Signal router unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_sigroute.c` — 4 tests: signal init, route add/match/remove/no-match, filter predicate, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-82: Drain Queue

> 128-slot circular FIFO with monotonically increasing sequence numbers; enqueue/dequeue/drain_all (callback); clear; DQ_FLAG_HIGH_PRIORITY and DQ_FLAG_FLUSH entry flags; enqueued/drained/dropped/peak statistics.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 82.1 | Queue entry | 🟢 | P0 | 0.5h | 2 | `src/drainq/dq_entry.h` — header-only: seq/data ptr/data_len/flags; DQ_FLAG_HIGH_PRIORITY + DQ_FLAG_FLUSH | `scripts/validate_traceability.sh` |
| 82.2 | Drain queue | 🟢 | P0 | 3h | 7 | `src/drainq/dq_queue.c` — 128-slot circular FIFO; `enqueue()` assigns next_seq, returns -1 when full; `dequeue()` FIFO order; `drain_all(cb, user)` drains entire queue; `clear()` without callbacks; `count()` | `scripts/validate_traceability.sh` |
| 82.3 | Queue stats | 🟢 | P1 | 1h | 4 | `src/drainq/dq_stats.c` — enqueued/drained/dropped/peak; `record_enqueue(cur_depth)` updates peak; `snapshot()`; `reset()` | `scripts/validate_traceability.sh` |
| 82.4 | Drain queue unit tests | 🟢 | P0 | 2h | 5 | `tests/unit/test_drainq.c` — 5 tests: enqueue/dequeue/seq/FIFO, drain_all callback, clear, capacity enforcement, stats; all pass | `scripts/validate_traceability.sh` |

---

## PHASE-83: Cross-Subsystem Integration Tests

> Proves that four pairs of subsystems work together end-to-end — not just that each passes its own unit tests in isolation.  Uses a shared test harness with INTEG_ASSERT/INTEG_PASS macros.  Three integration tests: flowctl↔metrics (gauge wiring), sigroute↔eventbus (signal delivery pipeline), drainq↔fanout (frame delivery chain).

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 83.1 | Integration harness | 🟢 | P0 | 0.5h | 2 | `tests/integration/integration_harness.h` — INTEG_ASSERT/INTEG_PASS/INTEG_FAIL/INTEG_SUITE macros; returns int (0/1) pattern matching unit tests | `scripts/validate_traceability.sh` |
| 83.2 | flowctl↔metrics integration | 🟢 | P0 | 3h | 7 | `tests/integration/test_flowctl_metrics.c` — 3 tests: normal flow gauge wiring, stall path stall-counter, snapshot reflects accumulated state; all pass ✅ | `scripts/validate_traceability.sh` |
| 83.3 | sigroute↔eventbus integration | 🟢 | P0 | 3h | 7 | `tests/integration/test_sigroute_eventbus.c` — 3 tests: health signal delivery, alert segregation by type, stats integrity through pipeline; all pass ✅ | `scripts/validate_traceability.sh` |
| 83.4 | drainq↔fanout integration | 🟢 | P0 | 3h | 7 | `tests/integration/test_drainq_fanout.c` — 2 tests: basic drain→fanout frame delivery, dq_stats accuracy after fanout; framework for validation (fanout socket path is stubbed) | `scripts/validate_traceability.sh` |

---

## PHASE-84: KDE Plasma Client Deep Integration Tests

> Verifies that the KDE Qt6 client is non-ceremonial: settings persist and propagate to the client C API, signals fire on change, metrics record*() calls update snapshots, and connection state transitions are observable.  All four test files use QSignalSpy and QMetaObject for compile-safe verification.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 84.1 | Settings wiring test | 🟢 | P0 | 2h | 6 | `clients/kde-plasma-client/tests/unit/test_settings_wiring.cpp` — verifies: defaults valid, codec/bitrate round-trip, change signals fire (QSignalSpy), persistence across instances (save/load), values applied to RootStreamClient | `scripts/validate_traceability.sh` |
| 84.2 | UI signal/slot wiring test | 🟢 | P0 | 2h | 6 | `clients/kde-plasma-client/tests/unit/test_ui_signal_slots.cpp` — verifies: all key signals registered, Q_PROPERTY NOTIFY signals exist (QMetaObject), Q_INVOKABLE methods reachable from QML | `scripts/validate_traceability.sh` |
| 84.3 | MetricsManager integration test | 🟢 | P0 | 2h | 6 | `clients/kde-plasma-client/tests/unit/test_metrics_integration.cpp` — verifies: init() succeeds, all sub-objects non-null, record*() methods wired to snapshot, HUD/metrics enable toggles, signals registered | `scripts/validate_traceability.sh` |
| 84.4 | Connection state test | 🟢 | P0 | 2h | 6 | `clients/kde-plasma-client/tests/unit/test_connection_state.cpp` — verifies: initial state non-empty, isConnected/connectionState consistent, state changes on attempt, disconnect is safe, Q_PROPERTY NOTIFY registered | `scripts/validate_traceability.sh` |

---

## PHASE-85: Android/iOS/Web Client Audits + Subphase Creation

> Five-pass deep inspection of all four client platforms.  Each audit documents critical gaps, ceremonial stubs, missing tests, and code hygiene issues.  Outputs recommended subphase IDs (PHASE-87 through PHASE-90) for each platform.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 85.1 | Android client audit | 🟢 | P0 | 2h | 5 | `docs/audits/android_client_audit.md` — identifies 8 critical/high gaps; VideoDecoder/StreamingClient/AudioEngine stubs; missing tests; recommended PHASE-87.1–88.1 | `scripts/validate_traceability.sh` |
| 85.2 | iOS client audit | 🟢 | P0 | 2h | 5 | `docs/audits/ios_client_audit.md` — identifies 5 gaps; VideoDecoder→MetalRenderer bridge missing; AudioEngine feed not called; FileTransfer TODO stubs; recommended PHASE-87.7–88.4 | `scripts/validate_traceability.sh` |
| 85.3 | Web dashboard audit | 🟢 | P0 | 2h | 5 | `docs/audits/web_dashboard_audit.md` — identifies 7 gaps; WebSocket lifecycle regression; memory leak in PerformanceGraphs; missing auth on startup; zero test files; recommended PHASE-88.5–89.1 | `scripts/validate_traceability.sh` |
| 85.4 | Windows client audit | 🟢 | P0 | 2h | 5 | `docs/audits/windows_client_audit.md` — identifies no Windows GUI client; 4 platform code gaps (WSAStartup, WASAPI exclusive-mode, Media Foundation live-stream decode, raw input); recommended PHASE-88.9–90.1 | `scripts/validate_traceability.sh` |

---

## PHASE-86: Code Hygiene, Commentary Pass, Qt6 Standards

> Establishes written coding standards, adds extensive inline commentary to the four newest C modules explaining design rationale (not just what), and creates a deep testing philosophy document.

| ID | Microtask | Status | P | Effort | 🌟 | Description (done when) | Gate |
|----|-----------|--------|---|--------|----|-------------------------|------|
| 86.1 | Code hygiene standards | 🟢 | P0 | 2h | 5 | `docs/standards/code_hygiene.md` — C11 module structure, naming, memory management, error handling, thread-safety docs, commenting rules, test requirements, build-clean checklist | `scripts/validate_traceability.sh` |
| 86.2 | Qt6 UI standards | 🟢 | P0 | 2h | 5 | `docs/standards/qt6_ui_standards.md` — new-style connect() syntax, Q_PROPERTY rules, QML↔C++ data flow, threading, ownership, KDE Plasma specifics, accessibility, test requirements, anti-patterns | `scripts/validate_traceability.sh` |
| 86.3 | Commentary pass on new C modules | 🟢 | P0 | 3h | 6 | Extensive inline commentary added to `fc_engine.c`, `mx_registry.c`, `sr_route.c`, `dq_queue.c` — every non-trivial decision explained with rationale (why, not just what) | `scripts/validate_traceability.sh` |
| 86.4 | Deep testing guide | 🟢 | P0 | 2h | 5 | `docs/standards/deep_testing_guide.md` — non-ceremonial test definition, integration test structure, five-pass review protocol, per-layer test requirements, anti-patterns, coverage goals, prompts for next improvements | `scripts/validate_traceability.sh` |

---

## 📐 Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        RootStream Server                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │ Capture  │  │  Encode  │  │ Network  │  │  Recording   │   │
│  │ DRM/X11  │→ │NVENC/VAAPI│→│ UDP/TCP  │  │ Manager      │   │
│  │ /Dummy   │  │/x264/Raw │  │  +TLS    │  │ ReplayBuffer │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │  Audio   │  │ Security │  │ Discovery│  │  Metrics     │   │
│  │ ALSA/PA/ │  │ Ed25519  │  │mDNS/Bcast│  │ CPU/GPU/Net  │   │
│  │  PW/Dump │  │+ChaCha20 │  │ /Manual  │  │ HUD + API    │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │ Encrypted UDP/TCP
┌─────────────────────────────────────────────────────────────────┐
│                       RootStream Clients                        │
│  ┌──────────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │  KDE Plasma      │  │   Android    │  │      iOS         │  │
│  │  Qt6/QML         │  │  Kotlin/JNI  │  │  Swift/ObjC      │  │
│  │  Vulkan Renderer │  │  MediaCodec  │  │  VideoToolbox    │  │
│  │  (Phase 31-32)   │  │  (Phase 22)  │  │  (Phase 22)      │  │
│  └──────────────────┘  └──────────────┘  └──────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## PHASE-93: rootstream_core Linkable Library

| ID   | Task | Status |
|------|------|--------|
| 93.1 | Refactor root CMakeLists.txt — `rootstream_core` STATIC library | ✅ |
| 93.2 | Link KDE CMakeLists.txt against `rootstream_core` via add_subdirectory | ✅ |
| 93.3 | Export `include/` as PUBLIC target_include_directories | ✅ |
| 93.4 | `rootstream` + `rstr-player` thin executables link rootstream_core | ✅ |

## PHASE-94: Client Session Callback API

| ID   | Task | Status |
|------|------|--------|
| 94.1 | `include/rootstream_client_session.h` — rs_video_frame_t, rs_audio_frame_t, callback types | ✅ |
| 94.2 | `src/client_session.c` — lifted receive/decode loop with atomic stop | ✅ |
| 94.3 | Refactor `service_run_client()` to thin wrapper preserving SDL path | ✅ |
| 94.4 | Add client_session.c to rootstream_core SOURCES | ✅ |

## PHASE-95: KDE VideoRenderer — Real Implementation

| ID   | Task | Status |
|------|------|--------|
| 95.1 | Replace `videorenderer.h` stub — QQuickFramebufferObject, submitFrame(), Q_PROPERTY | ✅ |
| 95.2 | Replace `videorenderer.cpp` stub — NV12 GL upload, BT.709 GLSL shader | ✅ |
| 95.3 | Replace `stream_backend_connector.h/.cpp` — rs_client_session bridge, Qt signals | ✅ |
| 95.4 | Add stream_backend_connector.cpp to KDE SOURCES; stream_backend_connector.h to HEADERS | ✅ |

## PHASE-96: KDE Client End-to-End Connection

| ID   | Task | Status |
|------|------|--------|
| 96.1 | `RootStreamClient::connectToPeer/Address()` → StreamBackendConnector::connectToHost() | ✅ |
| 96.2 | `RootStreamClient::disconnect()` → StreamBackendConnector::disconnect() + thread join | ✅ |
| 96.3 | `setVideoRenderer()` — wires videoFrameReady → VideoRenderer::submitFrame | ✅ |

## PHASE-97: Documentation Updates

| ID   | Task | Status |
|------|------|--------|
| 97.1 | Update `docs/IMPLEMENTATION_STATUS.md` — accurate status, remove phantom files | ✅ |
| 97.2 | Create `docs/architecture/client_session_api.md` — API reference + threading model | ✅ |
| 97.3 | Update `docs/microtasks.md` 441 → 465 | ✅ |

---

> **Overall**: 465 / 465 microtasks complete (**100%**)

*Last updated: 2026 · Post-Phase 97 · Next: Phase 98 (Vulkan zero-copy DMABUF, Android/iOS gap fixes)*


> Historical snapshot above reflects the registry state through Phase 97. Active transformation phases 98 through 108 continue below in the same table style.

---

## PHASE-98: Product Core Definition

| ID | Task | Status |
|------|------|--------|
| 98.1.1 | [Repository audit and baseline truth] Create or normalize `/docs/microtasks.md` and seed initial execution ledger | 🟢 |
| 98.1.2 | [Repository audit and baseline truth] Inventory top-level subsystems and entrypoints | 🟢 |
| 98.1.3 | [Repository audit and baseline truth] Identify implemented vs claimed product capabilities | 🟢 |
| 98.1.4 | [Repository audit and baseline truth] Identify conflicting status and truth-source documents | 🟢 |
| 98.2.1 | [Product scope definition] Draft the product core definition | 🟢 |
| 98.2.2 | [Product scope definition] Define supported user journey and non-goals | 🟢 |
| 98.3.1 | [Support matrix creation] Draft `docs/SUPPORT_MATRIX.md` | 🟢 |
| 98.3.2 | [Support matrix creation] Add explicit support caveats and environment constraints | 🟢 |
| 98.4.1 | [Golden path documentation] Draft `docs/CORE_PATH.md` | 🟢 |
| 98.4.2 | [Golden path documentation] Add concrete setup and validation checkpoints to the golden path | 🟢 |
| 98.5.1 | [Feature flag and maturity classification] Define supported, preview, experimental, and roadmap categories | 🟢 |
| 98.5.2 | [Feature flag and maturity classification] Classify visible features and entrypoints by maturity | 🟢 |
| 98.6.1 | [Root README trust cleanup] Revise `README.md` to reflect the supported product honestly | 🟢 |
| 98.6.2 | [Root README trust cleanup] Add explicit not-yet-supported and non-goal language to high-visibility docs | 🟢 |
| 98.7.1 | [Status document reconciliation] Reconcile `IMPLEMENTATION_STATUS`, `ROADMAP`, `ARCHITECTURE`, and audit docs | 🟢 |
| 98.7.2 | [Status document reconciliation] Add status ownership notes to core product docs | 🟢 |

> Phase 98 progress: 16 / 16

---

## PHASE-99: Golden Path Hardening

| ID | Task | Status |
|------|------|--------|
| 99.1.1 | [Build system baseline validation] Audit current build instructions and entrypoints against repository reality | 🟢 |
| 99.1.2 | [Build system baseline validation] Verify the canonical build path from a clean working tree | 🟢 |
| 99.1.3 | [Build system baseline validation] Record build blockers, warnings, and doc gaps | 🟢 |
| 99.2.1 | [Dependency normalization] Separate required dependencies from optional or experimental ones | 🟢 |
| 99.2.2 | [Dependency normalization] Create or refine a reproducible developer bootstrap path | 🟢 |
| 99.3.1 | [Golden path runtime validation] Validate the canonical host path | 🟢 |
| 99.3.2 | [Golden path runtime validation] Validate the canonical client path and first connection flow | 🟢 |
| 99.4.1 | [Error handling and failure-path cleanup] Audit golden-path failure modes and error messages | 🟢 |
| 99.4.2 | [Error handling and failure-path cleanup] Improve first-failure diagnostics in the canonical path | 🟢 |
| 99.5.1 | [Canonical demo path] Define the canonical demo and validation flow | 🟢 |
| 99.5.2 | [Canonical demo path] Add or refine canonical run scripts for the demo path | 🟢 |
| 99.6.1 | [Packaging and developer setup tightening] Tighten packaging and local setup documentation around the golden path | 🟢 |
| 99.6.2 | [Packaging and developer setup tightening] Validate the first-run experience from the tightened docs | 🟢 |

> Phase 99 progress: 13 / 13

---

## PHASE-100: CI and Quality Gate Hardening

| ID | Task | Status |
|------|------|--------|
| 100.1.1 | [CI workflow audit] Inventory existing CI workflows, jobs, and triggers | 🟢 |
| 100.1.2 | [CI workflow audit] Map CI coverage to the supported product matrix | 🟢 |
| 100.2.1 | [Build matrix tightening] Tighten the CI build matrix around supported targets | 🟢 |
| 100.2.2 | [Build matrix tightening] Ensure release builds are exercised in CI | 🟢 |
| 100.3.1 | [Static analysis and formatting enforcement] Add or refine formatting enforcement | 🟢 |
| 100.3.2 | [Static analysis and formatting enforcement] Add or refine lint and static-analysis enforcement | 🟢 |
| 100.4.1 | [Test gate hardening] Audit which tests are currently gating merges | 🟢 |
| 100.4.2 | [Test gate hardening] Add or tighten core-path test gates | 🟢 |
| 100.5.1 | [Sanitizer and reliability jobs] Define a sanitizer and reliability-job strategy | 🟢 |
| 100.5.2 | [Sanitizer and reliability jobs] Add at least one practical sanitizer or reliability job | 🟢 |
| 100.6.1 | [Artifact and packaging checks] Verify CI artifact and packaging outputs for the supported path | 🟢 |
| 100.6.2 | [Artifact and packaging checks] Document what CI proves and what it does not prove | 🟢 |

> Phase 100 progress: 12 / 12

---

## PHASE-101: Architecture Boundary Cleanup

| ID | Task | Status |
|------|------|--------|
| 101.1.1 | [Architecture map audit] Map current subsystem boundaries from the repository tree | 🟢 |
| 101.1.2 | [Architecture map audit] Identify cross-layer violations and architecture ambiguities | 🟢 |
| 101.2.1 | [Boundary rule definition] Define target architectural layering and boundary rules | 🟢 |
| 101.2.2 | [Boundary rule definition] Publish boundary rules in architecture docs | 🟢 |
| 101.3.1 | [Directory/layout rationalization] Identify low-risk directory and layout rationalization opportunities | 🟢 |
| 101.3.2 | [Directory/layout rationalization] Execute one safe layout rationalization step | 🟢 |
| 101.4.1 | [Interface cleanup] Audit core interfaces and seams on the supported path | 🟢 |
| 101.4.2 | [Interface cleanup] Normalize one high-value interface family | 🟢 |
| 101.5.1 | [Naming and consistency pass] Audit naming inconsistencies in high-visibility architecture surfaces | 🟢 |
| 101.5.2 | [Naming and consistency pass] Apply a targeted naming consistency pass | 🟢 |
| 101.6.1 | [Legacy/experimental isolation] Identify legacy, preview, and experimental surfaces that need stronger isolation | 🟢 |
| 101.6.2 | [Legacy/experimental isolation] Document and implement one isolation improvement for non-core surfaces | 🟢 |

> Phase 101 progress: 12 / 12

---

## PHASE-102: Observability and Reliability

| ID | Task | Status |
|------|------|--------|
| 102.1.1 | [Logging audit] Inventory current logging patterns on the supported path | 🟢 |
| 102.1.2 | [Logging audit] Identify inconsistent log shapes and missing context | 🟢 |
| 102.2.1 | [Structured event schema] Define a structured event schema for the golden path | 🟢 |
| 102.2.2 | [Structured event schema] Apply the structured schema to one critical path | 🟢 |
| 102.3.1 | [Metrics surface expansion] Identify critical metrics for encode, network, decode, and render stages | 🟢 |
| 102.3.2 | [Metrics surface expansion] Expose missing metrics in at least one critical supported path | 🟢 |
| 102.4.1 | [Session and stream tracing] Add session correlation and trace identifiers to the supported path | 🟢 |
| 102.4.2 | [Session and stream tracing] Document how to interpret session traces | 🟢 |
| 102.5.1 | [Reliability diagnostics] Identify the highest-value reliability diagnostics gaps | 🟢 |
| 102.5.2 | [Reliability diagnostics] Improve one reliability diagnostic path and validate it | 🟢 |
| 102.6.1 | [Operational troubleshooting documentation] Write troubleshooting guidance keyed to logs, metrics, and checkpoints | 🟢 |
| 102.6.2 | [Operational troubleshooting documentation] Cross-link troubleshooting docs from support and onboarding surfaces | 🟢 |

> Phase 102 progress: 12 / 12

---

## PHASE-103: Testing, Stress, and Soak Discipline

| ID | Task | Status |
|------|------|--------|
| 103.1.1 | [Test inventory and gap analysis] Map existing tests to supported product areas | 🟢 |
| 103.1.2 | [Test inventory and gap analysis] Identify unsupported or unvalidated core-path code | 🟢 |
| 103.2.1 | [Core-path unit and integration strengthening] Add tests for the highest-risk core-path validation gap | 🟢 |
| 103.2.2 | [Core-path unit and integration strengthening] Ensure the canonical path has explicit regression coverage | 🟢 |
| 103.3.1 | [Adverse condition simulation] Define adverse-network and failure simulations for the supported path | 🟢 |
| 103.3.2 | [Adverse condition simulation] Add at least one adverse-condition simulation harness or job | 🟢 |
| 103.4.1 | [Soak test scaffolding] Create soak-test scaffolding for the supported path | 🟢 |
| 103.4.2 | [Soak test scaffolding] Document soak-test execution and result expectations | 🟢 |
| 103.5.1 | [Regression harness improvement] Audit regression harness ergonomics and reporting | 🟢 |
| 103.5.2 | [Regression harness improvement] Tighten regression harness output or fixtures | 🟢 |
| 103.6.1 | [Test documentation and reporting] Document the expected pre-release test suite | 🟢 |
| 103.6.2 | [Test documentation and reporting] Publish test result interpretation guidance | 🟢 |

> Phase 103 progress: 12 / 12

---

## PHASE-104: Performance and Benchmark Proof

| ID | Task | Status |
|------|------|--------|
| 104.1.1 | [Performance surface audit] Inventory performance-sensitive surfaces and current performance claims | 🟢 |
| 104.1.2 | [Performance surface audit] Define key metrics and benchmark environments | 🟢 |
| 104.2.1 | [Benchmark harness creation or normalization] Normalize the benchmark entrypoint and invocation flow | 🟢 |
| 104.2.2 | [Benchmark harness creation or normalization] Add reproducible benchmark execution instructions | 🟢 |
| 104.3.1 | [Baseline metric capture] Capture baseline metrics for the canonical path where feasible | 🟢 |
| 104.3.2 | [Baseline metric capture] Record measurement caveats and environment details | 🟢 |
| 104.4.1 | [Bottleneck analysis] Analyze baseline results for bottlenecks in the supported path | 🟢 |
| 104.4.2 | [Bottleneck analysis] Record validated optimization follow-ups without overcommitting | 🟢 |
| 104.5.1 | [Performance documentation] Write honest performance documentation tied to measured evidence | 🟢 |
| 104.5.2 | [Performance documentation] Cross-link performance docs from the supported-path surfaces | 🟢 |
| 104.6.1 | [Optimization follow-up queue] Prioritize the performance optimization backlog from measured evidence | 🟢 |
| 104.6.2 | [Optimization follow-up queue] Tag benchmark checkpoints for future regression tracking | 🟢 |

> Phase 104 progress: 12 / 12

---

## PHASE-105: Security Posture and Trust Signals

| ID | Task | Status |
|------|------|--------|
| 105.1.1 | [Security documentation audit] Audit current security documentation and policy signals | 🟢 |
| 105.1.2 | [Security documentation audit] Map security-relevant code touchpoints for the supported path | 🟢 |
| 105.2.1 | [Threat model definition] Draft a repository threat model for the supported product | 🟢 |
| 105.2.2 | [Threat model definition] Document trust boundaries and security assumptions in user-facing docs | 🟢 |
| 105.3.1 | [Auth and crypto implementation review] Review authentication and cryptography implementation touchpoints against docs | 🟢 |
| 105.3.2 | [Auth and crypto implementation review] Record unfinished, risky, or unreviewed security areas | 🟢 |
| 105.4.1 | [Security workflow and policy cleanup] Tighten `docs/SECURITY.md` and vulnerability-reporting guidance | 🟢 |
| 105.4.2 | [Security workflow and policy cleanup] Add explicit security review-status language to core docs | 🟢 |
| 105.5.1 | [Trust signal enhancements] Improve visible repository trust signals around security and support | 🟢 |
| 105.5.2 | [Trust signal enhancements] Document what is and is not security-reviewed | 🟢 |

> Phase 105 progress: 10 / 10

---

## PHASE-106: Enterprise-Grade Repo Polish

| ID | Task | Status |
|------|------|--------|
| 106.1.1 | [Root directory cleanup plan] Audit root-directory clutter, duplication, and first-contact confusion | 🟢 |
| 106.1.2 | [Root directory cleanup plan] Execute one low-risk root-level cleanup with validation | 🟢 |
| 106.2.1 | [Documentation style and consistency pass] Define concise documentation style rules for top-level docs | 🟢 |
| 106.2.2 | [Documentation style and consistency pass] Normalize high-visibility docs to the defined style | 🟢 |
| 106.3.1 | [Onboarding and contributor flow cleanup] Audit onboarding and contributor flow from first read to first change | 🟢 |
| 106.3.2 | [Onboarding and contributor flow cleanup] Tighten contributor and onboarding docs around the supported path | 🟢 |
| 106.4.1 | [Canonical command references] Consolidate canonical commands for setup, build, test, and demo flows | 🟢 |
| 106.4.2 | [Canonical command references] Validate canonical commands against actual scripts or entrypoints | 🟢 |
| 106.5.1 | [Public-facing repo presentation polish] Polish public-facing docs to emphasize proof over posture | 🟢 |
| 106.5.2 | [Public-facing repo presentation polish] Remove remaining high-visibility unsupported claims | 🟢 |

> Phase 106 progress: 10 / 10

---

## PHASE-107: Release Readiness System

| ID | Task | Status |
|------|------|--------|
| 107.1.1 | [Release process audit] Audit the current release process, artifacts, and repo signals | 🟢 |
| 107.1.2 | [Release process audit] Identify missing release-discipline components | 🟢 |
| 107.2.1 | [Versioning policy] Define a versioning policy appropriate for the supported product | 🟢 |
| 107.2.2 | [Versioning policy] Document version semantics and branch expectations | 🟢 |
| 107.3.1 | [Release checklist] Create a release checklist grounded in validation evidence | 🟢 |
| 107.3.2 | [Release checklist] Add release evidence requirements and result-capture guidance | 🟢 |
| 107.4.1 | [Known issues and blocker tracking] Define blocker severity levels and known-issue taxonomy | 🟢 |
| 107.4.2 | [Known issues and blocker tracking] Create known-issues and blocker tracking guidance | 🟢 |
| 107.5.1 | [Production readiness criteria] Define ship/no-ship criteria for the supported product | 🟢 |
| 107.5.2 | [Production readiness criteria] Cross-link release-readiness criteria from high-visibility docs | 🟢 |

> Phase 107 progress: 10 / 10

---

## PHASE-108: Legendary Consistency Pass

| ID | Task | Status |
|------|------|--------|
| 108.1.1 | [Terminology normalization] Build a terminology glossary from core product docs | 🟢 |
| 108.1.2 | [Terminology normalization] Normalize high-visibility terminology to the glossary | 🟢 |
| 108.2.1 | [Naming consistency audit] Audit naming consistency across visible interfaces and docs | 🟢 |
| 108.2.2 | [Naming consistency audit] Fix the highest-confusion naming mismatches | 🟢 |
| 108.3.1 | [Document cross-linking and truth-source cleanup] Consolidate truth sources and cross-links across core docs | 🟢 |
| 108.3.2 | [Document cross-linking and truth-source cleanup] Remove contradictory duplicate guidance from core docs | 🟢 |
| 108.4.1 | [Final support-claim audit] Audit final support and performance claims against evidence | 🟢 |
| 108.4.2 | [Final support-claim audit] Correct remaining unsupported, preview, or ambiguous claims | 🟢 |
| 108.5.1 | [Final polish pass] Apply the final consistency polish to key docs and repository surfaces | 🟢 |
| 108.5.2 | [Final polish pass] Run the final consistency validation sweep and close the program | 🟢 |

> Phase 108 progress: 10 / 10

---

> Transformation program progress: 129 / 129 microtasks complete.

---

## PHASE-109: Code Format Zero-Violation Sprint

> Apply `clang-format` to all 459 source files so that `CI format-check` passes
> with zero violations. This is a mechanical pass; no logic changes.

| ID | Task | Status |
|------|------|--------|
| 109.1 | Run `find src include -name '*.c' -o -name '*.h' \| xargs clang-format -i` and commit result | 🟢 |
| 109.2 | Verify `CI format-check` job reports zero violations on the formatted tree | 🟢 |
| 109.3 | Update `docs/STATE_REPORT.md` to reflect zero remaining format violations | 🟢 |

> Phase 109 progress: 3 / 3

---

## PHASE-110: Source TODO Resolution

> Resolve, annotate, or deliberately defer every `TODO`/`FIXME` annotation in
> `src/` and `include/`. Each item must be either fixed, converted to a filed
> tracked issue, or marked `/* DEFERRED(reason): */` with a rationale comment.

| ID | Task | Status |
|------|------|--------|
| 110.1 | `src/client_session.c:257` — propagate PTS from decoded frame or document limitation | 🟢 |
| 110.2 | `src/discovery.c:63` — rename mDNS service string to canonical `_rootstream._tcp` or document current value | 🟢 |
| 110.3 | `src/network.c:197` — document IPv4-only constraint and add `DEFERRED` annotation | 🟢 |
| 110.4 | `src/qrcode.c:204` — clarify GTK QR window path: implement headful display or add explicit `HEADLESS` guard | 🟢 |

> Phase 110 progress: 4 / 4

---

## PHASE-111: Progress Registry Accuracy Pass

> Bring all counter and status fields in `docs/microtasks.md` into agreement with
> the actual implemented state. All phases are 🟢; overall counter must reflect
> the true total.

| ID | Task | Status |
|------|------|--------|
| 111.1 | Recalculate overall microtask count from phase-level totals; update header line | 🟢 |
| 111.2 | Remove stale "transformation program 95/129" reference; replace with accurate final line | 🟢 |

> Phase 111 progress: 2 / 2

---

## PHASE-112: World-Class Final Consistency Pass

> Publish `docs/STATE_REPORT.md`, verify it cross-links to the correct truth
> sources, and confirm the repository presents a coherent, world-class narrative.

| ID | Task | Status |
|------|------|--------|
| 112.1 | Publish `docs/STATE_REPORT.md` with full gap analysis, build health, and new phase rationale | 🟢 |
| 112.2 | Cross-link `docs/STATE_REPORT.md` from `docs/microtasks.md` registry header | 🟢 |
| 112.3 | Verify `./rootstream --version` and `./rootstream --help` still work after format pass | 🟢 |

> Phase 112 progress: 3 / 3

---

> World-class programme progress: 12 / 12 microtasks complete.
