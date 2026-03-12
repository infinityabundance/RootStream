# RootStream Claims Audit

This audit compares high-visibility product claims against repository evidence visible on disk on 2026-03-12. Labels are:

- `EVIDENCED`: code, manifests, tests, or build targets directly support the claim.
- `PARTIAL`: some implementation exists, but the claim overstates completeness, support, or validation.
- `UNSUPPORTED`: the claim conflicts with current repository evidence or names a capability not supported by the visible implementation.
- `UNCLEAR`: the claim may be plausible, but the repository does not currently provide enough evidence to treat it as proved.

## Audited Claims

| Source | Claim | Repository evidence | Label | Follow-up |
| --- | --- | --- | --- | --- |
| [`README.md`](../../README.md) | RootStream is a Linux-oriented secure P2P streaming product. | Root [`CMakeLists.txt`](../../CMakeLists.txt) builds the Linux `rootstream` executable; [`src/main.c`](../../src/main.c), [`src/network.c`](../../src/network.c), and [`src/crypto.c`](../../src/crypto.c) provide host/network/crypto paths. | `EVIDENCED` | `98.2.1` |
| [`README.md`](../../README.md) | "Zero-configuration" pairing via QR code and instant connection. | [`src/qrcode.c`](../../src/qrcode.c), [`src/main.c`](../../src/main.c), and integration script [`tests/integration/test_stream.sh`](../../tests/integration/test_stream.sh) show QR generation and peer-code flow; however, there is no completed golden-path validation proving a frictionless first-run experience. | `PARTIAL` | `98.4.1`, `99.3.2`, `99.6.2` |
| [`README.md`](../../README.md) | VA-API hardware encoding is available on Intel/AMD. | [`CMakeLists.txt`](../../CMakeLists.txt) detects VA-API and compiles [`src/vaapi_encoder.c`](../../src/vaapi_encoder.c); [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md) and tests reference the VA-API path. | `EVIDENCED` | `98.3.1` |
| [`README.md`](../../README.md) | NVIDIA support is via a VDPAU wrapper and consumer GPU support is "Yes". | The repo contains [`src/nvenc_encoder.c`](../../src/nvenc_encoder.c) and `HAVE_NVENC` checks, but no VDPAU wrapper source was found. [`docs/ROADMAP.md`](../../docs/ROADMAP.md) still lists direct NVENC support as future work. | `UNSUPPORTED` | `98.6.1`, `98.7.1`, `99.1.1` |
| [`README.md`](../../README.md), [`docs/ARCHITECTURE.md`](../../docs/ARCHITECTURE.md) | End-to-end latency is around `14-24ms` and baseline memory is around `15MB`. | The repo has component benchmarks in [`benchmarks/`](../../benchmarks) and host-side latency logging, but no current baseline report or reproducible end-to-end measurement artifact was found. README itself later says the numbers come from limited testing and no comprehensive benchmark suite exists yet. | `UNCLEAR` | `104.1.1`, `104.3.1`, `104.5.1` |
| [`README.md`](../../README.md) | mDNS discovery and LAN auto-discovery are implemented. | [`src/discovery.c`](../../src/discovery.c), [`src/discovery_broadcast.c`](../../src/discovery_broadcast.c), [`src/discovery_manual.c`](../../src/discovery_manual.c), `HAVE_AVAHI` checks, and [`tests/integration/test_discovery_fallback.c`](../../tests/integration/test_discovery_fallback.c) support this. | `EVIDENCED` | `98.4.1` |
| [`README.md`](../../README.md) | Recording supports multiple codecs, container formats, smart storage, and instant replay. | [`src/recording/recording_manager.cpp`](../../src/recording/recording_manager.cpp), encoder wrappers, [`src/recording/disk_manager.h`](../../src/recording/disk_manager.h), [`src/recording/replay_buffer.h`](../../src/recording/replay_buffer.h), and recording tests exist; the top-level CLI path visibly exposes only basic `--record FILE` flow and the supported surface is not yet documented. | `PARTIAL` | `98.3.1`, `98.6.1`, `99.5.1` |
| [`README.md`](../../README.md) | The KDE Plasma client is the recommended native desktop client. | [`clients/kde-plasma-client/CMakeLists.txt`](../../clients/kde-plasma-client/CMakeLists.txt) defines `rootstream-kde-client`, but the client README still marks video rendering, audio playback, input injection, performance metrics, and mDNS discovery as in progress. | `PARTIAL` | `98.6.1`, `98.7.1`, `99.3.2` |
| [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md) | KDE client phases 12-16 are complete. | The subtree build and sources are present, but [`clients/kde-plasma-client/README.md`](../../clients/kde-plasma-client/README.md) explicitly lists core areas as still in progress. | `UNSUPPORTED` | `98.7.1` |
| [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md) | The embedded web dashboard API server, WebSocket push, and auth token support are complete. | [`src/web/api_server.c`](../../src/web/api_server.c), [`src/web/websocket_server.c`](../../src/web/websocket_server.c), and [`tests/unit/test_web_dashboard.c`](../../tests/unit/test_web_dashboard.c) exist, but no standalone web-dashboard executable target was found in the current root build graph. | `PARTIAL` | `98.3.1`, `98.7.1`, `99.1.1` |
| [`frontend/README.md`](../../frontend/README.md) | A real-time web dashboard exists for monitoring and management. | [`frontend/package.json`](../../frontend/package.json) and React sources under [`frontend/src/`](../../frontend/src) exist; backend assumptions are documented, but the current supported run/build path is ambiguous and not tied to a verified top-level entrypoint. | `PARTIAL` | `98.3.1`, `99.1.1` |
| [`android/README.md`](../../android/README.md) | The Android client is a native secure P2P streaming application with rendering, decode, discovery, controls, and PiP. | The Gradle app, manifest, `MainActivity`, and JNI scaffolding exist, but the README's own implementation-status section marks Vulkan, OpenGL ES, video decode, audio, network, discovery, and PiP as stubs or TODO. | `PARTIAL` | `98.3.1`, `98.6.2`, `98.7.1` |
| [`ios/RootStream/README.md`](../../ios/RootStream/README.md) | The iOS client provides Metal rendering, VideoToolbox decode, mDNS, and biometric auth. | The iOS app source tree and CI workflow exist; [`ios/RootStream/RootStream/UI/LoginView.swift`](../../ios/RootStream/RootStream/UI/LoginView.swift) and [`ios/RootStream/RootStream/Utils/SecurityManager.swift`](../../ios/RootStream/RootStream/Utils/SecurityManager.swift) show biometric-related code, but the login flow still contains placeholder completion text and no supported product path has been validated. | `PARTIAL` | `98.3.1`, `98.6.2`, `98.7.1` |
| [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md) | VR / Proton support is materially implemented. | VR sources and tests are present, but [`src/vr/README.md`](../../src/vr/README.md) says the current implementation provides stub/mock functionality and full OpenXR integration remains future work. | `PARTIAL` | `98.3.1`, `98.7.1`, `101.6.1` |
| [`infrastructure/README.md`](../../infrastructure/README.md) | RootStream has cloud infrastructure for deployment and management across AWS, Azure, and GCP. | Infrastructure source and Terraform/Docker/K8s files exist, but [`docs/ROADMAP.md`](../../docs/ROADMAP.md) explicitly lists user accounts, central servers, and any cloud control-plane as out of scope. | `UNSUPPORTED` | `98.2.1`, `98.3.1`, `98.7.1`, `101.6.1` |

## High-Risk Mismatches

- README marketing still leans on NVIDIA via VDPAU-wrapper language, while the visible code path is NVENC-oriented and the roadmap still treats NVIDIA work as future.
- README positions the KDE client as the recommended desktop path, but the client subtree still documents core runtime features as incomplete.
- `docs/IMPLEMENTATION_STATUS.md` overstates completion for at least the KDE client, web dashboard surface, and VR support relative to the current subtree docs and build reality.
- Mobile subtree READMEs present broad capability sets, but their own status sections and code comments show significant stub or placeholder areas.
- Infrastructure docs present a cloud deployment story that conflicts with the roadmap's explicit "no cloud control-plane" stance.

## Follow-Up Coverage

- Support and maturity classification: `98.3.1`, `98.5.2`
- README and public truth cleanup: `98.6.1`, `98.6.2`
- Cross-document reconciliation: `98.7.1`
- Build and entrypoint verification for ambiguous surfaces: `99.1.1`
- Benchmark-backed performance truth: `104.1.1`, `104.3.1`, `104.5.1`
