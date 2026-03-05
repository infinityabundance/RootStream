# Android Client Audit — RootStream

> **Generated:** 2026-03 · **Phase:** PHASE-85.1
> **Scope:** All Kotlin source files under `android/RootStream/app/src/main/kotlin/`
> **Pass count:** 5 (as required by the deep-testing prompt)

---

## Executive Summary

The Android client is structured correctly using MVVM (ViewModel + StateFlow),
Hilt dependency injection, and Jetpack Compose UI.  However, **12 critical
gaps** were identified where TODO stubs prevent the app from functioning beyond
a UI skeleton.  The app will compile and present a UI, but no actual streaming,
audio, or input data will flow.

---

## Gap Inventory (by file)

### 1. `viewmodel/StreamViewModel.kt`

| Line | Issue | Severity |
|------|-------|----------|
| 35 | `// TODO: Integrate with StreamingClient` — `connect(peerId)` fakes a 1-second delay then sets STREAMING state.  No real `StreamingClient` call is made.  Stats are mock values. | 🔴 CRITICAL |
| 48 | `// TODO: Close streaming client connection` — `disconnect()` sets state to DISCONNECTED without closing any real socket or JNI handle. | 🔴 CRITICAL |

**Impact:** Users see "Connected" and mock FPS/latency stats while no stream is
actually received.  The connection is entirely ceremonial.

**Recommended subphase:** PHASE-87.1 — Wire `StreamViewModel.connect()` to a
real `StreamingClient` JNI/Retrofit call.

---

### 2. `viewmodel/SettingsViewModel.kt`

| Line | Issue | Severity |
|------|-------|----------|
| – | Settings are loaded/saved via `DataStore` but bitrate, codec, and resolution are never applied to `StreamingClient`.  Setting changes do not affect the streaming session. | 🟠 HIGH |

**Impact:** Settings panel is cosmetic — values persist but are ignored.

**Recommended subphase:** PHASE-87.2 — Add `applySettings(client: StreamingClient)` to `SettingsViewModel`.

---

### 3. `rendering/VideoDecoder.kt`

| Line | Issue | Severity |
|------|-------|----------|
| 14 | `TODO Phase 22.2.5: Complete implementation with…` — `MediaCodec` setup is incomplete. | 🔴 CRITICAL |
| 39 | `// TODO: Create and configure MediaCodec` — decoder is never created; `decode(data)` is a no-op. | 🔴 CRITICAL |
| 58 | `// TODO: Queue input buffer` | 🔴 CRITICAL |
| 65 | `// TODO: Release output buffer to surface` | 🔴 CRITICAL |
| 77 | `// TODO: Query MediaCodecList for supported codecs` | 🟠 HIGH |

**Impact:** Video decoding does not happen.  The screen is perpetually blank
even when bytes arrive from the network.

**Recommended subphase:** PHASE-87.3 — Implement `VideoDecoder` using
`MediaCodec` async API with a `Surface` output target.

---

### 4. `rendering/OpenGLRenderer.kt` / `VulkanRenderer.kt`

| Issue | Severity |
|-------|----------|
| Both renderers contain stub `render(frame)` methods that accept a frame parameter but never upload texture data.  OpenGL path calls `glClear()` only. | 🔴 CRITICAL |
| Vulkan renderer has `TODO: submit command buffer` comment — command recording is absent. | 🔴 CRITICAL |

**Recommended subphase:** PHASE-87.4 — Implement OpenGL frame upload and
Vulkan command buffer recording.

---

### 5. `audio/AudioEngine.kt` / `audio/OpusDecoder.kt`

| Issue | Severity |
|-------|----------|
| `AudioEngine.startPlayback()` creates an `AudioTrack` but never feeds it data. | 🔴 CRITICAL |
| `OpusDecoder.decode()` has `// TODO: Decode Opus packet` — calls `opusDecode()` JNI stub that returns null. | 🔴 CRITICAL |

**Recommended subphase:** PHASE-87.5 — Wire `OpusDecoder` output to `AudioTrack.write()`.

---

### 6. `network/StreamingClient.kt`

| Issue | Severity |
|-------|----------|
| `connect()` opens a TCP socket and reads bytes into a `ByteArray` but does not parse packet headers or dispatch frames to `VideoDecoder` / `AudioEngine`. | 🟠 HIGH |
| `disconnect()` closes socket but does not stop coroutines, risking `CancellationException` leak. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-87.6 — Implement packet parsing and
dispatcher routing (video → `VideoDecoder`, audio → `AudioEngine`).

---

### 7. `input/InputController.kt`

| Issue | Severity |
|-------|----------|
| Touch events captured correctly but `sendInputEvent()` has `// TODO` for serialisation — input is never transmitted over the network. | 🟠 HIGH |

---

### 8. `transfer/FileTransferManager.kt`

| Issue | Severity |
|-------|----------|
| `sendFile()` reads file bytes but TODO prevents writing to streaming connection. | 🟡 MEDIUM |
| `receiveFile()` never called from `StreamingClient`; incoming data chunks are silently discarded. | 🟡 MEDIUM |

---

## Missing Test Coverage

- No unit tests exist under `app/src/test/` or `app/src/androidTest/`.
- Recommended: Add ViewModel unit tests using `TestCoroutineDispatcher` and
  `FakeStreamingClient`.
- Recommended: Add Compose UI tests using `createComposeRule()` verifying
  StreamScreen state transitions.

**Recommended subphase:** PHASE-88.1 — Create Android test target with
`StreamViewModelTest`, `SettingsViewModelTest`, and `VideoDecoderTest`.

---

## Code Hygiene Observations

- All ViewModels use `StateFlow` correctly (no mutable exposure). ✅
- Hilt DI module bindings are present but stub implementations are bound
  (e.g., `FakeStreamingClient` bound in `NetworkModule`). ⚠️
- No `ProGuard`/`R8` rules provided — release builds may strip JNI method
  names. ⚠️

---

## Priority Order for Implementation

1. `VideoDecoder.kt` — without this nothing is visible (PHASE-87.3)
2. `StreamingClient.kt` packet dispatch (PHASE-87.6)
3. `AudioEngine.kt` + `OpusDecoder.kt` (PHASE-87.5)
4. `StreamViewModel` real connection (PHASE-87.1)
5. `SettingsViewModel` apply-to-client (PHASE-87.2)
6. `InputController` serialisation (PHASE-87.6)
7. Tests (PHASE-88.1)
