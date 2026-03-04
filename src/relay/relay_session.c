/*
 * relay_session.c — Relay session manager implementation
 */

#include "relay_session.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

static uint64_t now_us_relay(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
}

struct relay_session_manager_s {
    relay_session_entry_t entries[RELAY_SESSION_MAX];
    bool                  used[RELAY_SESSION_MAX];
    relay_session_id_t    next_id;
    pthread_mutex_t       lock;
};

relay_session_manager_t *relay_session_manager_create(void) {
    relay_session_manager_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    pthread_mutex_init(&m->lock, NULL);
    m->next_id = 1;
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        m->entries[i].host_fd   = -1;
        m->entries[i].viewer_fd = -1;
    }
    return m;
}

void relay_session_manager_destroy(relay_session_manager_t *mgr) {
    if (!mgr) return;
    pthread_mutex_destroy(&mgr->lock);
    free(mgr);
}

int relay_session_open(relay_session_manager_t *mgr,
                       const uint8_t           *token,
                       int                      host_fd,
                       relay_session_id_t       *out_id) {
    if (!mgr || !token || !out_id) return -1;

    pthread_mutex_lock(&mgr->lock);
    int slot = -1;
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (!mgr->used[i]) { slot = i; break; }
    }
    if (slot < 0) {
        pthread_mutex_unlock(&mgr->lock);
        return -1;
    }

    relay_session_entry_t *e = &mgr->entries[slot];
    memset(e, 0, sizeof(*e));
    e->id          = mgr->next_id++;
    e->state       = RELAY_STATE_WAITING;
    e->host_fd     = host_fd;
    e->viewer_fd   = -1;
    e->created_us  = now_us_relay();
    memcpy(e->token, token, RELAY_TOKEN_LEN);
    mgr->used[slot] = true;

    *out_id = e->id;
    pthread_mutex_unlock(&mgr->lock);
    return 0;
}

int relay_session_pair(relay_session_manager_t *mgr,
                       const uint8_t           *token,
                       int                      viewer_fd,
                       relay_session_id_t       *out_id) {
    if (!mgr || !token || !out_id) return -1;

    pthread_mutex_lock(&mgr->lock);
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (!mgr->used[i]) continue;
        relay_session_entry_t *e = &mgr->entries[i];
        if (e->state != RELAY_STATE_WAITING) continue;
        if (memcmp(e->token, token, RELAY_TOKEN_LEN) != 0) continue;

        e->viewer_fd = viewer_fd;
        e->state     = RELAY_STATE_PAIRED;
        *out_id      = e->id;
        pthread_mutex_unlock(&mgr->lock);
        return 0;
    }
    pthread_mutex_unlock(&mgr->lock);
    return -1;
}

int relay_session_close(relay_session_manager_t *mgr,
                        relay_session_id_t       id) {
    if (!mgr) return -1;

    pthread_mutex_lock(&mgr->lock);
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (mgr->used[i] && mgr->entries[i].id == id) {
            mgr->entries[i].state = RELAY_STATE_CLOSING;
            mgr->used[i] = false;
            pthread_mutex_unlock(&mgr->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mgr->lock);
    return -1;
}

int relay_session_get(relay_session_manager_t *mgr,
                      relay_session_id_t       id,
                      relay_session_entry_t   *out) {
    if (!mgr || !out) return -1;

    pthread_mutex_lock(&mgr->lock);
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (mgr->used[i] && mgr->entries[i].id == id) {
            *out = mgr->entries[i];
            pthread_mutex_unlock(&mgr->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mgr->lock);
    return -1;
}

size_t relay_session_count(relay_session_manager_t *mgr) {
    if (!mgr) return 0;
    size_t count = 0;
    pthread_mutex_lock(&mgr->lock);
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (mgr->used[i]) count++;
    }
    pthread_mutex_unlock(&mgr->lock);
    return count;
}

int relay_session_add_bytes(relay_session_manager_t *mgr,
                            relay_session_id_t       id,
                            uint64_t                 bytes) {
    if (!mgr) return -1;
    pthread_mutex_lock(&mgr->lock);
    for (int i = 0; i < RELAY_SESSION_MAX; i++) {
        if (mgr->used[i] && mgr->entries[i].id == id) {
            mgr->entries[i].bytes_relayed += bytes;
            pthread_mutex_unlock(&mgr->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mgr->lock);
    return -1;
}
