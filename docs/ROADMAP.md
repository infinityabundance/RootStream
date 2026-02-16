# RootStream Roadmap

This roadmap is **not a contract**, but a guide.  
RootStream will evolve as we learn from real-world usage, hardware quirks, and community feedback.

Core principles (these NEVER change):

- Linux-first
- Low latency over convenience
- Minimal dependencies
- No custom cryptography
- Clarity over cleverness

---

## v1.1 – Core Loop & Client MVP

Goal: **End-to-end Linux ↔ Linux streaming that is boringly reliable.**

### Features

- [ ] Linux client MVP
  - [ ] VA-API decode path
  - [ ] Basic frame presentation (SDL2 or DRM/KMS)
  - [ ] Input capture + injection (keyboard/mouse)
  - [ ] Clean connect/disconnect flow

- [ ] Deterministic latency instrumentation
  - [x] Host-side timestamps per stage (capture → encode → send)
  - [x] Optional log mode: p50/p95/p99 latency over time (host loop)
  - [ ] Client-side timestamps per stage (recv → decode → present)
  - [ ] Debug overlay (FPS + latency) on client

- [ ] Identity & pairing
  - [ ] Stable device identity key storage
  - [x] Human-readable peer fingerprint (for tray/CLI)
  - [x] Document backup/restore of identity keys

### Docs

- [x] Update `ARCHITECTURE.md` with client-side architecture
- [x] Add basic troubleshooting for:
  - [x] Client cannot decode
  - [x] Black screen / no frames
  - [x] Input not working

---

## v1.2 – NVIDIA & Audio

Goal: **Make RootStream the obvious choice for NVIDIA users and add solid low-latency audio.**

### NVIDIA path (NVENC/NVDEC)

- [ ] Direct NVENC support on host
  - [ ] Use NVIDIA SDK directly (no VA-API shim)
  - [ ] Latency-focused presets
  - [ ] Fallback/disable on unsupported GPUs

- [ ] NVDEC support on client (optional but ideal)
  - [ ] Detect NVIDIA hardware decode
  - [ ] Benchmark vs VA-API path

### Audio streaming

- [ ] ALSA-based audio capture on host
- [ ] Opus encoding (small frame size: 2.5–5 ms)
- [ ] Audio transport in protocol
- [ ] Client-side Opus decode and playback
- [ ] A/V synchronization (shared clock model)
- [ ] Option to disable audio entirely

### Docs

- [ ] GPU-specific documentation (Intel/AMD/NVIDIA)
- [ ] Audio troubleshooting (desync, stutter, missing audio)

---

## v1.3 – Multi-monitor & polish

Goal: **Handle real desktops, not just single-monitor lab setups.**

- [ ] Multi-monitor selection
  - [ ] Enumerate outputs/CRTCs
  - [ ] Choose which monitor to stream
  - [ ] Document behavior on hotplug / unplug

- [ ] Better host UX
  - [ ] Tray UI for monitor selection
  - [ ] Quick toggle for “stream full desktop” vs “single monitor”

- [ ] Stability & polish
  - [ ] Fuzz-ish testing of packet handling
  - [ ] Crash-resilient service mode

---

## v2.0 – Protocol as a Platform

Goal: **Turn RootStream into a clean reference implementation and protocol that others can target.**

- [ ] Formalize wire protocol in `PROTOCOL.md`
  - [ ] Message types (control, video, audio, input)
  - [ ] Version negotiation
  - [ ] Compatibility rules

- [ ] Headless / automation mode
  - [ ] `rootstream host --headless --card /dev/dri/cardX`
  - [ ] Use cases: render nodes, CI visualizations, remote GPU workloads

- [ ] Windows client (client-only, no host)
  - [ ] Decode + input only
  - [ ] LAN/VPN use, no cloud

- [ ] Experimental hooks (documented)
  - [ ] Pre-encode/post-decode hooks
  - [ ] Input filters (for power users only)

---

## Out-of-Scope (for now)

These are **explicitly not priorities**, to protect RootStream’s identity:

- User accounts, central servers, or any “cloud” control-plane
- In-browser/WebRTC client
- “Install-everything for everyone” GUI installer
- Generic remote desktop for every OS under the sun

RootStream focuses on:

> Secure, low-latency, Linux-centric game streaming  
> with transparent architecture and minimal moving parts.
