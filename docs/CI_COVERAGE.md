# RootStream CI Coverage

This document describes what the CI pipeline proves, what it does not prove,
and how CI maps to the supported product matrix in
[`docs/SUPPORT_MATRIX.md`](SUPPORT_MATRIX.md).

---

## CI Jobs and What They Prove

| Job | Trigger | What it validates | What it does NOT validate |
|---|---|---|---|
| `build` (release, debug, headless) | push/PR | The Linux native binary compiles with GTK, without GTK, and with debug flags on a clean Ubuntu machine | Runtime streaming; hardware paths (VA-API, DRM, NVENC); multi-machine scenarios |
| `unit-tests` | push/PR | Crypto (Ed25519, ChaCha20) and encoding (NAL parsing, frame buffer, control packet) unit behavior | Audio, network, display, or integration behavior |
| `integration-tests` | push/PR | The CLI startup, identity generation, QR output, host startup, and loopback setup as exercised by `tests/integration/test_stream.sh` | Full sustained two-machine client render path |
| `format-check` | push/PR | All `src/` and `include/` `.c`/`.h` files conform to `.clang-format` | Logical correctness or behavior |
| `code-quality` | push/PR | cppcheck static analysis (informational); absence of raw `strcpy`/`sprintf`/`gets` | All security or correctness issues |
| `sanitizer` | push/PR | Unit tests pass cleanly under GCC AddressSanitizer + UBSanitizer (no memory errors, no undefined behavior in hot paths) | Full streaming path under sanitizer; kernel or hardware paths |
| `memory-check` | push/PR | Unit tests pass with no definite memory leaks under valgrind | Full streaming path; optional-dependency paths |
| `windows-build` | push/PR | The Windows client CMake build compiles without errors | Windows runtime; KDE/Linux paths |
| `cmake-linux-build` | push/PR | The CMake Linux build compiles and tests pass via ctest | Makefile build; runtime streaming |

## What CI Covers Today

- ✅ Canonical Linux native binary builds (GTK, headless, debug variants)
- ✅ Crypto unit tests (Ed25519 keypair, ChaCha20-Poly1305 encrypt/decrypt)
- ✅ Encoding unit tests (H.264/H.265 NAL parsing, frame buffer, control packets)
- ✅ CLI startup, help, and version output
- ✅ Identity generation and QR output (`--qr`)
- ✅ Host startup and network initialization
- ✅ Code formatting (clang-format)
- ✅ Static analysis (cppcheck)
- ✅ Memory safety (ASan/UBSan and valgrind on unit test paths)
- ✅ Windows client build (compile-only)
- ✅ CMake build path with ctest

## What CI Does NOT Cover

- ❌ Full sustained two-machine streaming session (host → peer, first frame delivered)
- ❌ Hardware capture paths (DRM/KMS, X11 SHM) — no real display available in CI
- ❌ Hardware encoder paths (VA-API, NVENC) — no GPU available in CI
- ❌ Audio pipeline in real playback mode — no audio hardware in CI
- ❌ KDE Plasma client build or runtime validation
- ❌ Android or iOS app build or test
- ❌ React web dashboard functional test
- ❌ VR/Proton stack runtime
- ❌ Long-duration streaming or soak tests
- ❌ Performance regression tracking

## Relationship to the Support Matrix

| Surface | CI coverage | SUPPORT_MATRIX.md status |
|---|---|---|
| Linux native host (`rootstream host`) | Build + CLI startup + integration script | Supported |
| Linux native peer (`rootstream connect`) | Build + CLI startup + integration script | Supported |
| Pairing/bootstrap (`--qr`, peer code) | Identity generation validated in CI | Supported |
| KDE Plasma client | Not covered in CI (separate CMakeLists) | Preview |
| Windows client | Compile-only | Preview |
| Web dashboard | Not covered | Experimental |
| Android client | Not covered | Experimental |
| iOS client | iOS CI workflow exists (`.github/workflows/ios-ci.yml`) | Experimental |
| VR stack | Not covered | Experimental |

## CI Coverage Gap Plan

The following are tracked as Phase 103 work:

1. Add a loopback streaming test that exercises the encode → transmit → decode path without real hardware.
2. Add performance regression tracking for the canonical path (encode latency, memory usage).
3. Add an integration test that validates the client binary connects to a headless host.

See [`docs/microtasks.md`](microtasks.md) PHASE-103 for the full testing roadmap.
