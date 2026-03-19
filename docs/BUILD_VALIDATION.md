# RootStream Build Validation

This document records the outcome of Phase 99.1 — build system baseline validation
against the canonical Linux-first product path.

Use this document alongside:

- Supported product definition: [`docs/PRODUCT_CORE.md`](PRODUCT_CORE.md)
- Step-by-step flow: [`docs/CORE_PATH.md`](CORE_PATH.md)
- Build quick-start: [`docs/QUICKSTART.md`](QUICKSTART.md)

---

## Canonical Build Path

The supported build uses the root `Makefile` on a Linux host.  CMake is also
present and works, but `make` is the primary developer entrypoint.

```bash
# Minimal required dependencies (Ubuntu / Debian)
sudo apt-get install -y \
    build-essential pkg-config \
    libdrm-dev libva-dev \
    libsodium-dev libopus-dev \
    libasound2-dev libsdl2-dev \
    libqrencode-dev libpng-dev \
    libx11-dev

# Build the native binary
make

# Headless build (no GTK3 system-tray; safe in CI and minimal installs)
make HEADLESS=1

# Debug build
make DEBUG=1 HEADLESS=1
```

## Required vs Optional Dependencies

### Required

| Dependency | pkg-config name | Why |
|---|---|---|
| build-essential / gcc | — | C11 compiler |
| libdrm | libdrm | DRM/KMS display capture |
| libsodium | libsodium | Cryptographic primitives (Ed25519, ChaCha20) |
| libopus | opus | Audio codec |
| ALSA | alsa | Audio capture/playback |
| SDL2 | sdl2 | Client-side display |
| libqrencode | libqrencode | QR pairing code generation |
| libpng | libpng | PNG support for QR output |

### Optional (graceful degradation)

| Dependency | pkg-config name | Effect when absent |
|---|---|---|
| GTK3 | gtk+-3.0 | No system tray; use `HEADLESS=1` |
| VA-API | libva libva-drm | No hardware encode; software fallback used |
| Avahi | avahi-client | No mDNS discovery; broadcast/manual fallback |
| PipeWire | libpipewire-0.3 | PipeWire audio backend disabled |
| PulseAudio | (built-in) | PulseAudio backend disabled |
| X11 | x11 | X11 capture backend disabled |
| ncurses | ncurses | No TUI; CLI-only fallback |
| FFmpeg | libavformat etc | No FFmpeg software encoder |
| NVENC | (manual) | No NVIDIA hardware encode |

## Conditional Build Flags

| Flag | Description |
|---|---|
| `HEADLESS=1` | Disable GTK3 system tray; uses stub tray backend |
| `DEBUG=1` | Enable debug symbols, disable optimizations |
| `NO_CRYPTO=1` | Disable encryption (non-functional; testing only) |
| `NO_QR=1` | Disable QR code generation |
| `NO_DRM=1` | Disable DRM/KMS capture |

## Build Validation Results (Phase 99.1)

The following was validated on Ubuntu 24.04 with the available dependencies:

| Check | Result | Notes |
|---|---|---|
| `make HEADLESS=1` | ✅ Pass | Clean build with all available deps |
| `./rootstream --help` | ✅ Pass | Binary runs and shows help |
| `make test-build` | ✅ Pass | Unit test binaries compile |
| `./tests/unit/test_crypto` | ✅ Pass | 10/10 crypto tests pass |
| `./tests/unit/test_encoding` | ✅ Pass | 18/18 encoding tests pass |
| CMake `cmake -B build -S .` | ✅ Pass | CMake configures successfully |

## Known Build Blockers Fixed (Phase 99.1)

The following pre-existing compilation errors were fixed as part of this validation:

| File | Issue | Fix |
|---|---|---|
| `include/rootstream_client_session.h:27` | `/*` inside block comment caused `-Werror=comment` | Changed `decoder/*.c` to `decoder/...c` |
| `include/rootstream.h` | `playback_fn` typedef used `int16_t *` but all PipeWire implementations used `const int16_t *` | Updated all playback function signatures to `const int16_t *` |
| `include/rootstream.h` | `FRAME_FORMAT_NV12` and `FRAME_FORMAT_RGBA` constants missing | Added `FRAME_FORMAT_*` defines alongside `frame_buffer_t` |
| `include/rootstream.h` | `rootstream_ctx_t` missing `peer_host` and `peer_port` fields | Added fields; `peer_host` is a `char[256]` buffer |
| `include/rootstream.h` | `settings_t` missing `audio_channels` and `audio_sample_rate` | Added both fields |
| `include/rootstream.h` | `rootstream_ctx_t` missing `current_audio` buffer | Added anonymous struct with `data`, `size`, `capacity` |
| `src/ai_logging.c:23` | Unused `ctx` parameter caused `-Werror=unused-parameter` | Added `(void)ctx` guard |
| `Makefile` | `src/client_session.c` missing from `SRCS` | Added to source list |
| `Makefile` | `src/audio_capture_pipewire.c` and `src/audio_playback_pipewire.c` missing from `SRCS` | Added; both have `#else` stubs for non-PipeWire builds |
| `Makefile` | PipeWire not detected or linked | Added `PIPEWIRE_FOUND` detection block with `HAVE_PIPEWIRE` define |
| `src/client_session.c:310` | `rootstream_opus_decode` last arg should be `size_t *` not a value | Fixed call to use `&pcm_len` |

## Remaining Known Gaps

- Opus audio not available in the current test environment (linker test only; runtime audio path needs full Opus install).
- The integration test (`tests/integration/test_stream.sh`) exercises CLI startup, identity generation, and QR output but does not yet prove a full sustained client render path (see CORE_PATH.md limitations).
- GTK3 system tray requires `libgtk-3-dev`; `HEADLESS=1` is the supported build path for CI and server deployments.

## Verifying Your Build

After building, run:

```bash
# 1. Binary exists and runs
./rootstream --version

# 2. Identity generation
XDG_CONFIG_HOME=/tmp/rootstream-test ./rootstream --qr

# 3. Unit tests
make test-build && ./tests/unit/test_crypto && ./tests/unit/test_encoding
```

See [`docs/CORE_PATH.md`](CORE_PATH.md) for the full canonical validation sequence.
