/*
 * pointer_sync.c — Remote cursor / pointer synchronisation implementation
 */

#include "pointer_sync.h"

#include <stdlib.h>
#include <string.h>

struct pointer_sync_s {
    remote_pointer_t peers[POINTER_SYNC_MAX_PEERS];
    int              count;
    uint64_t         timeout_us;
};

pointer_sync_t *pointer_sync_create(uint64_t timeout_us) {
    pointer_sync_t *ps = calloc(1, sizeof(*ps));
    if (!ps) return NULL;
    ps->timeout_us = (timeout_us > 0) ? timeout_us : POINTER_SYNC_TIMEOUT_US;
    return ps;
}

void pointer_sync_destroy(pointer_sync_t *ps) {
    free(ps);
}

static remote_pointer_t *find_peer(pointer_sync_t *ps, uint32_t peer_id) {
    for (int i = 0; i < ps->count; i++) {
        if (ps->peers[i].peer_id == peer_id) return &ps->peers[i];
    }
    return NULL;
}

static remote_pointer_t *alloc_peer(pointer_sync_t *ps, uint32_t peer_id) {
    if (ps->count < POINTER_SYNC_MAX_PEERS) {
        remote_pointer_t *p = &ps->peers[ps->count++];
        memset(p, 0, sizeof(*p));
        p->peer_id = peer_id;
        return p;
    }
    /* Evict the oldest (first) slot when full */
    memmove(&ps->peers[0], &ps->peers[1],
            (POINTER_SYNC_MAX_PEERS - 1) * sizeof(remote_pointer_t));
    remote_pointer_t *p = &ps->peers[POINTER_SYNC_MAX_PEERS - 1];
    memset(p, 0, sizeof(*p));
    p->peer_id = peer_id;
    return p;
}

void pointer_sync_update(pointer_sync_t           *ps,
                         const annotation_event_t *event) {
    if (!ps || !event) return;

    switch (event->type) {
    case ANNOT_POINTER_MOVE:
        {
            uint32_t pid = event->pointer_move.peer_id;
            remote_pointer_t *p = find_peer(ps, pid);
            if (!p) p = alloc_peer(ps, pid);
            p->pos              = event->pointer_move.pos;
            p->last_updated_us  = event->timestamp_us;
            p->visible          = true;
        }
        break;

    case ANNOT_POINTER_HIDE:
        {
            /* peer_id not in pointer_hide payload; hide all if peer_id==0 */
            for (int i = 0; i < ps->count; i++) {
                ps->peers[i].visible = false;
            }
        }
        break;

    default:
        break;
    }
}

int pointer_sync_get(const pointer_sync_t *ps,
                     uint32_t              peer_id,
                     remote_pointer_t     *out) {
    if (!ps || !out) return -1;
    for (int i = 0; i < ps->count; i++) {
        if (ps->peers[i].peer_id == peer_id) {
            *out = ps->peers[i];
            return 0;
        }
    }
    return -1;
}

int pointer_sync_get_all(const pointer_sync_t *ps,
                          remote_pointer_t     *out,
                          int                   max_count) {
    if (!ps || !out || max_count <= 0) return 0;

    int n = 0;
    for (int i = 0; i < ps->count && n < max_count; i++) {
        if (ps->peers[i].visible) {
            out[n++] = ps->peers[i];
        }
    }
    return n;
}

void pointer_sync_expire(pointer_sync_t *ps, uint64_t now_us) {
    if (!ps) return;

    for (int i = 0; i < ps->count; i++) {
        if (ps->peers[i].visible &&
            now_us - ps->peers[i].last_updated_us > ps->timeout_us) {
            ps->peers[i].visible = false;
        }
    }
}
