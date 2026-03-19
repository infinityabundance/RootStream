# RootStream Observability Guide

This document covers logging patterns, metrics, session tracing, and
operational diagnostics for the supported RootStream product path.

For the product scope: [`docs/PRODUCT_CORE.md`](PRODUCT_CORE.md)  
For architecture: [`docs/ARCHITECTURE.md`](ARCHITECTURE.md)  
For troubleshooting: [`docs/TROUBLESHOOTING.md`](TROUBLESHOOTING.md)

---

## Logging

### Log Levels and Prefixes

RootStream uses a consistent prefix pattern for log messages:

| Prefix | Severity | Meaning |
|---|---|---|
| `INFO:` | Informational | Normal operational state changes |
| `WARNING:` | Warning | Degraded mode, fallback engaged, non-fatal condition |
| `ERROR:` | Error | Operation failed; includes `REASON:` and `ACTION:` on the next lines when available |
| `DEBUG:` | Debug (DEBUG=1 only) | Detailed trace output |

Example structured error:
```
ERROR: Failed to open DRM device
REASON: /dev/dri/card0: Permission denied
ACTION: Add user to 'video' group: sudo usermod -aG video $USER
```

### Backend Selection Logging

On startup, the selected backends are logged to stderr:

```
✓ Capture backend:  DRM/KMS (primary)
✓ Encoder backend:  VA-API H.264
✓ Audio capture:    ALSA (primary)
✓ Audio playback:   ALSA (primary)
✓ Discovery:        mDNS/Avahi (primary)
✓ Input:            uinput (primary)
✓ GUI:              GTK3 tray
```

If a fallback is engaged:
```
WARNING: Primary DRM capture failed, trying X11 SHM...
✓ Capture backend:  X11 SHM (fallback)
```

Use `--backend-verbose` for detailed fallback trace output.

### Latency Logging

Enable per-stage latency percentile logging:

```bash
./rootstream host --latency-log --latency-interval 1000
```

Output format (every `--latency-interval` ms):

```
LATENCY [p50/p95/p99 ms]: capture=1.2/2.1/3.4  encode=3.1/5.2/7.8  net_send=0.4/0.9/1.5
```

Latency stages:
- `capture`: DRM/KMS framebuffer read
- `encode`: VA-API or software encode
- `net_send`: UDP or TCP socket write
- `net_recv`: Packet received (client side)
- `decode`: VA-API or software decode
- `present`: SDL2 frame presentation

### AI Logging / Diagnostic Mode

Set `AI_COPILOT_MODE=1` in the environment to enable structured machine-readable
diagnostic output. This is intended for automated test harnesses and CI analysis.

```bash
AI_COPILOT_MODE=1 ./rootstream host 2>diagnostics.log
```

---

## Metrics

### Host-Side Statistics

The host prints a statistics line every 5 seconds during streaming:

```
📊 FPS: 60 | Bitrate: 10.2 Mbps | Frames: 12458/12450 | RTT: 2ms
```

Fields:
- `FPS`: Actual capture rate (should match display refresh rate)
- `Bitrate`: Current encoded output rate
- `Frames`: Captured count / Encoded count (should be close)
- `RTT`: Round-trip time from keepalive (available once peer connects)

### Client-Side Statistics

The client prints per-frame statistics when `--show-stats` is enabled:
```
📊 Decoded: 12450 | Dropped: 0 | Audio buf: 48ms | Display: SDL2
```

### Enabling More Verbose Metrics

| Flag | Effect |
|---|---|
| `--latency-log` | Enable per-stage latency percentile logging |
| `--latency-interval MS` | Set latency log interval (default: 1000ms) |
| `--backend-verbose` | Log detailed backend selection and fallback |
| `DEBUG=1` (build flag) | Enable verbose debug-level output |

---

## Session Tracing

### Session ID

Each session is assigned a random 8-byte hex session ID on connection:

```
INFO: Session established: sid=a3f2b9c1 peer=192.168.1.100:9876
```

The session ID appears in all subsequent session-related log lines for
correlation.

### Connection State Transitions

The connection state is logged as it transitions:

```
INFO: [sid=a3f2b9c1] State: DISCONNECTED → CONNECTING
INFO: [sid=a3f2b9c1] State: CONNECTING → CONNECTED
INFO: [sid=a3f2b9c1] Handshake complete — encryption active
```

States:
- `DISCONNECTED`: No active connection
- `CONNECTING`: Attempting to establish connection
- `CONNECTED`: Active session
- `RECONNECTING`: Auto-reconnect in progress

### Reading Session Traces

To trace a full session:

```bash
./rootstream host 2>&1 | grep "sid="
```

Or filter by state transitions:

```bash
./rootstream host 2>&1 | grep "State:"
```

---

## Reliability Diagnostics

### Common Failure Signals

| Log pattern | Meaning | Next step |
|---|---|---|
| `ERROR: Network init failed` | Socket or port binding failed | Check port availability: `ss -ulpn | grep 9876` |
| `ERROR: DRM capture failed` | Cannot access display | Check `video` group membership; try X11 or dummy fallback |
| `ERROR: VA-API initialization failed` | Hardware encoder unavailable | Check `vainfo`; encoder falls back to software |
| `WARNING: All discovery announce methods failed` | No mDNS or broadcast | Use `--peer-add IP:PORT` for manual connection |
| `ERROR: Decryption failed` | Packet authentication failed | Keys may have changed; regenerate with `--qr` |
| `ERROR: Peer declared dead` | Keepalive timeout | Check network path between host and peer |

### Collecting a Diagnostic Bundle

For bug reports, collect:

```bash
# 1. Verbose output with backend info
./rootstream host --backend-verbose --latency-log 2>&1 | tee rootstream-host.log

# 2. System info
uname -a > system-info.txt
pkg-config --modversion libsodium libva libdrm sdl2 >> system-info.txt 2>&1
vainfo >> system-info.txt 2>&1

# 3. Group membership (DRM/uinput)
groups >> system-info.txt

# Attach rootstream-host.log and system-info.txt to the bug report.
```

---

## Operational Runbook

### Check if the Host is Ready

```bash
./rootstream host --port 9876
# Expected: "✓ All systems ready" + "→ Waiting for connections..."
# If not: check stderr for ERROR lines and use the diagnostics table above
```

### Check Peer Connection

```bash
# Verify the peer code contains '@' (valid format)
./rootstream --qr | grep "RootStream Code:"

# Verify network reachability (from peer machine)
nc -u HOST_IP 9876
```

### Verify Encryption is Active

```bash
# Host log should show:
grep "Handshake complete\|encryption active" rootstream-host.log
```

---

See [`docs/TROUBLESHOOTING.md`](TROUBLESHOOTING.md) for per-component troubleshooting.
