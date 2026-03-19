/*
 * rm_table.c — Retry table implementation
 */

#include "rm_table.h"

#include <stdlib.h>
#include <string.h>

struct rm_table_s {
    rm_entry_t entries[RM_MAX_SLOTS];
    int count;
};

rm_table_t *rm_table_create(void) {
    return calloc(1, sizeof(rm_table_t));
}

void rm_table_destroy(rm_table_t *t) {
    free(t);
}

int rm_table_count(const rm_table_t *t) {
    return t ? t->count : 0;
}

static int find_slot(const rm_table_t *t, uint64_t request_id) {
    for (int i = 0; i < RM_MAX_SLOTS; i++)
        if (t->entries[i].in_use && t->entries[i].request_id == request_id)
            return i;
    return -1;
}

rm_entry_t *rm_table_add(rm_table_t *t, uint64_t request_id, uint64_t now_us,
                         uint64_t base_delay_us, uint32_t max_attempts) {
    if (!t || t->count >= RM_MAX_SLOTS)
        return NULL;
    for (int i = 0; i < RM_MAX_SLOTS; i++) {
        if (!t->entries[i].in_use) {
            if (rm_entry_init(&t->entries[i], request_id, now_us, base_delay_us, max_attempts) != 0)
                return NULL;
            t->count++;
            return &t->entries[i];
        }
    }
    return NULL;
}

int rm_table_remove(rm_table_t *t, uint64_t request_id) {
    if (!t)
        return -1;
    int s = find_slot(t, request_id);
    if (s < 0)
        return -1;
    memset(&t->entries[s], 0, sizeof(t->entries[s]));
    t->count--;
    return 0;
}

rm_entry_t *rm_table_get(rm_table_t *t, uint64_t request_id) {
    if (!t)
        return NULL;
    int s = find_slot(t, request_id);
    return (s >= 0) ? &t->entries[s] : NULL;
}

int rm_table_tick(rm_table_t *t, uint64_t now_us, void (*cb)(rm_entry_t *e, void *user),
                  void *user) {
    if (!t)
        return 0;
    int processed = 0;
    for (int i = 0; i < RM_MAX_SLOTS; i++) {
        if (!t->entries[i].in_use)
            continue;
        if (!rm_entry_is_due(&t->entries[i], now_us))
            continue;

        processed++;
        if (cb)
            cb(&t->entries[i], user);

        bool more = rm_entry_advance(&t->entries[i], now_us);
        if (!more) {
            /* Max attempts reached — auto-evict */
            memset(&t->entries[i], 0, sizeof(t->entries[i]));
            t->count--;
        }
    }
    return processed;
}
