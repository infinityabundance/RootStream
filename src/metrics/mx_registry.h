/*
 * mx_registry.h — Metrics Exporter: named gauge registry
 *
 * Maintains up to MX_MAX_GAUGES named gauges.  Gauges are registered
 * by name and looked up by name.  `mx_registry_snapshot_all()` copies
 * all registered gauges into a caller-supplied array for atomic export.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_MX_REGISTRY_H
#define ROOTSTREAM_MX_REGISTRY_H

#include <stddef.h>

#include "mx_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MX_MAX_GAUGES 64

/** Opaque gauge registry */
typedef struct mx_registry_s mx_registry_t;

/**
 * mx_registry_create — allocate registry
 *
 * @return Non-NULL, or NULL on OOM
 */
mx_registry_t *mx_registry_create(void);

/**
 * mx_registry_destroy — free registry (does NOT free gauge data)
 */
void mx_registry_destroy(mx_registry_t *r);

/**
 * mx_registry_register — add a gauge to the registry
 *
 * @param r     Registry
 * @param name  Unique gauge name (≤ MX_GAUGE_NAME_MAX-1 chars)
 * @return      Pointer to the registered gauge (owned by registry), or
 *              NULL if full or name already registered
 */
mx_gauge_t *mx_registry_register(mx_registry_t *r, const char *name);

/**
 * mx_registry_lookup — find a gauge by name
 *
 * @return Pointer to gauge (owned by registry), or NULL if not found
 */
mx_gauge_t *mx_registry_lookup(mx_registry_t *r, const char *name);

/**
 * mx_registry_count — current number of registered gauges
 */
int mx_registry_count(const mx_registry_t *r);

/**
 * mx_registry_snapshot_all — copy all gauge values into @out
 *
 * @param r        Registry
 * @param out      Caller-allocated array of mx_gauge_t
 * @param max_out  Size of out array
 * @return         Number of gauges copied
 */
int mx_registry_snapshot_all(const mx_registry_t *r, mx_gauge_t *out, int max_out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MX_REGISTRY_H */
