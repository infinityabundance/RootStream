/*
 * sr_route.c — Signal router implementation
 *
 * DESIGN RATIONALE
 * ----------------
 * The signal router decouples signal *producers* (health monitors, codec
 * error detectors, congestion detectors) from signal *consumers*
 * (event bus publishers, UI alert handlers, logging sinks).
 *
 * Matching algorithm:
 *   A signal matches route i when:
 *     (signal->signal_id & route[i].src_mask) == route[i].match_id
 *   This bitmask approach allows:
 *     - Exact match:   src_mask = 0xFFFFFFFF, match_id = <specific_id>
 *     - Wildcard:      src_mask = 0,          match_id = 0  (matches all)
 *     - Group match:   src_mask = 0xFFFF0000, match_id = 0x00010000
 *       (matches all signals in the 0x0001xxxx range)
 *
 * Why not string-based topic routing?
 *   Integer bitmask matching is O(1) per route per signal with no memory
 *   allocation.  String topics (MQTT-style) require strncmp or a trie,
 *   adding latency on signal-heavy paths (e.g., 60fps health probes).
 *
 * Delivery semantics:
 *   A signal may match MULTIPLE routes and is delivered to all of them.
 *   This is "fanout" semantics (not unicast).  The return value of
 *   sr_router_route() is the delivery count, not a boolean.
 *
 * Thread-safety: NOT thread-safe.  External locking required for
 * concurrent route manipulation and delivery.
 */

#include "sr_route.h"

#include <stdlib.h>
#include <string.h>

/* ── route entry (internal) ───────────────────────────────────────── */

typedef struct {
    uint32_t src_mask;        /* applied to signal_id before comparison */
    uint32_t match_id;        /* expected value after masking            */
    sr_filter_fn filter_fn;   /* optional per-signal predicate (may be NULL) */
    sr_deliver_fn deliver;    /* mandatory delivery callback             */
    void *user;               /* opaque pointer forwarded to callbacks   */
    bool in_use;              /* slot is occupied (false = free)         */
    sr_route_handle_t handle; /* unique, monotonically increasing ID     */
} route_entry_t;

/* ── router struct ────────────────────────────────────────────────── */

struct sr_route_s {
    route_entry_t routes[SR_MAX_ROUTES];
    int count;                     /* active route count */
    sr_route_handle_t next_handle; /* next handle to assign (never reused)  */
};

/* ── lifecycle ────────────────────────────────────────────────────── */

sr_router_t *sr_router_create(void) {
    /* calloc zero-initialises: all routes start with in_use=false,
     * count=0, next_handle=0. */
    return calloc(1, sizeof(sr_router_t));
}

void sr_router_destroy(sr_router_t *r) {
    free(r);
}
int sr_router_count(const sr_router_t *r) {
    return r ? r->count : 0;
}

sr_route_handle_t sr_router_add_route(sr_router_t *r, uint32_t src_mask, uint32_t match_id,
                                      sr_filter_fn filter_fn, sr_deliver_fn deliver, void *user) {
    /* deliver must be non-NULL: a route with no callback is useless and
     * would silently swallow matching signals.  filter_fn may be NULL
     * (no filtering = deliver all matching signals). */
    if (!r || !deliver || r->count >= SR_MAX_ROUTES)
        return SR_INVALID_HANDLE;

    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (!r->routes[i].in_use) {
            r->routes[i].src_mask = src_mask;
            r->routes[i].match_id = match_id;
            r->routes[i].filter_fn = filter_fn;
            r->routes[i].deliver = deliver;
            r->routes[i].user = user;
            r->routes[i].in_use = true;
            /* next_handle is monotonically increasing and never reused.
             * Reuse would break callers that cache a handle for removal
             * after the original route was removed and a new one was added
             * to the same slot. */
            r->routes[i].handle = r->next_handle++;
            r->count++;
            return r->routes[i].handle;
        }
    }
    return SR_INVALID_HANDLE;
}

int sr_router_remove_route(sr_router_t *r, sr_route_handle_t h) {
    if (!r || h < 0)
        return -1;
    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (r->routes[i].in_use && r->routes[i].handle == h) {
            /* memset to zero: clears in_use=false, nulls all pointers.
             * Prevents use-after-free if the router is somehow accessed
             * concurrently (still not safe, but at least won't call a
             * dangling function pointer). */
            memset(&r->routes[i], 0, sizeof(r->routes[i]));
            r->count--;
            return 0;
        }
    }
    return -1; /* handle not found — caller may have already removed it */
}

int sr_router_route(sr_router_t *r, const sr_signal_t *s) {
    if (!r || !s)
        return 0;
    int delivered = 0;

    /* Iterate ALL routes — a signal may match and be delivered to
     * multiple routes simultaneously (fanout semantics).
     * Short-circuit on first match would break multi-subscriber scenarios
     * (e.g., both a logger and an alert system subscribed to the same
     * signal range). */
    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (!r->routes[i].in_use)
            continue;

        /* Bitmask match: test only the bits indicated by src_mask.
         * A src_mask of 0 makes match_id=0 a wildcard (0 & anything == 0). */
        if ((s->signal_id & r->routes[i].src_mask) != r->routes[i].match_id)
            continue;

        /* Optional predicate filter: allows fine-grained routing beyond
         * what the bitmask alone can express (e.g., filter by level range,
         * source_id allow-list, time-of-day, etc.). */
        if (r->routes[i].filter_fn && !r->routes[i].filter_fn(s, r->routes[i].user))
            continue;

        r->routes[i].deliver(s, r->routes[i].user);
        delivered++;
    }
    return delivered;
}
