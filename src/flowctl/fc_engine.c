/*
 * fc_engine.c — Token-bucket flow control engine
 *
 * DESIGN RATIONALE
 * ----------------
 * A token bucket is the simplest correct mechanism for bounding the burst
 * size of a data sender.  The "bucket" holds up to window_bytes tokens
 * (bytes of send credit).  Each successful call to fc_engine_consume()
 * removes tokens; fc_engine_replenish() adds them back.
 *
 * Why not a leaky-bucket?
 *   A leaky bucket enforces a constant output *rate*.  A token bucket
 *   allows bursting up to the window, which is what TCP and modern ABR
 *   algorithms expect: send as fast as the receiver can absorb, not at
 *   a fixed drip rate.
 *
 * Why credit_step?
 *   Without a minimum replenish increment, a caller could call
 *   replenish(1) one byte at a time, adding CPU overhead and breaking
 *   the minimum-grant assumption.  credit_step enforces that each
 *   replenish event is worth at least one MTU-equivalent.
 *
 * Caller responsibility:
 *   This engine does not own a timer.  The caller decides *when* to
 *   replenish (on an ACK, on a scheduler tick, on bandwidth probe
 *   result, etc.).  This keeps the engine pure and testable.
 *
 * Thread-safety: NOT thread-safe.  See fc_engine.h.
 */

#include "fc_engine.h"

#include <stdlib.h>
#include <string.h>

/* ── internal struct ──────────────────────────────────────────────── */

struct fc_engine_s {
    fc_params_t params; /* immutable copy of caller-supplied config */
    uint32_t credit;    /* current available send credit (bytes)     */
};

/* ── lifecycle ────────────────────────────────────────────────────── */

fc_engine_t *fc_engine_create(const fc_params_t *p) {
    /* Validate: both critical limits must be non-zero.
     * A zero window_bytes would allow infinite credit after replenish.
     * A zero send_budget would start the engine with no credit at all,
     * causing the very first send attempt to stall before any data flows. */
    if (!p || p->window_bytes == 0 || p->send_budget == 0)
        return NULL;

    fc_engine_t *e = malloc(sizeof(*e));
    if (!e)
        return NULL; /* OOM: caller must handle NULL return */

    e->params = *p;             /* snapshot the config; caller may free p */
    e->credit = p->send_budget; /* start with one epoch's worth of credit */
    return e;
}

void fc_engine_destroy(fc_engine_t *e) {
    free(e);
}

/* ── credit management ────────────────────────────────────────────── */

bool fc_engine_can_send(const fc_engine_t *e, uint32_t bytes) {
    /* Non-destructive probe: does NOT change credit.
     * Callers use this to skip building a frame when credit is low,
     * avoiding the cost of encoding only to immediately drop it. */
    if (!e)
        return false;
    return e->credit >= bytes;
}

int fc_engine_consume(fc_engine_t *e, uint32_t bytes) {
    /* Deduct @bytes from available credit.
     * Contract: only call after fc_engine_can_send() returned true.
     * Returning -1 on insufficient credit (rather than clamping to 0)
     * forces the caller to handle the "shouldn't have called this"
     * programming error explicitly. */
    if (!e || e->credit < bytes)
        return -1;
    e->credit -= bytes;
    return 0;
}

uint32_t fc_engine_replenish(fc_engine_t *e, uint32_t bytes) {
    if (!e)
        return 0;

    uint32_t cap = e->params.window_bytes;

    /* Enforce the minimum credit grant (credit_step).
     * If the caller passes fewer bytes than credit_step (e.g., a
     * small ACK for just 10 bytes), we still grant credit_step.
     * This prevents micro-grants that would never unblock a full MTU. */
    uint32_t added = (bytes < e->params.credit_step) ? e->params.credit_step : bytes;

    /* Cap at window_bytes to prevent unbounded credit accumulation.
     * Accumulation would allow a stalled sender to burst far more than
     * the network path can sustain once it resumes. */
    e->credit = (e->credit + added > cap) ? cap : e->credit + added;
    return e->credit;
}

uint32_t fc_engine_credit(const fc_engine_t *e) {
    /* Safe read — returns 0 on NULL rather than crashing the caller's
     * logging/status path, which might query credit just for display. */
    return e ? e->credit : 0;
}

void fc_engine_reset(fc_engine_t *e) {
    /* Restore to send_budget (not window_bytes).
     * window_bytes is the in-flight cap; send_budget is the per-epoch
     * grant.  Resetting to window_bytes would allow an immediate burst
     * equal to the entire receive window, likely causing congestion. */
    if (e)
        e->credit = e->params.send_budget;
}
