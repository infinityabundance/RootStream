# RootStream Product Core

This document defines the supported RootStream product based on repository evidence gathered in Phase 98.1. It is not a roadmap. It describes the product path the repository can currently defend with code, build targets, and visible entrypoints.

For neighboring questions:

- Support and maturity labels: [`docs/SUPPORT_MATRIX.md`](./SUPPORT_MATRIX.md)
- Step-by-step supported workflow: [`docs/CORE_PATH.md`](./CORE_PATH.md)
- Future work only: [`docs/ROADMAP.md`](./ROADMAP.md)
- Architecture and subsystem boundaries: [`docs/ARCHITECTURE.md`](./ARCHITECTURE.md)

## Purpose

RootStream is a Linux-first, self-hosted, peer-to-peer game streaming toolchain for people who want direct device-to-device streaming without accounts or a required cloud control-plane.

The product core today is deliberately narrower than the full repository surface. The repo contains KDE, web, mobile, VR, and infrastructure code, but those surfaces are not the primary supported product path until later phases establish support, build, and runtime proof for them.

## Supported Product Today

The current supported product definition is:

- Host a stream from a Linux machine using the native `rootstream` executable.
- Connect from another Linux machine using the native `rootstream connect <code>` path exposed by `src/main.c`.
- Run this as a direct peer-to-peer workflow on a local network or equivalent private network where the host and client can reach each other.
- Use the project's built-in pairing, discovery, encryption, capture, encode, and transport layers that are already represented in the root build and native runtime.

This definition is evidence-backed by:

- The root build and entrypoints in [`CMakeLists.txt`](../CMakeLists.txt) and [`src/main.c`](../src/main.c).
- The repository inventory in [`docs/audits/repository_inventory.md`](./audits/repository_inventory.md).
- The claims grading in [`docs/audits/claims_audit.md`](./audits/claims_audit.md).
- The contradiction mapping in [`docs/audits/truth_source_conflicts.md`](./audits/truth_source_conflicts.md).

## Primary User and Use Case

The primary supported user is a technical Linux user who wants to stream games or an interactive Linux desktop session directly to a peer device without depending on hosted accounts or a vendor cloud.

The primary supported use case is:

- start the Linux host with `rootstream host`
- pair or connect directly with a peer
- join from another Linux machine with `rootstream connect <code>`
- complete a first encrypted stream on a LAN or similarly controlled network

This is the path the repository should optimize, document, test, and truthfully present first.

## Supported User Journey

The supported user journey today is:

1. Build or install the native Linux `rootstream` executable in a Linux environment.
2. On the host machine, start the streaming side with `rootstream host`.
3. Use `rootstream --qr` or the text peer code flow to share host identity material with the peer.
4. On the peer Linux machine, connect with `rootstream connect <code>`.
5. Validate that the session reaches an encrypted first-stream state on a LAN or similarly controlled network.

This journey is intentionally narrow. It maps to command surfaces and entrypoints already exposed by [`src/main.c`](../src/main.c) rather than to the broader collection of in-tree clients and dashboards.

## Canonical Product Path

Until support and runtime validation work says otherwise, the canonical product path is:

1. Linux host via `rootstream host`
2. Linux peer via `rootstream connect <code>`
3. direct P2P networking, local discovery, and built-in encryption

The KDE client, web dashboard, mobile apps, VR stack, Windows client packaging story, and infrastructure surfaces remain outside this canonical path for now. Some of them contain substantial code, but current repository evidence does not justify presenting them as the default supported product.

## Target Platforms

Supported target platform for the product core:

- Linux host
- Linux-focused native runtime and tooling
- private LAN or similarly controlled network environment

Present in the repository but not part of the supported core today:

- KDE Plasma desktop client subtree
- Windows client executable path
- Android client
- iOS client
- React web dashboard
- VR / Proton surfaces
- cloud and infrastructure deployment surfaces

## Non-Goals for the Current Product Definition

These are explicit non-goals for the currently supported product path:

- treating the KDE Plasma client as the default supported desktop client
- treating Android or iOS as first-class supported client paths
- treating the React dashboard as a default operations or control surface
- treating VR / Proton work as part of the supported baseline experience
- treating cloud deployment, hosted control planes, or account systems as part of the product core
- treating benchmark targets or roadmap ambitions as present support claims
- treating "code exists in-tree" as equivalent to "supported and validated"

These non-goals may change in later phases, but they are excluded from the current support definition until the repository has support-matrix, runtime, and validation proof to back them.

## Explicit Exclusions From the Product Core

The following are excluded from the current product core definition even when code exists in-tree:

- cloud-managed or centrally hosted RootStream service offerings
- browser-first or WebRTC-first client support
- mobile clients as a default supported path
- KDE client as the primary recommended desktop client
- VR / OpenXR support as part of the default product claim
- benchmark-backed performance claims beyond the evidence currently captured in the repo

These exclusions are based on the current audit evidence, not on long-term desirability.

## Document Role

- This document defines what RootStream is and which path the repository currently supports.
- `docs/SUPPORT_MATRIX.md` will later classify adjacent surfaces by maturity.
- `docs/CORE_PATH.md` will later describe the single step-by-step supported journey.
- `README.md` should eventually summarize this document instead of competing with it.
