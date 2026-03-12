# RootStream Core Path

This document describes the single canonical RootStream path the repository currently treats as the supported product journey. It is intentionally narrow and Linux-first.

The path below is grounded in the current root build, the native CLI entrypoints in [`src/main.c`](../src/main.c), and the existing integration script in [`tests/integration/test_stream.sh`](../tests/integration/test_stream.sh). Where full runtime proof is still incomplete, that limitation is stated explicitly instead of being hidden.

Use neighboring docs for adjacent truth:

- Product definition and non-goals: [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md)
- Support and maturity status: [`docs/SUPPORT_MATRIX.md`](./SUPPORT_MATRIX.md)
- Future-only work: [`docs/ROADMAP.md`](./ROADMAP.md)
- Architecture boundaries: [`docs/ARCHITECTURE.md`](./ARCHITECTURE.md)

## Canonical Scope

- Host: Linux native `rootstream host`
- Peer: Linux native `rootstream connect <code>`
- Network model: direct peer-to-peer on a LAN or similarly controlled network
- Pairing/bootstrap: `rootstream --qr`, peer code, and local discovery paths

This document does not define a KDE-first, mobile-first, browser-first, VR-first, or cloud-managed workflow.

## Prerequisites

- Linux environment for both host and peer
- Native RootStream build completed from the root project
- A usable display/capture environment on the host for an actual first stream
- Network reachability between host and peer on the chosen port
- Required root build dependencies available for the selected environment

## Canonical Sequence

1. Build the native binary from the repository root.
2. On the future host machine, run `rootstream --qr` once to generate identity material and expose a shareable RootStream code.
3. On the host machine, start the streaming side with `rootstream host`.
4. On the peer Linux machine, connect with `rootstream connect <code>`.
5. Confirm that the peer reaches the first successful encrypted stream state.

## Validation Checkpoints

| Step | Command or observation | Expected signal | Failure indicator | Diagnose next |
| --- | --- | --- | --- | --- |
| Binary available | `./rootstream --help` | Usage output renders and exits without crashing | Binary missing or command exits unexpectedly | Re-check the root build path, root `Makefile`, and `CMakeLists.txt`. |
| Identity generation | `XDG_CONFIG_HOME=/tmp/rootstream-core ./rootstream --qr` | Output includes `RootStream Code:` and a code containing `@`; integration script also expects generated keys under `$XDG_CONFIG_HOME/rootstream/keys/` | No code output, missing key files, or crypto/init failure | Inspect [`src/main.c`](../src/main.c), [`src/qrcode.c`](../src/qrcode.c), and the key-generation checks in [`tests/integration/test_stream.sh`](../tests/integration/test_stream.sh). |
| Host startup | `./rootstream host --port 19876` | CLI prints `INFO: Starting host mode`, selects or falls back to a display, then prints `✓ All systems ready` and `→ Waiting for connections...` | Immediate exit, `ERROR: Network init failed`, or display-selection failure | Inspect host stdout/stderr first, then host-side dependency and display assumptions in [`docs/SUPPORT_MATRIX.md`](./SUPPORT_MATRIX.md). |
| Peer initiation | `./rootstream connect <code>` | CLI prints `INFO: Connecting to peer: ...`, then `✓ Connection initiated` and `INFO: Waiting for handshake...` | `ERROR: Failed to connect to peer` or early exit from the client path | Inspect peer connectivity, the supplied code, and the connect path in [`src/main.c`](../src/main.c). |
| First-stream attempt | Host remains alive while the peer enters the client service loop | Host and peer both stay running long enough to indicate the session path advanced beyond argument parsing and initial network setup | Client or host exits immediately after connect, or no session progress is visible | Use host and peer logs first; then compare behavior with the loopback checks in [`tests/integration/test_stream.sh`](../tests/integration/test_stream.sh). |

## Automation Reference

- [`tests/integration/test_stream.sh`](../tests/integration/test_stream.sh) currently validates adjacent pieces of the canonical path:
  - CLI startup
  - key generation
  - QR output
  - host startup and network binding
  - config creation
  - loopback setup when a display is available
- The script does not yet prove a full sustained interactive client render path. That remains follow-up work in Phase 99.

## Why This Is the Canonical Path

- It maps directly to the root executable and command surface already present in [`src/main.c`](../src/main.c).
- It aligns with the product scope in [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md).
- It avoids treating the KDE client, web dashboard, Windows client, or mobile apps as equally supported defaults.
- The repository’s integration script already exercises adjacent pieces of this path: help/version startup, identity generation, QR output, host startup, config handling, and loopback setup.

## Current Limitations

- The repository has stronger evidence for startup, identity generation, QR output, and host/network initialization than it does for a fully validated end-to-end first-frame client success path.
- The integration script currently stops short of proving a complete interactive client session; it verifies loopback setup rather than a full sustained client render path.
- Hardware acceleration, mDNS convenience, GUI mode, and recording depend on optional system packages and runtime conditions.
- This path should currently be read as the canonical supported journey to optimize and validate further, not as proof that every adjacent surface has equal maturity.

## Adjacent Paths That Are Not Canonical

- `rootstream-kde-client`
- `rootstream-client` on Windows
- React dashboard and optional web API surfaces
- Android and iOS clients
- VR / Proton surfaces
- Infrastructure or cloud deployment surfaces
