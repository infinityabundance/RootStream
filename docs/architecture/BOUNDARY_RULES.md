# RootStream Architecture Boundary Rules

This document defines the target architectural layering for RootStream,
the rules for crossing subsystem boundaries, and the isolation policy for
non-core surfaces.

It is a companion to [`docs/ARCHITECTURE.md`](../ARCHITECTURE.md) (which
describes subsystem components) and
[`docs/SUPPORT_MATRIX.md`](../SUPPORT_MATRIX.md) (which defines surface
maturity).

---

## Architectural Layers

RootStream is organized into three layers:

```
┌─────────────────────────────────────────────────────┐
│  PRESENTATION LAYER                                   │
│  GTK tray · KDE client · Web dashboard · CLI         │
│  Android app · iOS app                               │
└──────────────────┬──────────────────────────────────┘
                   │  rs_client_session API / vtable callbacks
┌──────────────────▼──────────────────────────────────┐
│  SESSION / PROTOCOL LAYER                             │
│  client_session.c · service.c · crypto.c             │
│  packet_validate.c · session_hs/ · session/          │
│  network.c · network_tcp.c · network_reconnect.c     │
└──────────────────┬──────────────────────────────────┘
                   │  backend vtable / init_fn / capture_fn
┌──────────────────▼──────────────────────────────────┐
│  BACKEND / HARDWARE LAYER                             │
│  drm_capture.c · x11_capture.c · dummy_capture.c    │
│  vaapi_encoder.c · nvenc_encoder.c · ffmpeg_encoder  │
│  audio_capture_* · audio_playback_*                  │
│  display_sdl2.c · discovery.*                        │
└─────────────────────────────────────────────────────┘
```

---

## Boundary Rules

### Rule B-1: Presentation → Session

- **Allowed**: The presentation layer may call into the session layer
  **only through the public API** defined in
  `include/rootstream_client_session.h` (`rs_client_session_*` functions).
- **Prohibited**: Presentation code must **not** directly call backend
  functions (e.g., `audio_playback_write_alsa`, `capture_drm_frame`).
- **Rationale**: The KDE client, Android app, and web backend all share
  the same session layer without needing to know which audio or capture
  backend is active.

### Rule B-2: Session → Backend

- **Allowed**: Session code uses **vtable pointers** stored in
  `rootstream_ctx_t` (`capture_backend`, `encoder_backend`,
  `audio_capture_backend`, `audio_playback_backend`) to dispatch to
  the selected backend.
- **Allowed**: Session code may call typed vtable functions like
  `ctx->capture_backend->capture_fn(ctx, &frame)`.
- **Prohibited**: Session code must **not** call backend functions
  directly by name (i.e., no `capture_drm_frame(ctx, &frame)` in
  `service.c`).
- **Rationale**: Vtable dispatch enables fallback selection and testing
  with stub backends.

### Rule B-3: Backend isolation

- **Each backend file** (`drm_capture.c`, `audio_playback_alsa.c`, etc.)
  owns its own internal state via `void *` pointers inside
  `rootstream_ctx_t`.
- **Backend files must not** directly include or call into sibling
  backend files.
- **Backend files may** include `include/rootstream.h` for the shared
  context type.

### Rule B-4: Platform abstraction

- Platform-specific code (Linux vs Windows) belongs in
  `src/platform/platform_linux.c` and `src/platform/platform_win32.c`.
- Session and backend code includes `src/platform/platform.h` and
  uses `RS_PLATFORM_*` macros rather than `#ifdef __linux__` or
  `#ifdef _WIN32` inline.

### Rule B-5: Non-core surface isolation

- Code for **Experimental** surfaces (web backend, VR, mobile) lives
  in its own subdirectory and is guarded by a CMake `option()`:
  `BUILD_WEB_DASHBOARD`, `BUILD_VR_SUPPORT`.
- These surfaces **must not** introduce unconditional dependencies on
  the root binary; they should be additive.
- The root `make` and `cmake` default builds must succeed without
  enabling non-core options.

---

## Interface Contract Reference

| Interface | Defined in | Consumer |
|---|---|---|
| Capture backend vtable | `include/rootstream.h` `capture_backend_t` | `service.c`, `core.c` |
| Audio capture backend | `include/rootstream.h` `audio_capture_backend_t` | `service.c` |
| Audio playback backend | `include/rootstream.h` `audio_playback_backend_t` | `service.c`, `network.c` |
| Encoder backend vtable | `include/rootstream.h` `encoder_backend_t` | `service.c` |
| Client session API | `include/rootstream_client_session.h` | KDE client, service.c |
| Platform abstraction | `src/platform/platform.h` | All platform-sensitive code |

---

## Naming Conventions

| Pattern | Convention | Example |
|---|---|---|
| Backend init function | `<backend>_init_<name>` | `audio_playback_init_alsa` |
| Backend write/capture fn | `<backend>_<verb>_<name>` | `audio_playback_write_alsa` |
| Backend availability fn | `<backend>_<name>_available` | `audio_playback_alsa_available` |
| Backend cleanup fn | `<backend>_cleanup_<name>` | `audio_playback_cleanup_alsa` |
| Public session API fn | `rs_client_session_<verb>` | `rs_client_session_create` |
| Context type | `<subsystem>_ctx_t` | `encoder_ctx_t` |
| Type definitions | `<noun>_t` suffix | `frame_buffer_t`, `peer_t` |

---

## Directory Layout Rationale

```
src/
  *.c                     — root-level runtime: main, service, core, crypto, network
  audio/                  — (KDE client only, not root binary)
  codec/                  — codec subsystem (if extracted)
  network/                — network optimization subsystem
  platform/               — platform abstraction layer
  security/               — optional security subsystem
  session/                — session persistence
  session_hs/             — handshake protocol
  session_limit/          — session limiting
  vr/                     — experimental VR (BUILD_VR_SUPPORT)
  web/                    — experimental web API (BUILD_WEB_DASHBOARD)
  metrics/                — metrics collection
  ...

include/
  rootstream.h            — main public header: context, backends, API
  rootstream_client_session.h — client session API (Phase 94)

clients/kde-plasma-client/ — KDE client subtree (separate CMake)
android/                   — Android client subtree
ios/                       — iOS client subtree
frontend/                  — React web dashboard
infrastructure/            — Terraform, Docker, K8s (cloud/ops)
```

---

## Known Violations and Remediation

| File | Violation | Severity | Status |
|---|---|---|---|
| `src/service.c` | References `ctx->tray.menu` for ALSA/Pulse context storage (should use dedicated struct) | Medium | Tracked in Phase 102 |
| `src/network.c` | Direct call to `audio_playback_backend->playback_fn` bypasses session layer | Low | Acceptable for now; tracked in Phase 103 |
| `include/rootstream.h` | Dual-defined `rootstream_ctx_t` typedef forward decl | Low | Legacy; will resolve in Phase 108 |

---

See [`docs/ARCHITECTURE.md`](../ARCHITECTURE.md) for component descriptions
and [`docs/SUPPORT_MATRIX.md`](../SUPPORT_MATRIX.md) for surface maturity.
