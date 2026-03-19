# RootStream Known Issues

This document tracks known issues in the current supported product path.
See [`docs/RELEASE_PROCESS.md`](RELEASE_PROCESS.md) for severity definitions.

---

## Active Known Issues

### P1 — High

| ID | Summary | Workaround | Tracking |
|---|---|---|---|
| KI-001 | Host process may segfault on startup in environments without DRM hardware (e.g., CI, containers) | Use `--headless` mode or ensure a real display is connected | Phase 99 |

### P2 — Medium

| ID | Summary | Workaround | Tracking |
|---|---|---|---|
| KI-002 | mDNS discovery requires Avahi daemon to be running; fails silently otherwise | Use `--peer-add IP:PORT` for manual peer entry | Phase 99 |
| KI-003 | GTK3 system tray is not started when building with `HEADLESS=1`; tray icon is unavailable | Use `--service` mode or CLI commands directly | By design |
| KI-004 | Recording quality preset options are documented but `archival` preset may exceed real-time encoding speed on older hardware | Use `fast` or `balanced` preset | Phase 17 |

### P3 — Low

| ID | Summary | Workaround | Tracking |
|---|---|---|---|
| KI-005 | `QUICKSTART.md` dependency instructions target Arch Linux only | See `docs/BUILD_VALIDATION.md` for distro-specific instructions | Phase 99 |
| KI-006 | `docs/SECURITY.md` references a `PHASE21_SUMMARY.md` planning document that no longer exists | See `docs/THREAT_MODEL.md` for current security documentation | Phase 105 |

---

## Resolved Issues (Recent)

| ID | Summary | Fixed in |
|---|---|---|
| FX-001 | `audio_playback_write_*` functions used non-const `int16_t *` inconsistently with PipeWire implementation | Phase 99 (build fix) |
| FX-002 | `FRAME_FORMAT_NV12` and `FRAME_FORMAT_RGBA` constants were referenced but not defined | Phase 99 (build fix) |
| FX-003 | `src/client_session.c` and PipeWire audio sources were missing from the root Makefile | Phase 99 (build fix) |
| FX-004 | `rootstream_ctx_t` was missing `peer_host`, `peer_port`, and `current_audio` fields | Phase 99 (build fix) |
| FX-005 | `settings_t` was missing `audio_channels` and `audio_sample_rate` fields | Phase 99 (build fix) |

---

## Reporting New Issues

See [CONTRIBUTING.md](../CONTRIBUTING.md) for how to file a bug report with
the required diagnostic information.
