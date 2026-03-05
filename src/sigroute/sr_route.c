/*
 * sr_route.c — Signal router implementation
 */

#include "sr_route.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t          src_mask;
    uint32_t          match_id;
    sr_filter_fn      filter_fn;
    sr_deliver_fn     deliver;
    void             *user;
    bool              in_use;
    sr_route_handle_t handle;
} route_entry_t;

struct sr_route_s {
    route_entry_t     routes[SR_MAX_ROUTES];
    int               count;
    sr_route_handle_t next_handle;
};

sr_router_t *sr_router_create(void) {
    return calloc(1, sizeof(sr_router_t));
}
void sr_router_destroy(sr_router_t *r) { free(r); }
int  sr_router_count(const sr_router_t *r) { return r ? r->count : 0; }

sr_route_handle_t sr_router_add_route(sr_router_t   *r,
                                       uint32_t       src_mask,
                                       uint32_t       match_id,
                                       sr_filter_fn   filter_fn,
                                       sr_deliver_fn  deliver,
                                       void          *user) {
    if (!r || !deliver || r->count >= SR_MAX_ROUTES) return SR_INVALID_HANDLE;
    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (!r->routes[i].in_use) {
            r->routes[i].src_mask  = src_mask;
            r->routes[i].match_id  = match_id;
            r->routes[i].filter_fn = filter_fn;
            r->routes[i].deliver   = deliver;
            r->routes[i].user      = user;
            r->routes[i].in_use    = true;
            r->routes[i].handle    = r->next_handle++;
            r->count++;
            return r->routes[i].handle;
        }
    }
    return SR_INVALID_HANDLE;
}

int sr_router_remove_route(sr_router_t *r, sr_route_handle_t h) {
    if (!r || h < 0) return -1;
    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (r->routes[i].in_use && r->routes[i].handle == h) {
            memset(&r->routes[i], 0, sizeof(r->routes[i]));
            r->count--;
            return 0;
        }
    }
    return -1;
}

int sr_router_route(sr_router_t *r, const sr_signal_t *s) {
    if (!r || !s) return 0;
    int delivered = 0;
    for (int i = 0; i < SR_MAX_ROUTES; i++) {
        if (!r->routes[i].in_use) continue;
        if ((s->signal_id & r->routes[i].src_mask) != r->routes[i].match_id)
            continue;
        if (r->routes[i].filter_fn &&
            !r->routes[i].filter_fn(s, r->routes[i].user))
            continue;
        r->routes[i].deliver(s, r->routes[i].user);
        delivered++;
    }
    return delivered;
}
