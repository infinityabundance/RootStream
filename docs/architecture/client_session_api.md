# Client Session API — Architecture Reference

## Overview

`rootstream_client_session.h` (PHASE-94) defines the callback-based client
session API that decouples the streaming backend from the display layer.

---

## Problem Before PHASE-94

```
service_run_client()          ← all logic in one function
  ├── decoder init
  ├── SDL2 display init        ← welded to SDL
  ├── audio backend init
  └── receive loop
        ├── rootstream_net_recv()
        ├── rootstream_decode_frame()
        └── display_present_frame()  ← welded to SDL
```

KDE client could not reuse this pipeline:
- SDL2 display calls are not compatible with Qt Quick rendering
- The loop is synchronous — it blocks the calling thread forever
- There was no way to inject a different display backend

---

## Solution: Callback-Based Session API

```
rs_client_session_create(cfg)
rs_client_session_set_video_callback(session, my_on_video_fn, my_ctx)
rs_client_session_set_audio_callback(session, my_on_audio_fn, my_ctx)
rs_client_session_run(session)   ← blocking; call on a worker thread
rs_client_session_destroy(session)
```

The session owns all backend logic (network, crypto, reassembly, decode).  
The caller owns display/audio.

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         rootstream_core                              │
│                                                                      │
│  src/network.c → src/crypto.c → src/packet_validate.c               │
│       → src/vaapi_decoder.c (Linux)                                  │
│       → src/decoder_mf.c (Windows)                                   │
│                   ↓                                                  │
│         src/client_session.c                                         │
│            rs_client_session_t                                       │
│               on_video_fn ──────────────────────────────────────┐   │
│               on_audio_fn ──────────────────────────────────┐   │   │
│               on_state_fn                                   │   │   │
└─────────────────────────────────────────────────────────────│───│───┘
                                                              │   │
           ┌──────────────────────────────────────────────────┘   │
           ↓                                                        │
┌──────────────────────────────┐  ┌───────────────────────────────┐
│    SDL2 CLI path             │  │    KDE Plasma client          │
│                              │  │                               │
│    sdl_on_video_frame()      │  │    StreamBackendConnector     │
│      → display_present_frame │  │      cVideoCallback()         │
│                              │  │        → copy NV12            │
│    sdl_on_audio_frame()      │  │        → emit videoFrameReady │
│      → audio_playback_fn     │  │                   ↓           │
│                              │  │        VideoRenderer          │
│    (service.c wrapper)       │  │          submitFrame()        │
│                              │  │          → GL upload + draw   │
└──────────────────────────────┘  └───────────────────────────────┘
```

---

## API Reference

### `rs_client_config_t`

| Field           | Type          | Description                          |
|----------------|---------------|--------------------------------------|
| `peer_host`    | `const char*` | Peer hostname or IP (caller-owned)   |
| `peer_port`    | `int`         | Peer port number                     |
| `audio_enabled`| `bool`        | Enable audio decode + callback        |
| `low_latency`  | `bool`        | Request low-latency decode mode      |

**String lifetime**: `peer_host` and `peer_code` are NOT copied by
`rs_client_session_create()`.  The strings must remain valid until
`rs_client_session_destroy()` is called.  Use local storage (stack or
`QByteArray` member) to guarantee lifetime.

---

### `rs_video_frame_t`

| Field        | Type             | Description                                    |
|-------------|------------------|------------------------------------------------|
| `width`     | `int`            | Frame width in pixels                          |
| `height`    | `int`            | Frame height in pixels                         |
| `pixfmt`    | `rs_pixfmt_t`    | `RS_PIXFMT_NV12` or `RS_PIXFMT_RGBA`          |
| `plane0`    | `const uint8_t*` | Y luma plane (NV12) or RGBA data               |
| `plane1`    | `const uint8_t*` | UV chroma plane (NV12), NULL for RGBA          |
| `stride0`   | `int`            | Bytes per row for plane0                       |
| `stride1`   | `int`            | Bytes per row for plane1 (0 if NULL)           |
| `pts_us`    | `uint64_t`       | Presentation timestamp in microseconds         |
| `is_keyframe`| `bool`          | True if this is an intra-coded frame           |

**Lifetime**: valid ONLY for the duration of the `on_video` callback.  
**Always copy data before returning from the callback.**

---

### Threading Model

```
Thread A (GUI/owner):
  rs_client_session_create()
  rs_client_session_set_video_callback()
  → spawns Thread B (session worker)
  rs_client_session_request_stop()  ← thread-safe (atomic store)

Thread B (session worker):
  rs_client_session_run()           ← blocks here
    recv packets → decrypt → decode
    on_video(user, &frame)          ← callback fires here
    on_audio(user, &frame)          ← callback fires here
  returns when stop_requested or peer disconnects

Thread C (Qt render thread):
  VideoRenderer::synchronize()      ← reads pending_frame_ (QFBO protocol)
  VideoRenderer::VideoRendererGL::render()
```

---

## Frame Flow: SDL2 Path

```
rs_client_session_run()
  → decode to NV12 → on_video → sdl_on_video_frame()
       → frame_buffer_t bridge
       → display_present_frame()
           → SDL_UpdateYUVTexture()
           → SDL_RenderCopy()
           → SDL_RenderPresent()
```

## Frame Flow: KDE/Qt Path

```
rs_client_session_run()                  [session worker thread]
  → decode to NV12 → on_video → cVideoCallback()
       → memcpy NV12 into QByteArray
       → emit videoFrameReady(data, w, h)   [Qt::QueuedConnection]
                                            ↓ [GUI thread]
       VideoRenderer::submitFrame(data, w, h)
           → stores pending_frame_
           → calls update()                [schedules render]
                                            ↓ [render thread]
       VideoRendererGL::synchronize()
           → copies pending_frame_
       VideoRendererGL::render()
           → uploadNV12() → GL texture upload
           → drawQuad()   → shader (NV12→RGB BT.709)
```

---

## Upgrade Path: Zero-Copy DMABUF

The current MVP does a CPU memcpy in `cVideoCallback()`.  To eliminate it:

1. VA-API decoder exports a DMABUF file descriptor per frame.
2. `cVideoCallback()` receives the FD instead of a pixel pointer.
3. `VideoRendererGL` imports the FD via `EGL_EXT_image_dma_buf_import`.
4. The NV12 texture is created directly from the DMABUF — zero CPU copy.

This requires:
- `EGL_EXT_image_dma_buf_import` (available on Mesa 3.1+)
- VA-API decoder modified to export DMABUF FDs
- VideoRendererGL modified to use `eglCreateImageKHR` + `glEGLImageTargetTexture2DOES`

Estimated latency improvement: ~3–8ms at 4K resolution (eliminates ~6MB/frame memcpy).

---

## File Index

| File | Purpose |
|------|---------|
| `include/rootstream_client_session.h` | Public session API |
| `src/client_session.c` | Session implementation (lifted from service.c) |
| `src/service.c::service_run_client()` | Thin SDL wrapper over session API |
| `clients/kde-plasma-client/src/stream_backend_connector.h/.cpp` | Qt↔C bridge |
| `clients/kde-plasma-client/src/videorenderer.h/.cpp` | QQuickFBO NV12 renderer |
| `clients/kde-plasma-client/src/rootstreamclient.h/.cpp` | QML-facing client object |
