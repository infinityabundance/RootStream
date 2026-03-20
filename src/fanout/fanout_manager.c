/*
 * fanout_manager.c — Multi-client fanout streaming manager implementation
 */

#include "fanout_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h> /* write() */

/* ── Write helpers ─────────────────────────────────────────────── */

/*
 * Send @size bytes of @data on @fd with a 4-byte length prefix.
 * Returns 0 on success, -1 on error.
 */
static int send_frame(int fd, const uint8_t *data, size_t size, fanout_frame_type_t type) {
    if (fd < 0)
        return -1;

    /* 8-byte header: [magic:2][type:1][reserved:1][length:4] */
    uint8_t hdr[8];
    hdr[0] = 0x52; /* 'R' */
    hdr[1] = 0x53; /* 'S' */
    hdr[2] = (uint8_t)type;
    hdr[3] = 0;
    uint32_t len = (uint32_t)size;
    memcpy(hdr + 4, &len, 4);

    /* Best-effort; ignore partial sends in this implementation */
    ssize_t wr = send(fd, hdr, sizeof(hdr), MSG_NOSIGNAL);
    if (wr != (ssize_t)sizeof(hdr))
        return -1;

    wr = send(fd, data, size, MSG_NOSIGNAL);
    if (wr != (ssize_t)size)
        return -1;

    return 0;
}

/* ── Fanout manager ─────────────────────────────────────────────── */

struct fanout_manager_s {
    session_table_t *table; /* borrowed */
    fanout_stats_t stats;
    pthread_mutex_t stats_lock;
};

fanout_manager_t *fanout_manager_create(session_table_t *table) {
    if (!table)
        return NULL;

    fanout_manager_t *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    m->table = table;
    pthread_mutex_init(&m->stats_lock, NULL);
    return m;
}

void fanout_manager_destroy(fanout_manager_t *mgr) {
    if (!mgr)
        return;
    pthread_mutex_destroy(&mgr->stats_lock);
    free(mgr);
}

/* Per-session delivery callback data */
typedef struct {
    const uint8_t *data;
    size_t size;
    fanout_frame_type_t type;
    int delivered;
    int dropped;
} deliver_ctx_t;

static void deliver_to_session(const session_entry_t *entry, void *user_data) {
    deliver_ctx_t *ctx = (deliver_ctx_t *)user_data;

    /* Always deliver keyframes; drop deltas when highly congested */
    if (ctx->type == FANOUT_FRAME_VIDEO_DELTA) {
        /* Simple heuristic: drop if loss > 10% or RTT > 500 ms */
        if (entry->loss_rate > 0.10f || entry->rtt_ms > 500) {
            ctx->dropped++;
            return;
        }
    }

    int rc = send_frame(entry->socket_fd, ctx->data, ctx->size, ctx->type);
    if (rc == 0) {
        ctx->delivered++;
    } else {
        ctx->dropped++;
    }
}

int fanout_manager_deliver(fanout_manager_t *mgr, const uint8_t *frame_data, size_t frame_size,
                           fanout_frame_type_t type) {
    if (!mgr || !frame_data || frame_size == 0)
        return 0;

    deliver_ctx_t ctx = {
        .data = frame_data,
        .size = frame_size,
        .type = type,
        .delivered = 0,
        .dropped = 0,
    };

    session_table_foreach(mgr->table, deliver_to_session, &ctx);

    pthread_mutex_lock(&mgr->stats_lock);
    mgr->stats.frames_in++;
    if (ctx.delivered > 0)
        mgr->stats.frames_delivered++;
    mgr->stats.frames_dropped += (uint64_t)ctx.dropped;
    mgr->stats.active_sessions = session_table_count(mgr->table);
    pthread_mutex_unlock(&mgr->stats_lock);

    return ctx.delivered;
}

void fanout_manager_get_stats(const fanout_manager_t *mgr, fanout_stats_t *stats) {
    if (!mgr || !stats)
        return;
    pthread_mutex_lock((pthread_mutex_t *)&mgr->stats_lock);
    *stats = mgr->stats;
    pthread_mutex_unlock((pthread_mutex_t *)&mgr->stats_lock);
}

void fanout_manager_reset_stats(fanout_manager_t *mgr) {
    if (!mgr)
        return;
    pthread_mutex_lock(&mgr->stats_lock);
    memset(&mgr->stats, 0, sizeof(mgr->stats));
    pthread_mutex_unlock(&mgr->stats_lock);
}
