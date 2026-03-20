/*
 * mx_gauge.h — Metrics Exporter: integer gauge
 *
 * A gauge is a single named int64 value that can be set, incremented,
 * decremented, read, and reset independently.  Gauges are light-weight
 * value objects — no heap allocation.
 *
 * Thread-safety: NOT thread-safe (caller provides locking when needed).
 */

#ifndef ROOTSTREAM_MX_GAUGE_H
#define ROOTSTREAM_MX_GAUGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MX_GAUGE_NAME_MAX 48 /**< Maximum gauge name length (incl. NUL) */

/** Named integer gauge */
typedef struct {
    char name[MX_GAUGE_NAME_MAX]; /**< Human-readable identifier */
    int64_t value;                /**< Current value */
    int in_use;                   /**< Non-zero when registered */
} mx_gauge_t;

/**
 * mx_gauge_init — initialise a gauge with a given name
 *
 * @return 0 on success, -1 on NULL or empty name
 */
int mx_gauge_init(mx_gauge_t *g, const char *name);

/** mx_gauge_set — set absolute value */
void mx_gauge_set(mx_gauge_t *g, int64_t v);

/** mx_gauge_add — add delta (may be negative) */
void mx_gauge_add(mx_gauge_t *g, int64_t delta);

/** mx_gauge_get — return current value (0 if g is NULL) */
int64_t mx_gauge_get(const mx_gauge_t *g);

/** mx_gauge_reset — set value back to 0 */
void mx_gauge_reset(mx_gauge_t *g);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MX_GAUGE_H */
