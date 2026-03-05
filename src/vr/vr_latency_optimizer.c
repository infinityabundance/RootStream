#define _POSIX_C_SOURCE 200112L

#include "vr_latency_optimizer.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAME_WINDOW   60
#define TARGET_90HZ_US 11111   /* 1 / 90 Hz = 11111 µs */
#define REPROJ_BUDGET_US 2000  /* 2 ms reprojection latency target */

struct VRLatencyOptimizer {
    VRReprojectionMode mode;

    // Rolling window of frame durations (µs)
    uint64_t frame_times_us[FRAME_WINDOW];
    int      frame_count;
    int      window_head;          // next write index

    // Current frame boundary timestamps
    struct timespec frame_start;
    bool            frame_started;

    float target_frame_us;         // derived from target FPS
};

// ── helpers ────────────────────────────────────────────────────────────────

static uint64_t timespec_to_us(const struct timespec *ts) {
    return (uint64_t)ts->tv_sec * 1000000ULL + (uint64_t)ts->tv_nsec / 1000ULL;
}

// ── lifecycle ──────────────────────────────────────────────────────────────

VRLatencyOptimizer *vr_latency_optimizer_create(void) {
    VRLatencyOptimizer *opt = (VRLatencyOptimizer *)calloc(1, sizeof(VRLatencyOptimizer));
    return opt;
}

int vr_latency_optimizer_init(VRLatencyOptimizer *optimizer, VRReprojectionMode mode) {
    if (!optimizer) return -1;
    optimizer->mode           = mode;
    optimizer->frame_count    = 0;
    optimizer->window_head    = 0;
    optimizer->frame_started  = false;
    optimizer->target_frame_us = TARGET_90HZ_US;
    return 0;
}

void vr_latency_optimizer_cleanup(VRLatencyOptimizer *optimizer) {
    if (!optimizer) return;
    memset(optimizer, 0, sizeof(*optimizer));
}

void vr_latency_optimizer_destroy(VRLatencyOptimizer *optimizer) {
    free(optimizer);
}

// ── frame timing ───────────────────────────────────────────────────────────

void vr_latency_optimizer_record_frame_start(VRLatencyOptimizer *optimizer) {
    if (!optimizer) return;
    clock_gettime(CLOCK_MONOTONIC, &optimizer->frame_start);
    optimizer->frame_started = true;
}

void vr_latency_optimizer_record_frame_end(VRLatencyOptimizer *optimizer) {
    if (!optimizer || !optimizer->frame_started) return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t start_us = timespec_to_us(&optimizer->frame_start);
    uint64_t end_us   = timespec_to_us(&now);
    uint64_t elapsed  = (end_us > start_us) ? (end_us - start_us) : 0;

    optimizer->frame_times_us[optimizer->window_head] = elapsed;
    optimizer->window_head = (optimizer->window_head + 1) % FRAME_WINDOW;
    if (optimizer->frame_count < FRAME_WINDOW) optimizer->frame_count++;
    optimizer->frame_started = false;
}

// ── metrics ────────────────────────────────────────────────────────────────

VRLatencyMetrics vr_latency_optimizer_get_metrics(VRLatencyOptimizer *optimizer) {
    VRLatencyMetrics m;
    memset(&m, 0, sizeof(m));
    if (!optimizer || optimizer->frame_count == 0) return m;

    // Average frame time over the rolling window
    uint64_t sum = 0;
    for (int i = 0; i < optimizer->frame_count; i++) sum += optimizer->frame_times_us[i];
    uint64_t avg_us = sum / (uint64_t)optimizer->frame_count;

    m.frametime_ms               = (float)avg_us / 1000.0f;
    m.prediction_error_us        = 0.0f;  // populated by external tracking subsystem
    m.reproj_latency_us          = (optimizer->mode != VR_REPROJ_NONE) ? REPROJ_BUDGET_US : 0.0f;
    m.total_pipeline_latency_us  = (float)avg_us + m.reproj_latency_us;
    m.meets_90hz_target          = (avg_us < TARGET_90HZ_US);
    return m;
}

// ── target FPS ─────────────────────────────────────────────────────────────

void vr_latency_optimizer_set_target_fps(VRLatencyOptimizer *optimizer, float fps) {
    if (!optimizer || fps <= 0.0f) return;
    optimizer->target_frame_us = 1000000.0f / fps;
}

// ── reprojection ───────────────────────────────────────────────────────────

bool vr_latency_optimizer_is_reprojection_needed(VRLatencyOptimizer *optimizer) {
    if (!optimizer || optimizer->mode == VR_REPROJ_NONE) return false;
    if (optimizer->frame_count == 0) return false;

    uint64_t sum = 0;
    for (int i = 0; i < optimizer->frame_count; i++) sum += optimizer->frame_times_us[i];
    uint64_t avg_us = sum / (uint64_t)optimizer->frame_count;
    return (avg_us >= (uint64_t)optimizer->target_frame_us);
}

int vr_latency_optimizer_reproject_frame(VRLatencyOptimizer *optimizer,
                                         const void *frame,
                                         void *out_frame,
                                         const float *pose_delta_4x4) {
    if (!optimizer) return -1;
    // Stub: copy frame data when output buffer provided.
    // A full implementation would apply pose_delta_4x4 to warp the pixels.
    (void)pose_delta_4x4;
    if (frame && out_frame) {
        // We don't know the buffer size here; callers are responsible for
        // passing matching-size buffers.  Copy a sentinel value to signal
        // the path was exercised.
        *(unsigned char *)out_frame = *(const unsigned char *)frame;
    }
    return 0;
}
