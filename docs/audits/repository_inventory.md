# RootStream Repository Inventory

This inventory records the current repository layout and likely product entrypoints based on the tree, build files, app manifests, and high-visibility docs present on disk on 2026-03-12. It is intentionally evidence-based and does not treat README claims as implementation proof.

## Top-Level Subsystems

| Path | Apparent role | Evidence |
| --- | --- | --- |
| `src/` | Native C/C++ core, runtime, protocol, capture, encode, audio, networking, discovery, recording, security, web, and VR sources | Root [`CMakeLists.txt`](../../CMakeLists.txt) builds `rootstream_core`, `rootstream`, `rstr-player`, and `rootstream-client` from `src/` and related platform sources. |
| `include/` | Public native API headers | [`include/rootstream.h`](../../include/rootstream.h) and [`include/rootstream_client_session.h`](../../include/rootstream_client_session.h) are exported by the root build. |
| `clients/kde-plasma-client/` | Separate Qt 6 / QML KDE desktop client | [`clients/kde-plasma-client/CMakeLists.txt`](../../clients/kde-plasma-client/CMakeLists.txt) defines `rootstream-kde-client`; subtree contains `qml/`, `src/`, packaging, and tests. |
| `frontend/` | React web dashboard frontend | [`frontend/package.json`](../../frontend/package.json) defines `start`, `build`, and `test`; [`frontend/src/index.js`](../../frontend/src/index.js) and [`frontend/src/App.js`](../../frontend/src/App.js) are the frontend entry files. |
| `android/RootStream/` | Android client app with Kotlin and JNI/native rendering helpers | [`android/RootStream/app/build.gradle.kts`](../../android/RootStream/app/build.gradle.kts) defines the app module; native CMake under `app/src/main/cpp/` builds JNI libraries. |
| `ios/RootStream/` | iOS client app with SwiftUI, CocoaPods, and XCTest | [`ios/RootStream/Podfile`](../../ios/RootStream/Podfile), [`ios/RootStream/RootStream/App/RootStreamApp.swift`](../../ios/RootStream/RootStream/App/RootStreamApp.swift), and [`ios/RootStream/RootStreamTests/RootStreamTests.swift`](../../ios/RootStream/RootStreamTests/RootStreamTests.swift) are present. |
| `infrastructure/` | Cloud, Docker, Kubernetes, monitoring, Terraform, and deploy scripts | Subtrees exist for `cloud/`, `docker/`, `k8s/`, `monitoring/`, `terraform/`, and `scripts/`; see [`infrastructure/README.md`](../../infrastructure/README.md). |
| `benchmarks/` | Standalone performance benchmark sources and instructions | [`benchmarks/README.md`](../../benchmarks/README.md) documents standalone benchmark programs in [`benchmarks/`](../../benchmarks). |
| `tests/` | Root native unit and integration tests | [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt) defines CTest targets for integration and unit coverage. |
| `tools/` | Auxiliary native tools outside the main executable | [`tools/rstr-player.c`](../../tools/rstr-player.c) is built as the `rstr-player` executable by the root CMake project. |
| `scripts/` | Local validation and analysis helpers | Includes [`scripts/smoke.sh`](../../scripts/smoke.sh), [`scripts/run_cppcheck.sh`](../../scripts/run_cppcheck.sh), [`scripts/run_sanitizers.sh`](../../scripts/run_sanitizers.sh), and [`scripts/validate_traceability.sh`](../../scripts/validate_traceability.sh). |
| `packaging/` | Packaging helpers and distro artifacts | Includes [`packaging/build_appimage.sh`](../../packaging/build_appimage.sh) and [`packaging/rootstream.spec`](../../packaging/rootstream.spec). |
| `docs/` | Product, architecture, status, support, troubleshooting, and audit docs | High-visibility docs include [`README.md`](../../README.md), [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md), [`docs/ROADMAP.md`](../../docs/ROADMAP.md), [`docs/ARCHITECTURE.md`](../../docs/ARCHITECTURE.md), and [`docs/SECURITY.md`](../../docs/SECURITY.md). |
| `.github/workflows/` | CI surfaces | Current workflows are [`ci.yml`](../../.github/workflows/ci.yml) and [`ios-ci.yml`](../../.github/workflows/ios-ci.yml). |

## Candidate Product and Developer Entrypoints

