/*
 * scene_detector.c — Histogram-based scene-change detector implementation
 */

#include "scene_detector.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct scene_detector_s {
    double threshold;
    int warmup_frames;
    uint64_t frame_count;
    double prev_hist[SCENE_HIST_BINS]; /* normalised previous histogram */
    bool has_prev;
};

/* ── Internal helpers ─────────────────────────────────────────────── */

static void compute_histogram(const uint8_t *luma, int width, int height, int stride,
                              double out[SCENE_HIST_BINS]) {
    uint64_t counts[SCENE_HIST_BINS];
    memset(counts, 0, sizeof(counts));

    for (int y = 0; y < height; y++) {
        const uint8_t *row = luma + y * stride;
        for (int x = 0; x < width; x++) {
            int bin = row[x] * SCENE_HIST_BINS / 256;
            if (bin >= SCENE_HIST_BINS)
                bin = SCENE_HIST_BINS - 1;
            counts[bin]++;
        }
    }

    double total = (double)(width * height);
    for (int i = 0; i < SCENE_HIST_BINS; i++) {
        out[i] = (total > 0.0) ? ((double)counts[i] / total) : 0.0;
    }
}

/* Bhattacharyya-inspired L1 distance between two normalised histograms */
static double histogram_diff(const double a[SCENE_HIST_BINS], const double b[SCENE_HIST_BINS]) {
    double diff = 0.0;
    for (int i = 0; i < SCENE_HIST_BINS; i++) {
        double d = a[i] - b[i];
        diff += (d < 0.0) ? -d : d;
    }
    return diff / 2.0; /* normalise to [0, 1] */
}

/* ── Public API ───────────────────────────────────────────────────── */

scene_detector_t *scene_detector_create(const scene_config_t *config) {
    scene_detector_t *det = calloc(1, sizeof(*det));
    if (!det)
        return NULL;

    if (config) {
        det->threshold = config->threshold;
        det->warmup_frames = config->warmup_frames;
    } else {
        det->threshold = 0.35;
        det->warmup_frames = 2;
    }

    det->has_prev = false;
    det->frame_count = 0;
    return det;
}

void scene_detector_destroy(scene_detector_t *det) {
    free(det);
}

void scene_detector_reset(scene_detector_t *det) {
    if (!det)
        return;
    det->has_prev = false;
    det->frame_count = 0;
}

uint64_t scene_detector_frame_count(const scene_detector_t *det) {
    return det ? det->frame_count : 0;
}

scene_result_t scene_detector_push(scene_detector_t *det, const uint8_t *luma, int width,
                                   int height, int stride) {
    scene_result_t result = {false, 0.0, 0};

    if (!det || !luma || width <= 0 || height <= 0) {
        return result;
    }

    result.frame_number = det->frame_count++;

    double hist[SCENE_HIST_BINS];
    compute_histogram(luma, width, height, stride, hist);

    if (!det->has_prev || (int)result.frame_number < det->warmup_frames) {
        memcpy(det->prev_hist, hist, sizeof(hist));
        det->has_prev = true;
        return result;
    }

    double diff = histogram_diff(det->prev_hist, hist);
    result.histogram_diff = diff;
    result.scene_changed = (diff >= det->threshold);

    memcpy(det->prev_hist, hist, sizeof(hist));
    return result;
}
