/*
 * output_registry.c — Output target registry
 */

#include "output_registry.h"

#include <stdlib.h>
#include <string.h>

struct output_registry_s {
    output_target_t targets[OUTPUT_MAX_TARGETS];
    bool            used[OUTPUT_MAX_TARGETS];
    int             count;
};

output_registry_t *output_registry_create(void) {
    return calloc(1, sizeof(output_registry_t));
}

void output_registry_destroy(output_registry_t *r) { free(r); }

int output_registry_count(const output_registry_t *r) {
    return r ? r->count : 0;
}

int output_registry_active_count(const output_registry_t *r) {
    if (!r) return 0;
    int n = 0;
    for (int i = 0; i < OUTPUT_MAX_TARGETS; i++)
        if (r->used[i] && r->targets[i].state == OT_ACTIVE) n++;
    return n;
}

static int find_slot(const output_registry_t *r, const char *name) {
    for (int i = 0; i < OUTPUT_MAX_TARGETS; i++)
        if (r->used[i] &&
            strncmp(r->targets[i].name, name, OUTPUT_NAME_MAX) == 0)
            return i;
    return -1;
}

output_target_t *output_registry_add(output_registry_t *r,
                                       const char *name,
                                       const char *url,
                                       const char *protocol) {
    if (!r || !name) return NULL;
    if (r->count >= OUTPUT_MAX_TARGETS) return NULL;
    if (find_slot(r, name) >= 0) return NULL; /* duplicate */
    for (int i = 0; i < OUTPUT_MAX_TARGETS; i++) {
        if (!r->used[i]) {
            ot_init(&r->targets[i], name, url, protocol);
            r->used[i] = true;
            r->count++;
            return &r->targets[i];
        }
    }
    return NULL;
}

int output_registry_remove(output_registry_t *r, const char *name) {
    if (!r || !name) return -1;
    int slot = find_slot(r, name);
    if (slot < 0) return -1;
    r->used[slot] = false;
    r->count--;
    return 0;
}

output_target_t *output_registry_get(output_registry_t *r, const char *name) {
    if (!r || !name) return NULL;
    int slot = find_slot(r, name);
    return (slot >= 0) ? &r->targets[slot] : NULL;
}

int output_registry_set_state(output_registry_t *r,
                                const char *name,
                                ot_state_t state) {
    output_target_t *t = output_registry_get(r, name);
    if (!t) return -1;
    t->state = state;
    return 0;
}

int output_registry_enable(output_registry_t *r, const char *name) {
    output_target_t *t = output_registry_get(r, name);
    if (!t) return -1;
    t->enabled = true;
    if (t->state == OT_DISABLED) t->state = OT_IDLE;
    return 0;
}

int output_registry_disable(output_registry_t *r, const char *name) {
    output_target_t *t = output_registry_get(r, name);
    if (!t) return -1;
    t->enabled = false;
    t->state   = OT_DISABLED;
    return 0;
}

void output_registry_foreach(output_registry_t *r,
                               void (*cb)(output_target_t *t, void *user),
                               void *user) {
    if (!r || !cb) return;
    for (int i = 0; i < OUTPUT_MAX_TARGETS; i++)
        if (r->used[i]) cb(&r->targets[i], user);
}
