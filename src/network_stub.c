/*
 * network_stub.c - Stubbed networking for NO_CRYPTO builds
 *
 * Provides timestamps and safe errors when networking is unavailable.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int rootstream_net_init(rootstream_ctx_t *ctx, uint16_t port) {
    (void)ctx;
    (void)port;
    fprintf(stderr, "ERROR: Networking unavailable (NO_CRYPTO build)\n");
    fprintf(stderr, "FIX: Install libsodium and rebuild without NO_CRYPTO=1\n");
    return -1;
}

int rootstream_net_send_encrypted(rootstream_ctx_t *ctx, peer_t *peer,
                                  uint8_t type, const void *data, size_t size) {
    (void)ctx;
    (void)peer;
    (void)type;
    (void)data;
    (void)size;
    fprintf(stderr, "ERROR: Cannot send packet (NO_CRYPTO build)\n");
    return -1;
}

int rootstream_net_recv(rootstream_ctx_t *ctx, int timeout_ms) {
    (void)ctx;
    (void)timeout_ms;
    fprintf(stderr, "ERROR: Cannot receive packets (NO_CRYPTO build)\n");
    return -1;
}

int rootstream_net_handshake(rootstream_ctx_t *ctx, peer_t *peer) {
    (void)ctx;
    (void)peer;
    fprintf(stderr, "ERROR: Cannot handshake (NO_CRYPTO build)\n");
    return -1;
}

peer_t* rootstream_add_peer(rootstream_ctx_t *ctx, const char *rootstream_code) {
    (void)ctx;
    (void)rootstream_code;
    fprintf(stderr, "ERROR: Cannot add peer (NO_CRYPTO build)\n");
    return NULL;
}

peer_t* rootstream_find_peer(rootstream_ctx_t *ctx, const uint8_t *public_key) {
    (void)ctx;
    (void)public_key;
    return NULL;
}

void rootstream_remove_peer(rootstream_ctx_t *ctx, peer_t *peer) {
    (void)ctx;
    (void)peer;
}

int rootstream_connect_to_peer(rootstream_ctx_t *ctx, const char *rootstream_code) {
    (void)ctx;
    (void)rootstream_code;
    fprintf(stderr, "ERROR: Cannot connect to peer (NO_CRYPTO build)\n");
    return -1;
}

uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
