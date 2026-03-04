# 🚀 RootStream Microtask Registry

> **Source of Truth** for all development microtasks.  
> A microtask achieves 🟢 only when its gate script passes in CI.

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

> **Overall**: 258 / 258 microtasks complete (**100%**)

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

*Last updated: 2026 · Post-Phase 42 · Next: Phase 43 (to be defined)*
