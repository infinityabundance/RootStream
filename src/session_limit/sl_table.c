/*
 * sl_table.c — Session table implementation
 */

#include "sl_table.h"

#include <stdlib.h>
#include <string.h>

struct sl_table_s {
    sl_entry_t entries[SL_MAX_SLOTS];
    int max_sessions;
    int count;
};

sl_table_t *sl_table_create(int max_sessions) {
    if (max_sessions < 1 || max_sessions > SL_MAX_SLOTS)
        return NULL;
    sl_table_t *t = calloc(1, sizeof(*t));
    if (!t)
        return NULL;
    t->max_sessions = max_sessions;
    return t;
}

void sl_table_destroy(sl_table_t *t) {
    free(t);
}

int sl_table_count(const sl_table_t *t) {
    return t ? t->count : 0;
}

static int find_slot(const sl_table_t *t, uint64_t session_id) {
    for (int i = 0; i < SL_MAX_SLOTS; i++)
        if (t->entries[i].in_use && t->entries[i].session_id == session_id)
            return i;
    return -1;
}

sl_entry_t *sl_table_add(sl_table_t *t, uint64_t session_id, const char *remote_ip,
                         uint64_t start_us) {
    if (!t)
        return NULL;
    if (t->count >= t->max_sessions)
        return NULL; /* cap reached */
    for (int i = 0; i < SL_MAX_SLOTS; i++) {
        if (!t->entries[i].in_use) {
            sl_entry_init(&t->entries[i], session_id, remote_ip, start_us);
            t->count++;
            return &t->entries[i];
        }
    }
    return NULL; /* table full (shouldn't happen if count < max_sessions) */
}

int sl_table_remove(sl_table_t *t, uint64_t session_id) {
    if (!t)
        return -1;
    int slot = find_slot(t, session_id);
    if (slot < 0)
        return -1;
    memset(&t->entries[slot], 0, sizeof(t->entries[slot]));
    t->count--;
    return 0;
}

sl_entry_t *sl_table_get(sl_table_t *t, uint64_t session_id) {
    if (!t)
        return NULL;
    int slot = find_slot(t, session_id);
    return (slot >= 0) ? &t->entries[slot] : NULL;
}

void sl_table_foreach(sl_table_t *t, void (*cb)(sl_entry_t *e, void *user), void *user) {
    if (!t || !cb)
        return;
    for (int i = 0; i < SL_MAX_SLOTS; i++)
        if (t->entries[i].in_use)
            cb(&t->entries[i], user);
}
