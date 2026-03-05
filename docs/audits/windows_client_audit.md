# Windows Client Audit — RootStream

> **Generated:** 2026-03 · **Phase:** PHASE-85.4
> **Scope:** Windows-specific code: `src/platform/platform_win32.c`,
>   `src/input_win32.c`, `src/audio_wasapi.c`, `src/decoder_mf.c`
> **Pass count:** 5 (as required by the deep-testing prompt)

---

## Executive Summary

Unlike the Android and iOS clients, RootStream does not have a separate
Windows-native GUI client.  The Windows support consists of platform
abstraction files (Win32 API, Winsock2, WASAPI audio, Media Foundation
video decode) that allow the **server/host** binary to run on Windows.
There is **no Windows streaming-client GUI** equivalent to the KDE Plasma
client.

This creates three action items:

1. The existing Windows platform code has 4 identified gaps (below).
2. A dedicated Windows client (Qt6/Win32 GUI) is **absent** and should
   be planned as a future phase.
3. The WASAPI audio path and Media Foundation decoder have no unit tests.

---

## Gap Inventory (by file)

### 1. `src/platform/platform_win32.c`

| Issue | Severity |
|-------|----------|
| `rs_platform_init()` initialises `perf_freq` but does not call `WSAStartup()`.  Any code that calls Winsock functions before `rs_socket_init()` will fail with `WSANOTINITIALISED`. | 🟠 HIGH |
| `rs_platform_cleanup()` is a no-op — does not call `WSACleanup()`, leaving Winsock resources leaked on process exit. | 🟡 MEDIUM |
| `error_buf` is `__declspec(thread)` (TLS) — fine for multi-threaded use, but the buffer is only 256 bytes.  Long Winsock error strings (e.g., from `FormatMessage`) may be truncated without null-termination. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.9 — Move `WSAStartup`/`WSACleanup` into
`rs_platform_init`/`rs_platform_cleanup`, increase `error_buf` to 512, and
add explicit null-termination after `FormatMessage`.

---

### 2. `src/audio_wasapi.c`

| Issue | Severity |
|-------|----------|
| WASAPI shared-mode exclusive latency path: `IAudioClient::Initialize` is called with `AUDCLNT_SHAREMODE_EXCLUSIVE` but the fallback to `AUDCLNT_SHAREMODE_SHARED` on `AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED` is missing.  On systems where exclusive mode is blocked by another app, audio init silently fails and the caller never knows. | 🟠 HIGH |
| `wasapi_write()` calls `IAudioRenderClient::ReleaseBuffer` but does not handle `AUDCLNT_E_DEVICE_INVALIDATED` (device disconnected).  This causes a hard error on headphone unplug. | 🟠 HIGH |
| No unit tests — WASAPI is a COM interface and is mockable via a thin wrapper, but no wrapper or test exists. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.10 — Add shared-mode fallback, handle
`AUDCLNT_E_DEVICE_INVALIDATED` with a re-initialisation callback, and add
a WASAPI mock wrapper (`IWasapiMock`) for unit tests.

---

### 3. `src/decoder_mf.c` (Media Foundation decoder)

| Issue | Severity |
|-------|----------|
| `MFCreateSourceReaderFromURL` is used for file-based decoding only.  Network stream decoding (e.g., from an H.264 RTP bitstream) requires `MFCreateSinkWriter` + `IMFTransform` pipeline — this is entirely absent. | 🔴 CRITICAL |
| `MFShutdown()` is not called in the error path of `decoder_mf_init()` — if `MFStartup()` succeeds but later initialisation fails, the MF runtime is never shut down. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.11 — Implement `IMFTransform`-based H.264
decode pipeline for live network streams; fix `MFShutdown` error path.

---

### 4. `src/input_win32.c`

| Issue | Severity |
|-------|----------|
| Raw Input registration (`RegisterRawInputDevices`) is correct but `WM_INPUT` message handling does not check `GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT` before calling `GetRawInputData`.  Buffered (`RIM_INPUTSINK`) messages are processed incorrectly. | 🟡 MEDIUM |
| No mouse acceleration (`RAWMOUSE.usFlags & MOUSE_MOVE_ABSOLUTE`) handling — absolute-position mice (e.g., drawing tablets, RDP virtual mouse) are treated as relative-move devices, causing cursor drift. | 🟡 MEDIUM |

**Recommended subphase:** PHASE-88.12 — Add `RIM_INPUT` guard and add
`MOUSE_MOVE_ABSOLUTE` branch in `WM_INPUT` handler.

---

## Missing: Windows Streaming Client GUI

There is no Windows-native or Qt6-on-Windows streaming **client** GUI
(equivalent to the KDE Plasma client).  Options:

| Option | Pros | Cons |
|--------|------|------|
| Port KDE client to Windows (Qt6 is cross-platform) | Reuses all existing C++/QML code | Requires MSVC or MinGW build system setup |
| New Win32/DirectX native client | Best Windows UX, DirectX decode/render | Large scope |
| Electron wrapper around web dashboard | Fastest path | High resource usage |

**Recommended subphase:** PHASE-90.1 — Create `clients/windows-client/`
as a Qt6 CMake project, porting `kde-plasma-client` with WASAPI audio
backend and D3D11/DXGI renderer replacing OpenGL/Vulkan paths.

---

## Missing Test Coverage

- `src/platform/platform_win32.c` — no unit tests (can be mocked on Linux
  with `__attribute__((weak))` stubs for Win32 symbols).
- `src/audio_wasapi.c` — no tests.
- `src/decoder_mf.c` — no tests.

**Recommended subphase:** PHASE-88.13 — Add cross-compile mock tests for
Win32 platform functions using `#ifdef UNIT_TEST` stub overrides.

---

## Priority Order for Implementation

1. `decoder_mf.c` live-stream decode pipeline (PHASE-88.11) — without this Windows host cannot decode incoming streams
2. WASAPI device-invalidated handling (PHASE-88.10) — device disconnect crash
3. `WSAStartup/WSACleanup` in platform init (PHASE-88.9)
4. Windows GUI client planning (PHASE-90.1)
5. Input Win32 raw input fixes (PHASE-88.12)
6. Test coverage (PHASE-88.13)
