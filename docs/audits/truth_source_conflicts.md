# RootStream Truth-Source Conflict Map

This document records where repository documents currently disagree about product maturity, support status, or implementation state. It also proposes which document should answer each class of question once reconciliation work is complete.

## Proposed Truth-Source Boundaries

| Topic | Proposed primary truth source | Why |
| --- | --- | --- |
| Current execution state and active work queue | [`docs/microtasks.md`](../microtasks.md) | The canonical ledger now owns current-phase task status. |
| Current implementation evidence and claim grading | [`docs/audits/claims_audit.md`](./claims_audit.md) | It explicitly labels visible claims as evidenced, partial, unsupported, or unclear. |
| Repository layout and entrypoints | [`docs/audits/repository_inventory.md`](./repository_inventory.md) | It maps the current tree and build/manifests without marketing language. |
| Future work and aspiration only | [`docs/ROADMAP.md`](../ROADMAP.md) | Roadmap items should describe future intent, not present support. |
| Supported surfaces and maturity levels | `docs/SUPPORT_MATRIX.md` (to be created by `98.3.1`) | Support decisions need a dedicated source instead of being inferred from README prose. |
| Public product positioning | [`README.md`](../../README.md) after `98.6.1` | README should mirror the support matrix and core path instead of inventing parallel truth. |
| Architecture and dependency boundaries | [`docs/ARCHITECTURE.md`](../ARCHITECTURE.md) after `98.7.1` | Architecture should explain structure and constraints, not product support commitments. |

## Concrete Contradictions

| Topic | Source A | Source B | Conflict | Proposed truth source | Follow-up |
| --- | --- | --- | --- | --- | --- |
| Current execution status | [`docs/microtasks.md`](../microtasks.md) says the phase-98 ledger is canonical. | [`docs/IMPLEMENTATION_STATUS.md`](../IMPLEMENTATION_STATUS.md) still reports old phase counts and overall completion math. | The repository still contains two incompatible stories about what the current execution program is tracking. | [`docs/microtasks.md`](../microtasks.md) | `98.7.1` |
| Supported platform scope | [`README.md`](../../README.md) says Linux-to-Linux is the supported practical path and recommends other platforms use something else for now. | [`android/README.md`](../../android/README.md) and [`ios/RootStream/README.md`](../../ios/RootStream/README.md) present broad mobile client capability sets. | Top-level positioning narrows support while mobile subtree docs read like active product surfaces. | `docs/SUPPORT_MATRIX.md` | `98.2.1`, `98.3.1`, `98.6.2`, `98.7.1` |
| KDE client readiness | [`README.md`](../../README.md) presents the KDE Plasma client as the recommended native desktop client. | [`clients/kde-plasma-client/README.md`](../../clients/kde-plasma-client/README.md) marks video rendering, audio playback, input injection, metrics, and mDNS as in progress; [`docs/IMPLEMENTATION_STATUS.md`](../IMPLEMENTATION_STATUS.md) calls the KDE phases complete. | High-visibility docs simultaneously present the KDE client as recommended, complete, and still missing core runtime work. | [`docs/audits/claims_audit.md`](./claims_audit.md) until `docs/SUPPORT_MATRIX.md` exists | `98.3.1`, `98.6.1`, `98.7.1` |
| NVIDIA support story | [`README.md`](../../README.md) says NVIDIA works via a VDPAU wrapper and presents consumer GPU support positively. | [`docs/ROADMAP.md`](../ROADMAP.md) says direct NVENC support is future work, while the visible code path is [`src/nvenc_encoder.c`](../../src/nvenc_encoder.c) and no VDPAU wrapper source was found. | The user-facing NVIDIA story is internally inconsistent across README, roadmap, and code evidence. | [`docs/audits/claims_audit.md`](./claims_audit.md) plus `99.1.1` build audit | `98.6.1`, `98.7.1`, `99.1.1` |
| Web dashboard maturity | [`docs/IMPLEMENTATION_STATUS.md`](../IMPLEMENTATION_STATUS.md) says the embedded HTTP API server, WebSocket push, and auth token support are complete. | [`frontend/README.md`](../../frontend/README.md) presents an operational dashboard, while the root build graph only exposes optional `src/web/*.c` sources and no standalone dashboard target. | The repo describes a complete dashboard surface, but the supported build/run story is still ambiguous. | [`docs/audits/claims_audit.md`](./claims_audit.md) until support/build docs are reconciled | `98.3.1`, `98.7.1`, `99.1.1` |
| Performance proof | [`README.md`](../../README.md) and [`docs/ARCHITECTURE.md`](../ARCHITECTURE.md) present `14-24ms` latency and `~15MB` baseline memory figures. | [`README.md`](../../README.md) later says those numbers come from limited testing and that no comprehensive benchmark suite exists yet; current benchmark docs only cover component-level cases. | Performance numbers appear both as headline guidance and as caveated non-baseline data. | Future benchmark docs produced by `104.2.2`, `104.3.1`, and `104.5.1` | `104.1.1`, `104.3.1`, `104.5.1` |
| VR maturity | [`docs/IMPLEMENTATION_STATUS.md`](../IMPLEMENTATION_STATUS.md) lists several VR/OpenXR tasks as completed. | [`src/vr/README.md`](../../src/vr/README.md) says the current VR implementation is stub/mock and full OpenXR integration remains future work. | Status language overstates readiness relative to the subsystem's own README. | [`docs/audits/claims_audit.md`](./claims_audit.md) until support matrix and architecture docs are reconciled | `98.7.1`, `101.6.1` |
| Cloud / no-server model | [`README.md`](../../README.md) says no central servers are required and positions RootStream as direct P2P. [`docs/ROADMAP.md`](../ROADMAP.md) says cloud control-plane work is out of scope. | [`infrastructure/README.md`](../../infrastructure/README.md) presents multi-cloud deployment and management as an infrastructure surface. | Infrastructure docs read like a supported product lane that conflicts with the top-level no-server and no-cloud positioning. | Product scope docs from `98.2.1` plus `docs/SUPPORT_MATRIX.md` | `98.2.1`, `98.3.1`, `98.7.1`, `101.6.1` |
| Roadmap timing vs current code | [`docs/ROADMAP.md`](../ROADMAP.md) places the Windows client in a future `v2.0` section. | [`CMakeLists.txt`](../../CMakeLists.txt) and [`ci.yml`](../../.github/workflows/ci.yml) already define and build `rootstream-client`. | The roadmap still frames a current codepath as future-only work. | [`docs/audits/repository_inventory.md`](./repository_inventory.md) for current existence; [`docs/ROADMAP.md`](../ROADMAP.md) for future work only | `98.7.1`, `99.1.1` |

## Reconciliation Direction

- `README.md` should stop being an independent status source and instead summarize the support matrix and core path.
- `docs/IMPLEMENTATION_STATUS.md` should either become a narrowly scoped implementation-evidence report or be reduced to a pointer to the newer audit/support docs.
- `docs/ROADMAP.md` should speak only about future work and should not double as a support or release-state document.
- Subtree READMEs for KDE, web, mobile, VR, and infrastructure should be explicitly labeled as supported, preview, experimental, or non-core once the support matrix exists.
