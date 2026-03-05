/*
 * mx_registry.c — Named gauge registry implementation
 *
 * DESIGN RATIONALE
 * ----------------
 * The registry is a flat array of mx_gauge_t structs rather than a hash
 * map or linked list.  This is intentional:
 *
 *   - MX_MAX_GAUGES = 64 is small enough that a linear scan on register
 *     or lookup is negligible (typically called once at startup / once
 *     per snapshot, not per-frame).
 *   - No dynamic allocation per gauge: the entire registry is one calloc().
 *   - Pointer stability: gauges are embedded in the array, so the pointer
 *     returned by mx_registry_register() remains valid for the lifetime
 *     of the registry.  Callers cache the pointer and avoid repeated
 *     lookups on hot paths.
 *
 * Duplicate rejection:
 *   Two scans are required: one to reject duplicates, one to find a free
 *   slot.  This is correct but O(2N).  For 64 gauges the cost is trivial
 *   and the two-pass design is clearer than a single-pass with a saved
 *   index that must handle the "found duplicate at i=10, free slot at
 *   i=20" case.
 *
 * snapshot_all copies by value (not by pointer):
 *   Callers call snapshot_all once to take a coherent read of all gauges
 *   for export/logging.  Returning copies prevents the caller from
 *   accidentally mutating live gauges via the snapshot array.
 */

#include "mx_registry.h"
#include <stdlib.h>
#include <string.h>

/* ── internal struct ──────────────────────────────────────────────── */

struct mx_registry_s {
    mx_gauge_t gauges[MX_MAX_GAUGES];  /* flat gauge array, zero-initialised */
    int        count;                  /* number of currently registered gauges */
};

/* ── lifecycle ────────────────────────────────────────────────────── */

mx_registry_t *mx_registry_create(void) {
    /* calloc zero-initialises: all gauges start with in_use=0, so the
     * register path correctly identifies every slot as free initially. */
    return calloc(1, sizeof(mx_registry_t));
}

void mx_registry_destroy(mx_registry_t *r) {
    /* The registry does not own any dynamically allocated per-gauge
     * memory — only the registry struct itself is freed here. */
    free(r);
}

int mx_registry_count(const mx_registry_t *r) {
    return r ? r->count : 0;
}

/* ── registration ─────────────────────────────────────────────────── */

mx_gauge_t *mx_registry_register(mx_registry_t *r, const char *name) {
    if (!r || !name || name[0] == '\0' || r->count >= MX_MAX_GAUGES)
        return NULL;

    /* Pass 1: reject duplicates.
     * Duplicate names would create two gauges with the same label,
     * making dashboard queries ambiguous ("which latency_us?"). */
    for (int i = 0; i < MX_MAX_GAUGES; i++)
        if (r->gauges[i].in_use &&
            strncmp(r->gauges[i].name, name, MX_GAUGE_NAME_MAX) == 0)
            return NULL;

    /* Pass 2: find the first free slot.
     * Slots can be freed externally only by zeroing in_use — this
     * registry does not expose a deregister API because metrics are
     * expected to be registered once at startup and live for the
     * duration of the process. */
    for (int i = 0; i < MX_MAX_GAUGES; i++) {
        if (!r->gauges[i].in_use) {
            if (mx_gauge_init(&r->gauges[i], name) != 0) return NULL;
            r->count++;
            return &r->gauges[i];  /* pointer into the array — stable */
        }
    }
    return NULL;  /* should not reach here: count guard above prevents this */
}

/* ── lookup ───────────────────────────────────────────────────────── */

mx_gauge_t *mx_registry_lookup(mx_registry_t *r, const char *name) {
    if (!r || !name) return NULL;
    /* Linear scan — acceptable for ≤64 gauges.  Callers on hot paths
     * should cache the returned pointer rather than calling lookup
     * every frame. */
    for (int i = 0; i < MX_MAX_GAUGES; i++)
        if (r->gauges[i].in_use &&
            strncmp(r->gauges[i].name, name, MX_GAUGE_NAME_MAX) == 0)
            return &r->gauges[i];
    return NULL;
}

/* ── snapshot ─────────────────────────────────────────────────────── */

int mx_registry_snapshot_all(const mx_registry_t *r,
                              mx_gauge_t          *out,
                              int                  max_out) {
    if (!r || !out || max_out <= 0) return 0;

    int n = 0;
    /* Copy all in-use gauges into the caller's array.
     * Copying by value means the caller gets a consistent snapshot of
     * all gauge values at this instant; subsequent gauge mutations do
     * not retroactively change the snapshot. */
    for (int i = 0; i < MX_MAX_GAUGES && n < max_out; i++)
        if (r->gauges[i].in_use) out[n++] = r->gauges[i];
    return n;
}

