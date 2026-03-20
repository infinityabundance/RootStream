/*
 * eb_bus.c — Event bus pub/sub implementation
 */

#include "eb_bus.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    eb_type_t type_id;
    eb_callback_t cb;
    void *user;
    bool in_use;
    eb_handle_t handle;
} subscription_t;

struct eb_bus_s {
    subscription_t subs[EB_MAX_SUBSCRIBERS];
    int count;
    eb_handle_t next_handle;
};

eb_bus_t *eb_bus_create(void) {
    eb_bus_t *b = calloc(1, sizeof(*b));
    if (b)
        b->next_handle = 0;
    return b;
}

void eb_bus_destroy(eb_bus_t *b) {
    free(b);
}

int eb_bus_subscriber_count(const eb_bus_t *b) {
    return b ? b->count : 0;
}

eb_handle_t eb_bus_subscribe(eb_bus_t *b, eb_type_t type_id, eb_callback_t cb, void *user) {
    if (!b || !cb || b->count >= EB_MAX_SUBSCRIBERS)
        return EB_INVALID_HANDLE;
    for (int i = 0; i < EB_MAX_SUBSCRIBERS; i++) {
        if (!b->subs[i].in_use) {
            b->subs[i].type_id = type_id;
            b->subs[i].cb = cb;
            b->subs[i].user = user;
            b->subs[i].in_use = true;
            b->subs[i].handle = b->next_handle++;
            b->count++;
            return b->subs[i].handle;
        }
    }
    return EB_INVALID_HANDLE;
}

int eb_bus_unsubscribe(eb_bus_t *b, eb_handle_t h) {
    if (!b || h < 0)
        return -1;
    for (int i = 0; i < EB_MAX_SUBSCRIBERS; i++) {
        if (b->subs[i].in_use && b->subs[i].handle == h) {
            memset(&b->subs[i], 0, sizeof(b->subs[i]));
            b->count--;
            return 0;
        }
    }
    return -1;
}

int eb_bus_publish(eb_bus_t *b, const eb_event_t *e) {
    if (!b || !e)
        return 0;
    int dispatched = 0;
    for (int i = 0; i < EB_MAX_SUBSCRIBERS; i++) {
        if (!b->subs[i].in_use)
            continue;
        if (b->subs[i].type_id == EB_TYPE_ANY || b->subs[i].type_id == e->type_id) {
            b->subs[i].cb(e, b->subs[i].user);
            dispatched++;
        }
    }
    return dispatched;
}
