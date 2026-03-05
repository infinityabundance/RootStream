# RootStream Benchmarks

Performance benchmarks for RootStream components.  Each benchmark is a
standalone C/C++ program that prints results to stdout in the format:

```
BENCH <name>: <metric>=<value> [<metric>=<value> ...]
```

Exit code 0 = within performance target; 1 = target missed.

---

## Benchmarks

### `encode_latency_bench.c`

Measures per-frame raw-encoder latency over 1000 iterations on synthetic
1280×720 NV12 frames.

**Build & run:**
```bash
gcc -O2 -o build/encode_latency_bench benchmarks/encode_latency_bench.c \
    -Iinclude && ./build/encode_latency_bench
```

**Expected output:**
```
BENCH encode_raw: min=Xus avg=Xus max=Xus
```

**Target:** avg < 5 000 µs

---

### `network_throughput_bench.c`

Creates a loopback TCP connection and transfers 10 MB to measure kernel
TCP throughput and first-chunk latency.

**Build & run:**
```bash
gcc -O2 -o build/network_throughput_bench \
    benchmarks/network_throughput_bench.c -lpthread && \
    ./build/network_throughput_bench
```

**Expected output:**
```
BENCH tcp_loopback: throughput=X MB/s latency=Xus
```

**Target:** throughput ≥ 100 MB/s

---

### `vulkan_renderer_bench.cpp`

Measures Vulkan frame-upload latency for 1080p and 4K NV12 frames, and
ring-buffer push/pop throughput.

**Build & run (requires Vulkan SDK):**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target vulkan_renderer_bench
./build/benchmarks/vulkan_renderer_bench
```

**Expected output:**
```
BENCH bench_frame_upload_1080p: min=Xus avg=Xus max=Xus
BENCH bench_frame_upload_4k:    min=Xus avg=Xus max=Xus
BENCH bench_ring_buffer_throughput: min=Xus avg=Xus max=Xus
```

**Target:** avg 1080p upload < 2 000 µs

---

## Running All Benchmarks

```bash
# Quick helper via Make (if target exists)
make benchmarks

# Or build individually as shown above
```

## Performance Targets Summary

| Benchmark              | Metric        | Target         |
|------------------------|---------------|----------------|
| `encode_latency_bench` | avg latency   | < 5 000 µs     |
| `network_throughput`   | throughput    | ≥ 100 MB/s     |
| `vulkan_renderer`      | 1080p upload  | < 2 000 µs avg |
