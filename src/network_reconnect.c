/*
 * network_reconnect.c - Peer reconnection with exponential backoff
 * 
 * Handles temporary connection failures gracefully.
 * Auto-reconnects with increasing delays to avoid flooding network.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BACKOFF_MS 100
#define MAX_BACKOFF_MS 30000
#define MAX_RECONNECT_ATTEMPTS 10

typedef struct {
    uint64_t last_attempt;
    uint64_t next_attempt;
    int attempt_count;
    int backoff_ms;
    bool is_reconnecting;
} reconnect_ctx_t;

/*
 * Initialize reconnection tracking for peer
 */
int peer_reconnect_init(peer_t *peer) {
    if (!peer) return -1;

    reconnect_ctx_t *rc = calloc(1, sizeof(reconnect_ctx_t));
    if (!rc) return -1;

    rc->backoff_ms = INITIAL_BACKOFF_MS;
    rc->attempt_count = 0;
    rc->last_attempt = 0;
    rc->next_attempt = 0;
    rc->is_reconnecting = false;

    peer->reconnect_ctx = rc;
    return 0;
}

/*
 * Try to reconnect to peer with backoff
 */
int peer_try_reconnect(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer || !peer->reconnect_ctx) return -1;

    reconnect_ctx_t *rc = (reconnect_ctx_t *)peer->reconnect_ctx;
    uint64_t now = get_timestamp_ms();

    /* Check if enough time has passed */
    if (now < rc->next_attempt) {
        return 0;  /* Not time yet */
    }

    /* Try reconnect */
    printf("INFO: Reconnecting to peer %s (attempt %d/%d)...\n",
           peer->hostname, rc->attempt_count + 1, MAX_RECONNECT_ATTEMPTS);

    int ret = 0;
    
    /* Try UDP first, then TCP */
    if (peer->transport == TRANSPORT_UDP || peer->transport == 0) {
        ret = rootstream_net_handshake(ctx, peer);
        if (ret < 0) {
            printf("INFO: UDP failed, trying TCP fallback...\n");
            ret = rootstream_net_tcp_connect(ctx, peer);
        }
    } else if (peer->transport == TRANSPORT_TCP) {
        ret = rootstream_net_tcp_connect(ctx, peer);
    }

    rc->last_attempt = now;
    rc->attempt_count++;

    if (ret == 0) {
        /* Reconnection successful */
        printf("✓ Peer %s reconnected\n", peer->hostname);
        rc->attempt_count = 0;
        rc->backoff_ms = INITIAL_BACKOFF_MS;
        rc->is_reconnecting = false;
        peer->state = PEER_CONNECTED;
        return 1;  /* Success */
    }

    /* Reconnection failed, schedule next attempt */
    if (rc->attempt_count >= MAX_RECONNECT_ATTEMPTS) {
        printf("ERROR: Max reconnection attempts reached for %s\n", peer->hostname);
        peer->state = PEER_FAILED;
        return -1;  /* Give up */
    }

    /* Exponential backoff */
    rc->backoff_ms = (rc->backoff_ms * 2 > MAX_BACKOFF_MS) ? MAX_BACKOFF_MS : rc->backoff_ms * 2;
    rc->next_attempt = now + rc->backoff_ms;
    rc->is_reconnecting = true;

    printf("WARNING: Will retry peer %s in %dms\n", peer->hostname, rc->backoff_ms);
    return 0;  /* Still trying */
}

/*
 * Cleanup reconnection context
 */
void peer_reconnect_cleanup(peer_t *peer) {
    if (!peer || !peer->reconnect_ctx) return;
    free(peer->reconnect_ctx);
    peer->reconnect_ctx = NULL;
}

/*
 * Reset backoff on successful communication
 */
void peer_reconnect_reset(peer_t *peer) {
    if (!peer || !peer->reconnect_ctx) return;
    reconnect_ctx_t *rc = (reconnect_ctx_t *)peer->reconnect_ctx;
    rc->attempt_count = 0;
    rc->backoff_ms = INITIAL_BACKOFF_MS;
    rc->is_reconnecting = false;
}
