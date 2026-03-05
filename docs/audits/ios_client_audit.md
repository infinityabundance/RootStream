# iOS Client Audit — RootStream

> **Generated:** 2026-03 · **Phase:** PHASE-85.2
> **Scope:** All Swift source files under `ios/RootStream/RootStream/`
> **Pass count:** 5 (as required by the deep-testing prompt)

---

## Executive Summary

The iOS client is the most complete of the mobile clients.  It uses
VideoToolbox for hardware H.264/H.265 decoding, Metal for GPU rendering,
and has a real XCTest suite.  However, **5 medium-to-high severity gaps**
remain, and the software decode path (VP9/AV1 via `LibvpxDecoder`) is a
blank-frame stub.  The streaming receive loop does not connect decoded
frames to the Metal renderer, and two TODO stubs prevent file-transfer
and push-notification features from functioning.

---

## Gap Inventory (by file)

### 1. `Rendering/VideoDecoder.swift`

| Issue | Severity |
|-------|----------|
| `LibvpxDecoder.decode()` returns a blank `CVPixelBuffer` — VP9/AV1 frames are never decoded.  Only H.264/H.265 via VideoToolbox is functional. | 🟠 HIGH |
| `VTDecompressionSession` output callback (decompressCallback) is implemented but the decoded `CVPixelBuffer` is not forwarded to `MetalRenderer`.  The pipeline breaks at the VideoToolbox output. | 🔴 CRITICAL |

**Impact:** H.264/H.265 decoding works but the decoded frame is lost
— nothing is rendered.  VP9/AV1 are entirely blank.

**Recommended subphase:** PHASE-87.7 — Connect `VideoDecoder.outputBuffer`
to `MetalRenderer.enqueueFrame()` via a delegate or closure callback.

---

### 2. `Rendering/MetalRenderer.swift`

| Issue | Severity |
|-------|----------|
| `MetalRenderer` has a render loop but no `enqueueFrame(CVPixelBuffer)` entry point called from `VideoDecoder`.  The renderer always shows the last static frame (or blank). | 🔴 CRITICAL |

**Recommended subphase:** See PHASE-87.7 above (same fix).

---

### 3. `Audio/AudioEngine.swift`

| Issue | Severity |
|-------|----------|
| `AudioEngine` uses `AVAudioEngine` with a `sourceNode`.  The source node's render callback uses a live `AVAudioPCMBuffer` but the buffer is only populated if `OpusDecoder.decode()` writes to it.  The bridge from `StreamingClient` → `OpusDecoder` → `AudioEngine.feed()` is not called anywhere. | 🔴 CRITICAL |

**Recommended subphase:** PHASE-87.8 — Add `StreamingClient.audioPacketHandler` closure that calls `AudioEngine.feed(pcmData:)`.

---

### 4. `Transfer/FileTransferManager.swift`

| Line | Issue | Severity |
|------|-------|----------|
| 63 | `_ = packet  // TODO: write packet to streaming connection` — file send is silently discarded. | 🟠 HIGH |
| 90 | `// TODO: read incoming DATA_TRANSFER chunks from streaming connection` — incoming file transfers never received. | 🟠 HIGH |

**Recommended subphase:** PHASE-88.2 — Wire `FileTransferManager` to `StreamingClient.send(packet:)`.

---

### 5. `Notifications/PushNotificationManager.swift`

| Line | Issue | Severity |
|------|-------|----------|
| 36 | `// TODO: send hex to the RootStream server for push targeting` — APNs device token is captured correctly but never registered with the server. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.3 — Add `APIClient.registerPushToken(token:)` call in `PushNotificationManager.didRegisterForRemoteNotifications`.

---

## Existing Test Coverage Assessment

`RootStreamTests.swift` covers:
- ✅ `KeychainManager` — store/load/delete credentials
- ✅ `UserDefaultsManager` — settings persistence round-trip
- ✅ `StreamPacket` — serialisation/deserialisation
- ✅ Basic `StreamingClient` connection lifecycle

**Missing tests:**
- ❌ `VideoDecoder` — no decode test (requires mock data NAL unit)
- ❌ `MetalRenderer` — no render test (requires `MTKView` mock)
- ❌ `AudioEngine` — no PCM feed test
- ❌ `FileTransferManager` — no send/receive test
- ❌ `SensorFusion` — no gyroscope/accelerometer fusion test
- ❌ `InputController` — no touch-to-packet encoding test

**Recommended subphase:** PHASE-88.4 — Expand `RootStreamTests.swift` with
the 6 missing test categories above.

---

## Code Hygiene Observations

- `AppState` uses `@Published` correctly; no retain cycles observed. ✅
- `StreamingClient` uses `async/await` throughout — no callback hell. ✅
- `MetalRenderer` captures `self` strongly in `draw()` closures — review
  for retain-cycle potential. ⚠️
- `OpusDecoder` is `final class` with no inheritance, good for devirtualisation. ✅
- Missing `@MainActor` annotation on UI-update closures in `VideoDecoder`
  output callback — may cause thread-safety warnings on Xcode 16. ⚠️

---

## Priority Order for Implementation

1. `VideoDecoder` → `MetalRenderer` bridge (PHASE-87.7) — without this nothing renders
2. `StreamingClient` → `AudioEngine` bridge (PHASE-87.8)
3. `FileTransferManager` wiring (PHASE-88.2)
4. Expand test coverage (PHASE-88.4)
5. Push notification registration (PHASE-88.3)
