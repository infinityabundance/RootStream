/*
 * sr_route.h — Signal Router: routing table
 *
 * A routing table of up to SR_MAX_ROUTES entries.  Each route matches
 * signals whose (signal_id & src_mask) == match_id, optionally filters
 * by level using a caller-supplied predicate, and invokes a delivery
 * callback when matched.
 *
 * sr_route_signal() scans all active routes; a signal may match and be
 * delivered to multiple routes simultaneously.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_SR_ROUTE_H
#define ROOTSTREAM_SR_ROUTE_H

#include "sr_signal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SR_MAX_ROUTES  32

/** Route handle */
typedef int sr_route_handle_t;
#define SR_INVALID_HANDLE  (-1)

/** Optional filter predicate — return true to allow delivery */
typedef bool (*sr_filter_fn)(const sr_signal_t *s, void *user);

/** Delivery callback */
typedef void (*sr_deliver_fn)(const sr_signal_t *s, void *user);

/** Opaque signal router */
typedef struct sr_route_s sr_router_t;

/**
 * sr_router_create — allocate router
 *
 * @return Non-NULL, or NULL on OOM
 */
sr_router_t *sr_router_create(void);

/**
 * sr_router_destroy — free router
 */
void sr_router_destroy(sr_router_t *r);

/**
 * sr_router_add_route — register a delivery route
 *
 * A signal matches this route when:
 *   (signal->signal_id & src_mask) == match_id
 * AND filter_fn(signal, user) returns true (or filter_fn is NULL).
 *
 * @param r         Router
 * @param src_mask  Bitmask applied to signal_id before comparison
 * @param match_id  Expected value after masking
 * @param filter_fn Optional per-signal predicate (may be NULL)
 * @param deliver   Delivery callback (must not be NULL)
 * @param user      Passed through to filter_fn and deliver
 * @return          Route handle, or SR_INVALID_HANDLE if table full
 */
sr_route_handle_t sr_router_add_route(sr_router_t   *r,
                                       uint32_t       src_mask,
                                       uint32_t       match_id,
                                       sr_filter_fn   filter_fn,
                                       sr_deliver_fn  deliver,
                                       void          *user);

/**
 * sr_router_remove_route — remove a route by handle
 *
 * @return 0 on success, -1 if not found
 */
int sr_router_remove_route(sr_router_t *r, sr_route_handle_t h);

/**
 * sr_router_route — route a signal through the table
 *
 * @return Number of routes the signal was delivered to
 */
int sr_router_route(sr_router_t *r, const sr_signal_t *s);

/**
 * sr_router_count — number of active routes
 */
int sr_router_count(const sr_router_t *r);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SR_ROUTE_H */
