/*
 * kfr_handler.c — Keyframe request dedup + rate limiter implementation
 */

#include "kfr_handler.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t ssrc;
    uint64_t last_forward_us;
    bool valid;
    bool has_forwarded;
} kfr_ssrc_entry_t;

struct kfr_handler_s {
    kfr_ssrc_entry_t entries[KFR_MAX_SSRC];
    uint64_t cooldown_us;
};

kfr_handler_t *kfr_handler_create(uint64_t cooldown_us) {
    kfr_handler_t *h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;
    h->cooldown_us = cooldown_us;
    return h;
}

void kfr_handler_destroy(kfr_handler_t *h) {
    free(h);
}

int kfr_handler_set_cooldown(kfr_handler_t *h, uint64_t cooldown_us) {
    if (!h)
        return -1;
    h->cooldown_us = cooldown_us;
    return 0;
}

static kfr_ssrc_entry_t *find_or_alloc(kfr_handler_t *h, uint32_t ssrc) {
    kfr_ssrc_entry_t *free_slot = NULL;
    for (int i = 0; i < KFR_MAX_SSRC; i++) {
        if (h->entries[i].valid && h->entries[i].ssrc == ssrc)
            return &h->entries[i];
        if (!h->entries[i].valid && !free_slot)
            free_slot = &h->entries[i];
    }
    if (free_slot) {
        free_slot->ssrc = ssrc;
        free_slot->last_forward_us = 0;
        free_slot->valid = true;
    }
    return free_slot;
}

kfr_decision_t kfr_handler_submit(kfr_handler_t *h, const kfr_message_t *msg, uint64_t now_us) {
    if (!h || !msg)
        return KFR_DECISION_SUPPRESS;

    kfr_ssrc_entry_t *e = find_or_alloc(h, msg->ssrc);
    if (!e)
        return KFR_DECISION_SUPPRESS; /* registry full */

    /* Urgent requests always bypass the cooldown */
    if (msg->priority > 0 || !e->has_forwarded || (now_us - e->last_forward_us) >= h->cooldown_us) {
        e->last_forward_us = now_us;
        e->has_forwarded = true;
        return KFR_DECISION_FORWARD;
    }
    return KFR_DECISION_SUPPRESS;
}

void kfr_handler_flush_ssrc(kfr_handler_t *h, uint32_t ssrc) {
    if (!h)
        return;
    for (int i = 0; i < KFR_MAX_SSRC; i++) {
        if (h->entries[i].valid && h->entries[i].ssrc == ssrc) {
            h->entries[i].last_forward_us = 0;
            h->entries[i].has_forwarded = false;
            return;
        }
    }
}

const char *kfr_decision_name(kfr_decision_t d) {
    switch (d) {
        case KFR_DECISION_FORWARD:
            return "FORWARD";
        case KFR_DECISION_SUPPRESS:
            return "SUPPRESS";
        default:
            return "UNKNOWN";
    }
}
