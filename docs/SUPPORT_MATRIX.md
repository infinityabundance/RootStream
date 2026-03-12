# RootStream Support Matrix

This matrix records the current support posture of visible RootStream surfaces based on repository evidence gathered in Phase 98.1 and refined by the product-core definition in [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md).

This document answers a narrower question than "does code exist?" It records what the repository currently supports, what appears usable but not yet support-committed, what remains experimental, and what should be treated as roadmap-only or non-core until stronger proof exists.

Use neighboring docs for different questions:

- Product definition and non-goals: [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md)
- Canonical supported workflow: [`docs/CORE_PATH.md`](./CORE_PATH.md)
- Future-only work: [`docs/ROADMAP.md`](./ROADMAP.md)
- Architecture boundaries: [`docs/ARCHITECTURE.md`](./ARCHITECTURE.md)

## Maturity Categories

| Status | Meaning |
| --- | --- |
| Supported | Present in the repository, part of the current product core, and something the docs should actively direct users toward today. |
| Preview | Present and intentionally visible, but not yet the canonical supported path; may have partial validation or incomplete support commitments. |
| Experimental | Code exists, but implementation presence is not the same as support; build, runtime, or documentation proof is incomplete, ambiguous, or contradicted elsewhere. |
| Roadmap | Future-facing work or a target state that should not be described as current support, even if adjacent scaffolding or partial code exists. |

These labels describe support commitment, not just code presence. A surface can exist in-tree and still be `Experimental` or `Roadmap` if the repository does not currently provide enough evidence to support it honestly.

## Surface Matrix