| Surface | Candidate entrypoint | Evidence | Notes |
| --- | --- | --- | --- |
| Linux host / tray app | `rootstream` via [`src/main.c`](../../src/main.c) | Root [`CMakeLists.txt`](../../CMakeLists.txt) defines `add_executable(rootstream src/main.c)` and the file documents default tray, `host`, `connect`, and `--service` modes. | This is the main Linux executable exposed by both CMake and the root [`Makefile`](../../Makefile). |
| Linux recording playback tool | `rstr-player` via [`tools/rstr-player.c`](../../tools/rstr-player.c) | Root [`CMakeLists.txt`](../../CMakeLists.txt) defines `add_executable(rstr-player tools/rstr-player.c)`. | Auxiliary tool, not a primary host/client surface. |
| Windows client | `rootstream-client` via [`src/main_client.c`](../../src/main_client.c) | Root [`CMakeLists.txt`](../../CMakeLists.txt) defines `add_executable(rootstream-client WIN32 src/main_client.c)`. | Windows-only client path. |
| Shared native backend | `rootstream_core` static library | Root [`CMakeLists.txt`](../../CMakeLists.txt) defines `add_library(rootstream_core STATIC ...)`. | Common backend for Linux host/tooling and a likely integration point for other clients. |
| KDE Plasma desktop client | `rootstream-kde-client` via [`clients/kde-plasma-client/src/main.cpp`](../../clients/kde-plasma-client/src/main.cpp) | [`clients/kde-plasma-client/CMakeLists.txt`](../../clients/kde-plasma-client/CMakeLists.txt) defines `add_executable(rootstream-kde-client ...)`. | Separate desktop client subtree with its own packaging and tests. |
| Web dashboard frontend | React app via [`frontend/src/index.js`](../../frontend/src/index.js) | [`frontend/package.json`](../../frontend/package.json) provides `npm start`, `npm run build`, and `npm test`. | Frontend expects REST API on `localhost:8080` and WebSocket on `localhost:8081` per [`frontend/README.md`](../../frontend/README.md). |
| Web dashboard backend/API | Optional `src/web/*.c` sources | Root [`CMakeLists.txt`](../../CMakeLists.txt) appends `src/web/api_server.c`, `websocket_server.c`, `auth_manager.c`, `rate_limiter.c`, and `api_routes.c` when `BUILD_WEB_DASHBOARD=ON`. | A standalone web-dashboard executable was not found in the current top-level target list; this appears to be optional backend functionality folded into the native build. |
| Android app | `com.rootstream.MainActivity` via [`android/RootStream/app/src/main/kotlin/com/rootstream/MainActivity.kt`](../../android/RootStream/app/src/main/kotlin/com/rootstream/MainActivity.kt) | [`android/RootStream/app/build.gradle.kts`](../../android/RootStream/app/build.gradle.kts) defines the `com.rootstream` application module. | Native JNI helpers are built from `app/src/main/cpp/CMakeLists.txt`. |
| Android native helpers | `rootstream_vulkan`, `rootstream_opus`, `rootstream_gles` | [`android/RootStream/app/src/main/cpp/CMakeLists.txt`](../../android/RootStream/app/src/main/cpp/CMakeLists.txt) defines three shared libraries. | These are support libraries, not user-facing launch entrypoints. |
| iOS app | `RootStreamApp` via [`ios/RootStream/RootStream/App/RootStreamApp.swift`](../../ios/RootStream/RootStream/App/RootStreamApp.swift) | [`ios-ci.yml`](../../.github/workflows/ios-ci.yml) builds workspace `RootStream.xcworkspace` and scheme `RootStream`. | iOS subtree contains app, UI, rendering, network, input, and utility sources. |
| Docker/local infra | [`infrastructure/docker/docker-compose.yml`](../../infrastructure/docker/docker-compose.yml) | `docker-compose.yml` is present and documented in [`infrastructure/README.md`](../../infrastructure/README.md). | Infra surface, not part of the default root build. |
| Deployment automation | [`infrastructure/scripts/deploy.sh`](../../infrastructure/scripts/deploy.sh) | Deploy/scale/backup scripts are present under [`infrastructure/scripts/`](../../infrastructure/scripts). | Separate operational path. |
| Terraform infra | [`infrastructure/terraform/main.tf`](../../infrastructure/terraform/main.tf) | Terraform module files are present under [`infrastructure/terraform/`](../../infrastructure/terraform). | Separate operational path. |
| Benchmarks | Standalone sources in [`benchmarks/`](../../benchmarks) | [`benchmarks/encode_latency_bench.c`](../../benchmarks/encode_latency_bench.c), [`benchmarks/network_throughput_bench.c`](../../benchmarks/network_throughput_bench.c), and [`benchmarks/vulkan_renderer_bench.cpp`](../../benchmarks/vulkan_renderer_bench.cpp) are present. | [`benchmarks/README.md`](../../benchmarks/README.md) documents manual invocations; its documented `vulkan_renderer_bench` CMake target was not surfaced by the initial root build-file search. |
| Root native tests | CTest plus standalone test executables | [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt) defines multiple unit and integration test executables. | Separate validation surface. |

## High-Visibility Documentation Surfaces

- [`README.md`](../../README.md): public project positioning and install/use claims.
- [`docs/IMPLEMENTATION_STATUS.md`](../../docs/IMPLEMENTATION_STATUS.md): implementation-vs-claim status narrative.
- [`docs/ROADMAP.md`](../../docs/ROADMAP.md): planned or aspirational work.
- [`docs/ARCHITECTURE.md`](../../docs/ARCHITECTURE.md): subsystem and design narrative.
- [`docs/SECURITY.md`](../../docs/SECURITY.md): security posture and reporting guidance.
- [`docs/QUICKSTART.md`](../../docs/QUICKSTART.md) and [`docs/TROUBLESHOOTING.md`](../../docs/TROUBLESHOOTING.md): user and contributor onboarding surfaces.

## First-Pass Boundary Notes

- The native root build is centered on a shared `rootstream_core` static library and thin executable entrypoints, which means multiple product surfaces depend on the same backend code.
- Desktop client work is split between the root native executable path and the separate KDE/Qt client subtree.
- Web support is split between a React frontend in `frontend/` and optional backend/API sources under `src/web/`.
- Mobile surfaces use separate toolchains and project structures rather than participating in the default root `make` flow.
- Infrastructure, benchmarks, and packaging are all present as separate operational surfaces that should not automatically be treated as supported end-user product paths.
