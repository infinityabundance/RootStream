# RootStream Testing Guide

This document describes the test suite structure, coverage, gaps, and
how to run tests for the supported product path.

For CI coverage details: [`docs/CI_COVERAGE.md`](CI_COVERAGE.md)

---

## Test Suite Overview

| Location | Type | Status |
|---|---|---|
| `tests/unit/test_crypto.c` | Unit — cryptography | ✅ 10/10 passing |
| `tests/unit/test_encoding.c` | Unit — H.264/H.265 NAL, frame buffer | ✅ 18/18 passing |
| `tests/integration/test_stream.sh` | Integration — CLI startup, identity, QR, host | ✅ Exercised in CI |
| `tests/unit/test_*.c` (others) | Unit — subsystem components | 🟡 Not all in CI |
| `tests/e2e/test_full_stream.sh` | E2E — Docker-based streaming | ⚪ Not in CI (requires Docker) |
| `tests/vulkan/test_vulkan_integration.c` | Integration — Vulkan renderer | ⚪ Not in CI (requires GPU) |

---

## Running Tests

### Quick check (canonical path)

```bash
# Build and run the two core unit test suites
make HEADLESS=1 test-build
./tests/unit/test_crypto
./tests/unit/test_encoding
```

### Full canonical demo validation

```bash
./scripts/demo.sh
```

### Integration test (CLI and startup)

```bash
./tests/integration/test_stream.sh
```

### CMake-based test suite

```bash
cmake -B build -S . -DENABLE_UNIT_TESTS=ON -DENABLE_INTEGRATION_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

---

## Test Coverage Map

| Product area | Unit test | Integration test | Gap |
|---|---|---|---|
| Ed25519 keypair generation | ✅ test_crypto | ✅ test_stream.sh (identity gen) | None |
| ChaCha20-Poly1305 encrypt/decrypt | ✅ test_crypto | — | — |
| H.264/H.265 NAL parsing | ✅ test_encoding | — | — |
| Frame buffer lifecycle | ✅ test_encoding | — | — |
| CLI startup and help | — | ✅ test_stream.sh | — |
| QR code generation | — | ✅ test_stream.sh | — |
| Host startup and network bind | — | ✅ test_stream.sh | — |
| DRM/KMS capture | — | ⚪ (hardware needed) | No software mock |
| VA-API encoding | — | ⚪ (hardware needed) | No software mock |
| Audio capture (ALSA) | — | ⚪ (hardware needed) | No software mock |
| Network packet encode/decode | 🟡 test_packet.c | — | Not in CI build |
| Session handshake | 🟡 test_session_hs.c | — | Not in CI build |
| ABR controller | 🟡 test_abr.c | — | Not in CI build |
| FEC encode/decode | 🟡 test_fec.c | — | Not in CI build |
| Full streaming session (E2E) | — | ⚪ test_full_stream.sh | Requires Docker |

---

## Known Test Gaps

The following gaps are tracked for Phase 103:

1. **Hardware path mocking**: DRM, VA-API, and ALSA cannot be tested in CI without
   hardware. A software loopback mode (dummy capture → raw encoder → network loopback)
   would allow CI to exercise the full streaming pipeline.

2. **Many unit tests not in CI build**: `tests/unit/test_*.c` files exist for most
   subsystems but the `make test-build` target only compiles `test_crypto` and
   `test_encoding`. The CMake build (`-DENABLE_UNIT_TESTS=ON`) builds more tests.

3. **No regression baselines**: There is no automated comparison of current test
   results against previous test runs. A regression was introduced and caught manually
   in Phase 99 (missing source files in Makefile).

4. **No soak or stress tests in CI**: Long-running tests for memory stability and
   sustained streaming are not currently automated.

---

## Adding a New Test

### Unit test (C)

1. Create `tests/unit/test_<subsystem>.c`
2. Include `tests/common/test_harness.h` for the test framework
3. Add the test to the CMake `ENABLE_UNIT_TESTS` block in `tests/CMakeLists.txt`
4. Add the test to `make test-build` if it tests a core-path subsystem

### Integration test (shell)

1. Create `tests/integration/test_<feature>.sh`
2. Use `tests/integration/integration_harness.h` for shared helpers
3. Add to the CI `integration-tests` job in `.github/workflows/ci.yml`

---

## Pre-Commit Test Checklist

Before opening a PR, run:

```bash
# 1. Build succeeds
make HEADLESS=1

# 2. Unit tests pass
./tests/unit/test_crypto && ./tests/unit/test_encoding

# 3. Demo validation passes
./scripts/demo.sh

# 4. Formatting check
find src include -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror
```
