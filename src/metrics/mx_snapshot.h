/*
 * mx_snapshot.h — Metrics Exporter: timestamped snapshot
 *
 * A snapshot captures all gauge values at a single point in time and
 * bundles them with a wall-clock timestamp for log/telemetry export.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_MX_SNAPSHOT_H
#define ROOTSTREAM_MX_SNAPSHOT_H

#include "mx_gauge.h"
#include "mx_registry.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A single point-in-time metrics snapshot */
typedef struct {
    uint64_t    timestamp_us;              /**< Wall-clock capture time (µs) */
    mx_gauge_t  gauges[MX_MAX_GAUGES];    /**< Captured gauge array */
    int         gauge_count;               /**< Number of valid entries */
} mx_snapshot_t;

/**
 * mx_snapshot_init — zero-initialise a snapshot
 *
 * @return 0 on success, -1 on NULL
 */
int mx_snapshot_init(mx_snapshot_t *s);

/**
 * mx_snapshot_dump — copy snapshot gauges into a flat caller buffer
 *
 * Copies up to @max_out entries from s->gauges[0..gauge_count-1]
 * into @out.
 *
 * @param s        Source snapshot
 * @param out      Caller-allocated mx_gauge_t array
 * @param max_out  Capacity of @out
 * @return         Number of entries written, or -1 on NULL
 */
int mx_snapshot_dump(const mx_snapshot_t *s,
                     mx_gauge_t          *out,
                     int                  max_out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MX_SNAPSHOT_H */
