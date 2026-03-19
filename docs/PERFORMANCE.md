# RootStream Performance Documentation

This document records measured performance baselines, benchmark environments,
and known bottlenecks for the supported RootStream product path.

**Important**: Performance data in this document is measured in CI-class
environments without real GPU hardware. Numbers for VA-API encoding, DRM
capture, and hardware decode will differ on actual gaming hardware.
See the Environment section for caveats.

---

## Performance-Sensitive Surfaces

| Surface | Stage | Key metric | Notes |
|---|---|---|---|
| DRM/KMS framebuffer capture | Capture | Capture latency (µs) | Hardware/driver dependent |
| VA-API H.264/H.265 encoding | Encode | Encode latency (µs) | GPU dependent |
| Raw (software) encoding | Encode | Encode latency (µs) | CPU dependent |
| UDP socket write | Send | Packet send latency (µs) | Network stack |
| Network throughput | Transport | MB/s sustained | Kernel TCP/UDP |
| Packet decode (client) | Decode | Decode latency (µs) | GPU or CPU dependent |
| SDL2 frame presentation | Display | Present latency (µs) | Compositor/vsync |

---

## Benchmarks

Benchmarks live in `benchmarks/` and are standalone C/C++ programs.

### Build and Run

```bash
# Raw encoder latency
gcc -O2 -o build/encode_latency_bench benchmarks/encode_latency_bench.c \
    -Iinclude && ./build/encode_latency_bench

# Network (TCP loopback) throughput
gcc -O2 -o build/network_throughput_bench \
    benchmarks/network_throughput_bench.c -lpthread && \
    ./build/network_throughput_bench
```

### Benchmark: Raw Encoder Latency (`encode_latency_bench.c`)

Measures per-frame raw encoder latency over 1,000 iterations on synthetic
1280×720 NV12 frames. The raw encoder is the software passthrough path
(no VA-API/NVENC).

**Target**: average < 5,000 µs

**Baseline measurement (CI environment, no hardware GPU)**:

```
BENCH encode_raw: min=36us avg=38us max=715us
```

| Metric | Value | Target | Status |
|---|---|---|---|
| Minimum | ~36 µs | — | — |
| Average | ~38 µs | < 5,000 µs | ✅ Pass |
| Maximum | ~715 µs | — | — |

_Note: The raw encoder simply copies frame data without real compression.
Actual encode latency for VA-API H.264 will be higher (typically 2–15ms
depending on GPU and preset)._

### Benchmark: Network Throughput (`network_throughput_bench.c`)

Creates a loopback TCP connection and transfers 10 MB to measure peak
kernel TCP throughput and first-chunk latency.

**Target**: throughput > 100 MB/s, first-chunk latency < 500 µs

**Baseline measurement (CI environment, loopback)**:

```
BENCH tcp_loopback: throughput=2515.1 MB/s latency=43us
```

| Metric | Value | Target | Status |
|---|---|---|---|
| Throughput | ~2,500 MB/s | > 100 MB/s | ✅ Pass |
| First-chunk latency | ~43 µs | < 500 µs | ✅ Pass |

_Note: Loopback TCP throughput is limited by kernel memory bandwidth, not
the NIC. Real LAN throughput is bounded by link speed (125 MB/s on 1Gbps)._

---

## Measurement Environment

Baseline measurements above were taken in GitHub Actions Ubuntu runner (CI)
with no real GPU hardware. Key environment caveats:

| Assumption | CI environment | Real gaming hardware |
|---|---|---|
| GPU | None | Intel/AMD/NVIDIA |
| VA-API encode | Not available | Available (2–15ms per frame) |
| DRM/KMS capture | Not available | Available (0.5–2ms) |
| Network | Kernel loopback | LAN (1Gbps typical) |
| CPU | Shared cloud VM | Dedicated gaming CPU |

---

## Latency Budget (End-to-End Target)

For the canonical path (Linux host → Linux peer on 1Gbps LAN):

| Stage | Target | Notes |
|---|---|---|
| DRM/KMS capture | 0.5–2ms | One framebuffer read |
| VA-API H.264 encode | 2–8ms | GPU-dependent; 10Mbps CBR |
| Network UDP | < 1ms | On 1Gbps LAN |
| Network decode (client) | 2–8ms | GPU-dependent |
| SDL2 display | < 1ms | Vsync-controlled |
| **Total end-to-end** | **< 20ms** | Without vsync wait |

_Note: These are targets, not measured guarantees. A full end-to-end
measurement requires two machines and hardware GPU on both sides.
See the Roadmap for planned benchmark coverage (Phase 104 follow-ups)._

---

## Known Bottlenecks

| Area | Bottleneck | Severity | Next action |
|---|---|---|---|
| DRM capture | `mmap`+`memcpy` copies full framebuffer each frame | Medium | Explore dma-buf zero-copy path |
| VA-API encoding | B-frame reordering adds latency | Low | Force B-frame=0 (already default in latency preset) |
| Network | No UDP pacing; burst at frame boundary | Medium | Phase 102: ABR pacing |
| Client decode | No decode queue depth control | Low | Phase 101: Queue depth cap |

---

## Optimization Follow-Up Queue

The following optimizations are candidates once baseline measurements on
real hardware are established:

1. **DMA-BUF zero-copy capture**: Avoid CPU memcpy by importing the DRM
   framebuffer directly into VA-API as a dmabuf surface.
2. **Encode timestamp loop**: Replace sleep-based loop with VBLANK interrupt
   wait (`drmWaitVBlank`) to align capture with display refresh.
3. **Network pacing**: Send packets at a constant rate instead of bursting
   at end of each frame encode to reduce jitter.
4. **Client-side jitter buffer**: Add a small (1-2 frame) jitter buffer to
   absorb network timing variation without adding sustained latency.

---

## Running Performance Tests in CI

The benchmarks are not currently part of CI (they require real hardware for
meaningful numbers). To add a CI performance gate:

1. Add a benchmark job to `.github/workflows/ci.yml` that runs the
   software-only benchmarks.
2. Compare output against the expected baselines above.
3. Fail the job if average raw-encode latency exceeds 5ms or TCP
   throughput is below 100MB/s.

See `benchmarks/README.md` for full benchmark documentation.
