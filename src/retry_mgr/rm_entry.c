/*
 * rm_entry.c — Retry entry implementation
 */

#include "rm_entry.h"

#include <string.h>

int rm_entry_init(rm_entry_t *e, uint64_t request_id, uint64_t now_us, uint64_t base_delay_us,
                  uint32_t max_attempts) {
    if (!e || max_attempts == 0)
        return -1;
    memset(e, 0, sizeof(*e));
    e->request_id = request_id;
    e->max_attempts = max_attempts;
    e->base_delay_us = base_delay_us;
    e->next_retry_us = now_us + base_delay_us; /* first fire after base_delay_us */
    e->in_use = true;
    return 0;
}

bool rm_entry_is_due(const rm_entry_t *e, uint64_t now_us) {
    if (!e || !e->in_use)
        return false;
    return now_us >= e->next_retry_us;
}

bool rm_entry_advance(rm_entry_t *e, uint64_t now_us) {
    if (!e)
        return false;
    e->attempt_count++;
    if (e->attempt_count >= e->max_attempts)
        return false; /* exhausted */

    /* Exponential back-off: delay = base × 2^(attempt_count-1) */
    uint64_t delay = e->base_delay_us;
    for (uint32_t i = 1; i < e->attempt_count; i++) {
        delay *= 2;
        if (delay >= RM_MAX_BACKOFF_US) {
            delay = RM_MAX_BACKOFF_US;
            break;
        }
    }
    e->next_retry_us = now_us + delay;
    return true;
}