| Surface | Role | Status | Primary evidence | Evidence basis |
| --- | --- | --- | --- | --- |
| Linux native host via `rootstream host` | Core host path | Supported | [`src/main.c`](../src/main.c), [`CMakeLists.txt`](../CMakeLists.txt), [`tests/integration/test_stream.sh`](../tests/integration/test_stream.sh) | Root build exposes the Linux host executable and CI builds/tests the native path on Ubuntu. |
| Linux native peer via `rootstream connect <code>` | Core peer path | Supported | [`src/main.c`](../src/main.c), [`src/service.c`](../src/service.c), [`src/client_session.c`](../src/client_session.c) | The shared native runtime exposes a Linux peer connection flow in the main executable and the product core defines this as the canonical peer path. |
| Linux pairing and discovery (`--qr`, peer code, mDNS/broadcast/manual`) | Core connection bootstrap | Supported | [`src/qrcode.c`](../src/qrcode.c), [`src/discovery.c`](../src/discovery.c), [`src/discovery_broadcast.c`](../src/discovery_broadcast.c), [`src/discovery_manual.c`](../src/discovery_manual.c) | Pairing and discovery code exists in the root runtime and is already part of the native Linux flow. |
| KDE Plasma client subtree | Alternate Linux desktop client | Preview | [`clients/kde-plasma-client/CMakeLists.txt`](../clients/kde-plasma-client/CMakeLists.txt), [`clients/kde-plasma-client/README.md`](../clients/kde-plasma-client/README.md) | A separate Qt/KDE client target exists, but its own README still marks core runtime areas as in progress, so it is not the canonical supported client. |
| Windows `rootstream-client` | Windows peer/client executable | Preview | [`src/main_client.c`](../src/main_client.c), [`CMakeLists.txt`](../CMakeLists.txt), [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | A Windows client entrypoint and CI build job exist, but the roadmap and top-level product positioning do not yet present it as part of the supported core. |
| Embedded web API backend in `src/web/` | Optional management/backend surface | Experimental | [`src/web/api_server.c`](../src/web/api_server.c), [`src/web/websocket_server.c`](../src/web/websocket_server.c), [`CMakeLists.txt`](../CMakeLists.txt) | Backend code exists behind the optional `BUILD_WEB_DASHBOARD` flag, but the default root build and supported run path do not currently make it a primary supported surface. |
| React web dashboard in `frontend/` | Browser management UI | Experimental | [`frontend/package.json`](../frontend/package.json), [`frontend/README.md`](../frontend/README.md) | The frontend is present, but its backend/API assumptions are not yet tied to a verified supported top-level entrypoint. |
| Android client in `android/RootStream/` | Mobile peer/client app | Experimental | [`android/README.md`](../android/README.md), [`android/RootStream/app/build.gradle.kts`](../android/RootStream/app/build.gradle.kts) | The Android app structure is substantial, but its own implementation-status section still marks rendering, decode, audio, network, and discovery as stub or TODO. |
| iOS client in `ios/RootStream/` | Mobile peer/client app | Experimental | [`ios/RootStream/README.md`](../ios/RootStream/README.md), [`ios/RootStream/RootStream/App/RootStreamApp.swift`](../ios/RootStream/RootStream/App/RootStreamApp.swift) | The app, workspace, and CI exist, but the repository still lacks a validated supported product path for iOS. |
| VR / Proton stack in `src/vr/` | Immersive/adjacent runtime surface | Experimental | [`src/vr/README.md`](../src/vr/README.md), [`CMakeLists.txt`](../CMakeLists.txt) | VR code exists behind the optional `BUILD_VR_SUPPORT` path, but current docs and audits do not support treating it as a supported product surface. |
| Cloud, Terraform, Docker, and K8s infrastructure in `infrastructure/` | Operational or deployment assets | Experimental | [`infrastructure/README.md`](../infrastructure/README.md), [`docs/ROADMAP.md`](./ROADMAP.md), [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md) | Infrastructure assets exist, but the supported product core explicitly excludes a cloud-managed or central-control-plane story. |
| Direct NVIDIA-first host acceleration story | High-visibility host capability claim | Roadmap | [`docs/ROADMAP.md`](./ROADMAP.md), [`docs/audits/claims_audit.md`](./audits/claims_audit.md), [`src/nvenc_encoder.c`](../src/nvenc_encoder.c) | NVIDIA-related code exists, but the docs conflict and the repository does not yet provide a clean, supportable NVIDIA-first story for the product core. |
| Full OpenXR-grade VR runtime support | High-visibility adjacent capability claim | Roadmap | [`src/vr/README.md`](../src/vr/README.md), [`docs/audits/claims_audit.md`](./audits/claims_audit.md) | VR scaffolding exists, but the current repo does not support describing full OpenXR headset support as present product reality. |

## Applied Maturity Labels

| Visible command, feature, or surface | Current maturity | Notes |
| --- | --- | --- |
| `rootstream host` | Supported | Canonical Linux host entrypoint for the supported product core. |
| `rootstream connect <code>` | Supported | Canonical Linux peer entrypoint for the supported product core. |
| `rootstream --qr` and peer-code bootstrap | Supported | Part of the current supported pairing and discovery path. |
| `rootstream --record` and native recording path | Preview | Code and CLI exposure exist, but the recording lane is not yet the primary supported story and still needs stronger end-to-end validation. |
| `rootstream-kde-client` | Preview | Visible desktop client target exists, but the subtree still documents several core areas as in progress. |
| `rootstream-client` on Windows | Preview | Build target and CI job exist, but the product core remains Linux-first and does not yet treat Windows as a default supported path. |
| Embedded web API/backend plus React dashboard | Experimental | Present and visible, but the supported build/run path remains ambiguous. |
| Android client | Experimental | Significant app structure exists, but core runtime areas remain stubbed or incomplete in the subtree's own status language. |
| iOS client | Experimental | App and CI surfaces exist, but the supported path is still unvalidated. |
| VR / Proton code path | Experimental | In-tree code exists, but the repo does not currently support presenting it as a stable user-facing product lane. |
| Direct NVIDIA-first host path | Roadmap | README and roadmap language still disagree about what is truly available today. |
| Full OpenXR-grade VR path | Roadmap | The future target should remain future language until later phases prove otherwise. |

## Support Direction

- The only currently supported end-user product path is the Linux native host and Linux native peer flow documented in [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md).
- Preview surfaces may become supported later, but they should not be presented as default or equally validated today.
- Experimental surfaces should be documented honestly as present code, not as support commitments.
- Future support expansion should only happen after build, runtime, and documentation proof exists for the target surface.

## Environment Constraints and Caveats

| Surface | Current environment assumptions | Caveats and unsupported or unverified environments |
| --- | --- | --- |
| Linux native host via `rootstream host` | Linux build using the root CMake/Make flow; core dependencies include SDL2, libsodium, Opus, libdrm, ALSA, and on non-headless builds GTK3; CI currently exercises Ubuntu Linux in [`.github/workflows/ci.yml`](../.github/workflows/ci.yml). | This is the only supported product surface. Hardware acceleration, discovery, GUI mode, and recording vary with optional dependencies such as VA-API, Avahi, PulseAudio, PipeWire, FFmpeg, X11, and ncurses. Browser-only, cloud-managed, and non-Linux host paths are outside the supported core. |
| Linux native peer via `rootstream connect <code>` | Same Linux native toolchain and shared runtime as the host path; peer flow is exposed by [`src/main.c`](../src/main.c) and depends on the root build succeeding. | This path is supported only as part of the Linux-to-Linux native core definition. A broader multi-platform client support promise is not yet justified. |
| Linux pairing and discovery (`--qr`, peer code, mDNS/broadcast/manual`) | QR generation depends on the root native runtime; mDNS depends on Avahi when available; fallback discovery code exists for broadcast and manual entry. | Discovery convenience depends on optional packages and local-network conditions. The repository supports the existence of these paths, but not a universal "zero configuration" promise across all environments. |
| KDE Plasma client subtree | Linux desktop environment with Qt 6.4+, libsodium, Opus, and `libRootStream`; optional KDE Frameworks 6, VA-API, PulseAudio, and PipeWire per [`clients/kde-plasma-client/README.md`](../clients/kde-plasma-client/README.md). | The subtree README still marks video rendering, audio playback, input injection, metrics, and mDNS as in progress. It should be treated as preview rather than as the default supported client. |
| Windows `rootstream-client` | Windows build path exists in CMake and CI. | Current product-core docs do not treat Windows as part of the supported core, and no canonical Windows-first journey is documented yet. |
| Embedded web API backend in `src/web/` | Requires opting into `BUILD_WEB_DASHBOARD=ON` in [`CMakeLists.txt`](../CMakeLists.txt). | Not part of the default root build or the current supported product path. Runtime and packaging expectations remain ambiguous. |
| React web dashboard in `frontend/` | Requires Node.js 16+ and npm; expects backend services on `localhost:8080` and `localhost:8081` per [`frontend/README.md`](../frontend/README.md). | Browser UI should be treated as experimental until a supported backend run path and environment story are documented and validated. |
| Android client in `android/RootStream/` | Android Studio Hedgehog or later, JDK 17, Android SDK 34, NDK r25c+, Gradle 8.2+, and Android API 24+ per [`android/README.md`](../android/README.md). | The Android subtree documents major rendering, decode, audio, network, discovery, and PiP work as stub or TODO. It is unverified as a supported client path. |
| iOS client in `ios/RootStream/` | iOS 15+, Xcode 14+, CocoaPods, and a Metal-capable device per [`ios/RootStream/README.md`](../ios/RootStream/README.md). | The iOS subtree is present, but the repository does not yet provide a validated, support-committed iOS user journey. |
| VR / Proton stack in `src/vr/` | Requires opting into `BUILD_VR_SUPPORT=ON`; README assumes VR hardware and platform-specific runtimes. | The VR README itself says the current implementation is stub/mock and that fuller OpenXR integration remains future work. |
| Cloud, Terraform, Docker, and K8s infrastructure in `infrastructure/` | Requires separate Docker, Terraform, Kubernetes, and cloud tooling outside the root product build. | These assets exist, but the product core explicitly excludes a required cloud control-plane or hosted service story. They should not be read as proof of supported cloud deployment for end users. |

## Current Support Boundary

- Supported today means Linux-first native host and Linux-first native peer on a private or similarly controlled network.
- Preview and experimental surfaces may build or contain substantial code, but they are not equivalent to a documented and validated support commitment.
- Environments not explicitly named above should be treated as unverified until later phases produce concrete build, runtime, and troubleshooting evidence.

## Source Links

- Product scope: [`docs/PRODUCT_CORE.md`](./PRODUCT_CORE.md)
- Repository inventory: [`docs/audits/repository_inventory.md`](./audits/repository_inventory.md)
- Claims grading: [`docs/audits/claims_audit.md`](./audits/claims_audit.md)
- Conflict map: [`docs/audits/truth_source_conflicts.md`](./audits/truth_source_conflicts.md)
