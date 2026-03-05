/*
 * session_table.c — Per-client session state table implementation
 */

#include "session_table.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
}

struct session_table_s {
    session_entry_t  entries[SESSION_TABLE_MAX];
    bool             used[SESSION_TABLE_MAX];
    session_id_t     next_id;
    pthread_mutex_t  lock;
};

session_table_t *session_table_create(void) {
    session_table_t *t = calloc(1, sizeof(*t));
    if (!t) return NULL;

    pthread_mutex_init(&t->lock, NULL);
    t->next_id = 1;

    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        t->entries[i].socket_fd = -1;
    }
    return t;
}

void session_table_destroy(session_table_t *table) {
    if (!table) return;
    pthread_mutex_destroy(&table->lock);
    free(table);
}

int session_table_add(session_table_t *table,
                      int              socket_fd,
                      const char      *peer_addr,
                      session_id_t    *out_id) {
    if (!table || !peer_addr || !out_id) return -1;

    pthread_mutex_lock(&table->lock);

    int slot = -1;
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (!table->used[i]) { slot = i; break; }
    }

    if (slot < 0) {
        pthread_mutex_unlock(&table->lock);
        return -1;
    }

    session_entry_t *e = &table->entries[slot];
    memset(e, 0, sizeof(*e));
    e->id             = table->next_id++;
    e->state          = SESSION_STATE_ACTIVE;
    e->socket_fd      = socket_fd;
    e->bitrate_kbps   = 4000;    /* Default 4 Mbps */
    e->max_bitrate_kbps = 20000;
    e->connected_at_us = now_us();
    snprintf(e->peer_addr, sizeof(e->peer_addr), "%s", peer_addr);
    table->used[slot] = true;

    *out_id = e->id;
    pthread_mutex_unlock(&table->lock);
    return 0;
}

int session_table_remove(session_table_t *table, session_id_t id) {
    if (!table) return -1;

    pthread_mutex_lock(&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] && table->entries[i].id == id) {
            table->entries[i].state = SESSION_STATE_CLOSED;
            table->used[i] = false;
            pthread_mutex_unlock(&table->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&table->lock);
    return -1;
}

int session_table_get(const session_table_t *table,
                      session_id_t           id,
                      session_entry_t       *out) {
    if (!table || !out) return -1;

    pthread_mutex_lock((pthread_mutex_t *)&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] && table->entries[i].id == id) {
            *out = table->entries[i];
            pthread_mutex_unlock((pthread_mutex_t *)&table->lock);
            return 0;
        }
    }
    pthread_mutex_unlock((pthread_mutex_t *)&table->lock);
    return -1;
}

int session_table_update_bitrate(session_table_t *table,
                                 session_id_t     id,
                                 uint32_t         bitrate_kbps) {
    if (!table) return -1;

    pthread_mutex_lock(&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] && table->entries[i].id == id) {
            table->entries[i].bitrate_kbps = bitrate_kbps;
            pthread_mutex_unlock(&table->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&table->lock);
    return -1;
}

int session_table_update_stats(session_table_t *table,
                               session_id_t     id,
                               uint32_t         rtt_ms,
                               float            loss_rate) {
    if (!table) return -1;

    pthread_mutex_lock(&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] && table->entries[i].id == id) {
            table->entries[i].rtt_ms    = rtt_ms;
            table->entries[i].loss_rate = loss_rate;
            pthread_mutex_unlock(&table->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&table->lock);
    return -1;
}

size_t session_table_count(const session_table_t *table) {
    if (!table) return 0;

    size_t count = 0;
    pthread_mutex_lock((pthread_mutex_t *)&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] &&
            table->entries[i].state == SESSION_STATE_ACTIVE) {
            count++;
        }
    }
    pthread_mutex_unlock((pthread_mutex_t *)&table->lock);
    return count;
}

void session_table_foreach(const session_table_t *table,
                           void (*callback)(const session_entry_t *,
                                            void *),
                           void *user_data) {
    if (!table || !callback) return;

    pthread_mutex_lock((pthread_mutex_t *)&table->lock);
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        if (table->used[i] &&
            table->entries[i].state == SESSION_STATE_ACTIVE) {
            callback(&table->entries[i], user_data);
        }
    }
    pthread_mutex_unlock((pthread_mutex_t *)&table->lock);
}
