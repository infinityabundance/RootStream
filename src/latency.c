/*
 * latency.c - Deterministic latency instrumentation
 *
 * This module records per-frame stage timings and prints percentile
 * summaries (p50/p95/p99). The goal is to make performance regressions
 * obvious with minimal runtime overhead.
 */

#include "../include/rootstream.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_u64(const void *a, const void *b) {
    const uint64_t lhs = *(const uint64_t *)a;
    const uint64_t rhs = *(const uint64_t *)b;

    if (lhs < rhs) {
        return -1;
    }
    if (lhs > rhs) {
        return 1;
    }
    return 0;
}

static uint64_t percentile_value(uint64_t *values, size_t count, double percentile) {
    if (count == 0) {
        return 0;
    }

    if (percentile <= 0.0) {
        return values[0];
    }
    if (percentile >= 1.0) {
        return values[count - 1];
    }

    size_t rank = (size_t)ceil(percentile * (double)count) - 1;
    if (rank >= count) {
        rank = count - 1;
    }

    return values[rank];
}

static void fill_metric_samples(const latency_stats_t *stats,
                                uint64_t *capture,
                                uint64_t *encode,
                                uint64_t *send,
                                uint64_t *total) {
    size_t sample_count = stats->count;
    bool wrapped = stats->count >= stats->capacity;

    for (size_t i = 0; i < sample_count; i++) {
        size_t index = wrapped ? (stats->cursor + i) % stats->capacity : i;
        latency_sample_t sample = stats->samples[index];

        capture[i] = sample.capture_us;
        encode[i] = sample.encode_us;
        send[i] = sample.send_us;
        total[i] = sample.total_us;
    }
}

static void latency_report(latency_stats_t *stats, uint64_t now_ms) {
    if (!stats->enabled || stats->count == 0) {
        return;
    }

    size_t sample_count = stats->count;

    uint64_t *capture = malloc(sample_count * sizeof(uint64_t));
    uint64_t *encode = malloc(sample_count * sizeof(uint64_t));
    uint64_t *send = malloc(sample_count * sizeof(uint64_t));
    uint64_t *total = malloc(sample_count * sizeof(uint64_t));

    if (!capture || !encode || !send || !total) {
        fprintf(stderr, "ERROR: Latency report skipped (allocation failure)\n");
        free(capture);
        free(encode);
        free(send);
        free(total);
        stats->last_report_ms = now_ms;
        return;
    }

    fill_metric_samples(stats, capture, encode, send, total);

    qsort(capture, sample_count, sizeof(uint64_t), compare_u64);
    qsort(encode, sample_count, sizeof(uint64_t), compare_u64);
    qsort(send, sample_count, sizeof(uint64_t), compare_u64);
    qsort(total, sample_count, sizeof(uint64_t), compare_u64);

    uint64_t cap_p50 = percentile_value(capture, sample_count, 0.50);
    uint64_t cap_p95 = percentile_value(capture, sample_count, 0.95);
    uint64_t cap_p99 = percentile_value(capture, sample_count, 0.99);

    uint64_t enc_p50 = percentile_value(encode, sample_count, 0.50);
    uint64_t enc_p95 = percentile_value(encode, sample_count, 0.95);
    uint64_t enc_p99 = percentile_value(encode, sample_count, 0.99);

    uint64_t send_p50 = percentile_value(send, sample_count, 0.50);
    uint64_t send_p95 = percentile_value(send, sample_count, 0.95);
    uint64_t send_p99 = percentile_value(send, sample_count, 0.99);

    uint64_t total_p50 = percentile_value(total, sample_count, 0.50);
    uint64_t total_p95 = percentile_value(total, sample_count, 0.95);
    uint64_t total_p99 = percentile_value(total, sample_count, 0.99);

    printf("LATENCY: samples=%zu interval=%lums\n", sample_count, stats->report_interval_ms);
    printf("  capture: p50=%luus p95=%luus p99=%luus\n", cap_p50, cap_p95, cap_p99);
    printf("  encode:  p50=%luus p95=%luus p99=%luus\n", enc_p50, enc_p95, enc_p99);
    printf("  send:    p50=%luus p95=%luus p99=%luus\n", send_p50, send_p95, send_p99);
    printf("  total:   p50=%luus p95=%luus p99=%luus\n", total_p50, total_p95, total_p99);

    free(capture);
    free(encode);
    free(send);
    free(total);

    stats->last_report_ms = now_ms;
}

int latency_init(latency_stats_t *stats, size_t capacity, uint64_t report_interval_ms, bool enabled) {
    if (!stats) {
        fprintf(stderr, "ERROR: Latency stats init failed (NULL context)\n");
        return -1;
    }

    memset(stats, 0, sizeof(*stats));
    stats->enabled = enabled;
    stats->capacity = capacity > 0 ? capacity : 1;
    stats->report_interval_ms = report_interval_ms;
    stats->last_report_ms = get_timestamp_ms();

    if (!enabled) {
        return 0;
    }

    stats->samples = calloc(stats->capacity, sizeof(latency_sample_t));
    if (!stats->samples) {
        fprintf(stderr, "ERROR: Latency stats init failed (out of memory)\n");
        stats->enabled = false;
        return -1;
    }

    return 0;
}

void latency_cleanup(latency_stats_t *stats) {
    if (!stats) {
        return;
    }

    free(stats->samples);
    stats->samples = NULL;
    stats->enabled = false;
    stats->capacity = 0;
    stats->count = 0;
    stats->cursor = 0;
    stats->report_interval_ms = 0;
    stats->last_report_ms = 0;
}

void latency_record(latency_stats_t *stats, const latency_sample_t *sample) {
    if (!stats || !stats->enabled || !sample) {
        return;
    }

    stats->samples[stats->cursor] = *sample;
    stats->cursor = (stats->cursor + 1) % stats->capacity;

    if (stats->count < stats->capacity) {
        stats->count++;
    }

    uint64_t now_ms = get_timestamp_ms();
    if (stats->report_interval_ms == 0) {
        return;
    }

    if (now_ms - stats->last_report_ms >= stats->report_interval_ms) {
        latency_report(stats, now_ms);
    }
}
