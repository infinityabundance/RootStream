/**
 * @file vulkan_renderer_bench.cpp
 * @brief Benchmark for Vulkan frame upload and ring buffer throughput
 *
 * Measures the per-frame latency (min/avg/max in microseconds) of:
 *   - bench_frame_upload_1080p  – NV12 1920×1080 upload + render + present
 *   - bench_frame_upload_4k     – NV12 3840×2160 upload + render + present
 *   - bench_ring_buffer_throughput – push/pop pairs through the ring buffer
 *
 * Each benchmark runs 1000 iterations.  Results are printed to stdout in the
 * format:
 *   BENCH name: min=Xus avg=Xus max=Xus
 *
 * Return value: 0 if the average 1080p upload latency is < 2000 µs (2 ms
 * target), 1 otherwise.
 */

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <vector>

extern "C" {
#include "clients/kde-plasma-client/src/renderer/vulkan_renderer.h"
#include "clients/kde-plasma-client/src/frame_ring_buffer.h"
}

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using US        = std::chrono::microseconds;

static constexpr int ITERATIONS = 1000;

/* ── Timing helpers ─────────────────────────────────────────────────────── */

static inline TimePoint now() { return Clock::now(); }
static inline long elapsed_us(TimePoint start)
{
    return static_cast<long>(
        std::chrono::duration_cast<US>(now() - start).count());
}

struct BenchResult {
    long min_us;
    long avg_us;
    long max_us;
};

static BenchResult summarise(const std::vector<long> &samples)
{
    BenchResult r{};
    r.min_us = *std::min_element(samples.begin(), samples.end());
    r.max_us = *std::max_element(samples.begin(), samples.end());
    long sum = std::accumulate(samples.begin(), samples.end(), 0L);
    r.avg_us = sum / static_cast<long>(samples.size());
    return r;
}

static void print_result(const char *name, const BenchResult &r)
{
    std::printf("BENCH %s: min=%ldus avg=%ldus max=%ldus\n",
                name, r.min_us, r.avg_us, r.max_us);
}

/* ── Synthetic NV12 frame builder ──────────────────────────────────────── */

static uint8_t *make_nv12(uint32_t w, uint32_t h)
{
    size_t sz = static_cast<size_t>(w) * h * 3 / 2;
    uint8_t *buf = static_cast<uint8_t *>(std::malloc(sz));
    if (!buf) return nullptr;
    std::memset(buf,                          0x10, (size_t)w * h);
    std::memset(buf + (size_t)w * h,          0x80, (size_t)w * (h / 2));
    return buf;
}

static void fill_frame(frame_t *f, uint8_t *buf, uint32_t w, uint32_t h)
{
    std::memset(f, 0, sizeof *f);
    f->data         = buf;
    f->size         = static_cast<uint32_t>((size_t)w * h * 3 / 2);
    f->width        = w;
    f->height       = h;
    f->format       = FRAME_FORMAT_NV12;
    f->timestamp_us = 0;
    f->is_keyframe  = false;
}

/* ── bench_frame_upload_1080p ──────────────────────────────────────────── */

static BenchResult bench_frame_upload_1080p(vulkan_context_t *ctx)
{
    const uint32_t W = 1920, H = 1080;
    uint8_t *buf = make_nv12(W, H);
    frame_t  frame;
    fill_frame(&frame, buf, W, H);

    std::vector<long> samples;
    samples.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        frame.timestamp_us = static_cast<uint64_t>(i);
        TimePoint t = now();
        if (ctx) {
            vulkan_upload_frame(ctx, &frame);
            vulkan_render(ctx);
            vulkan_present(ctx);
        }
        samples.push_back(elapsed_us(t));
    }

    std::free(buf);
    return summarise(samples);
}

/* ── bench_frame_upload_4k ─────────────────────────────────────────────── */

static BenchResult bench_frame_upload_4k(vulkan_context_t *ctx)
{
    const uint32_t W = 3840, H = 2160;
    uint8_t *buf = make_nv12(W, H);
    if (!buf) {
        BenchResult r{};
        return r;
    }
    frame_t frame;
    fill_frame(&frame, buf, W, H);

    std::vector<long> samples;
    samples.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        frame.timestamp_us = static_cast<uint64_t>(i);
        TimePoint t = now();
        if (ctx) {
            vulkan_upload_frame(ctx, &frame);
            vulkan_render(ctx);
            vulkan_present(ctx);
        }
        samples.push_back(elapsed_us(t));
    }

    std::free(buf);
    return summarise(samples);
}

/* ── bench_ring_buffer_throughput ──────────────────────────────────────── */

static BenchResult bench_ring_buffer_throughput(void)
{
    frame_ring_buffer_t rb;
    frame_ring_buffer_init(&rb);

    const uint32_t W = 320, H = 240;
    uint8_t y[320 * 240], uv[320 * 120];
    std::memset(y,  0x10, sizeof y);
    std::memset(uv, 0x80, sizeof uv);

    std::vector<long> samples;
    samples.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        TimePoint t = now();

        /* Drain any leftover frames so push can't fail */
        {
            frame_t tmp;
            while (frame_ring_buffer_pop(&rb, &tmp) == 0) {}
        }

        frame_ring_buffer_push(&rb, y, uv, W, H, static_cast<uint64_t>(i));
        frame_t out;
        frame_ring_buffer_pop(&rb, &out);

        samples.push_back(elapsed_us(t));
    }

    frame_ring_buffer_cleanup(&rb);
    return summarise(samples);
}

/* ── main ──────────────────────────────────────────────────────────────── */

int main(void)
{
    std::printf("Vulkan Renderer Benchmark (%d iterations each)\n\n",
                ITERATIONS);

    vulkan_context_t *ctx = vulkan_init(NULL);
    if (!ctx) {
        std::printf("NOTE: No GPU detected – upload benchmarks will measure "
                    "stub overhead only.\n\n");
    }

    BenchResult r1080p = bench_frame_upload_1080p(ctx);
    print_result("bench_frame_upload_1080p", r1080p);

    BenchResult r4k = bench_frame_upload_4k(ctx);
    print_result("bench_frame_upload_4k", r4k);

    BenchResult rrb = bench_ring_buffer_throughput();
    print_result("bench_ring_buffer_throughput", rrb);

    if (ctx) vulkan_cleanup(ctx);

    /* Pass/fail threshold: average 1080p upload must be < 2000 µs */
    if (r1080p.avg_us < 2000L) {
        std::printf("\nRESULT: PASS (avg 1080p upload %ldus < 2000us target)\n",
                    r1080p.avg_us);
        return 0;
    } else {
        std::printf("\nRESULT: FAIL (avg 1080p upload %ldus >= 2000us target)\n",
                    r1080p.avg_us);
        return 1;
    }
}
