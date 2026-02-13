/*
 * test_discovery_cache.c - Unit tests for PHASE 17 discovery cache
 * 
 * Tests peer cache management, TTL expiry, and statistics tracking.
 */

#include "../../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* Stub functions needed for linking */
uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* Stub discovery functions */
int discovery_broadcast_announce(rootstream_ctx_t *ctx) { (void)ctx; return 0; }
int discovery_broadcast_listen(rootstream_ctx_t *ctx, int timeout_ms) { (void)ctx; (void)timeout_ms; return 0; }
int discovery_save_peer_to_history(rootstream_ctx_t *ctx, const char *hostname,
                                   uint16_t port, const char *rootstream_code) {
    (void)ctx; (void)hostname; (void)port; (void)rootstream_code; return 0;
}

/* Test helper: Create a mock peer cache entry */
static peer_cache_entry_t create_test_peer(const char *hostname, const char *ip,
                                           uint16_t port, const char *capability) {
    peer_cache_entry_t entry = {0};
    strncpy(entry.hostname, hostname, sizeof(entry.hostname) - 1);
    strncpy(entry.ip_address, ip, sizeof(entry.ip_address) - 1);
    entry.port = port;
    snprintf(entry.rootstream_code, sizeof(entry.rootstream_code),
            "TESTCODE%s", hostname);
    strncpy(entry.capability, capability, sizeof(entry.capability) - 1);
    strncpy(entry.version, "1.0.0", sizeof(entry.version) - 1);
    entry.max_peers = 10;
    strncpy(entry.bandwidth, "100Mbps", sizeof(entry.bandwidth) - 1);
    entry.discovered_time_us = get_timestamp_us();
    entry.last_seen_time_us = entry.discovered_time_us;
    entry.ttl_seconds = 3600;
    entry.is_online = true;
    return entry;
}

/* Test 1: Add peer to cache */
void test_cache_add_peer() {
    printf("TEST: Add peer to cache...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t peer = create_test_peer("test-host-1", "192.168.1.100",
                                               9876, "host");
    
    int ret = discovery_cache_add_peer(&ctx, &peer);
    assert(ret == 0);
    assert(ctx.discovery.num_cached_peers == 1);
    assert(strcmp(ctx.discovery.peer_cache[0].hostname, "test-host-1") == 0);
    assert(strcmp(ctx.discovery.peer_cache[0].ip_address, "192.168.1.100") == 0);
    assert(ctx.discovery.peer_cache[0].port == 9876);
    assert(strcmp(ctx.discovery.peer_cache[0].capability, "host") == 0);
    
    printf("  ✓ Peer added successfully\n");
}

/* Test 2: Update existing peer */
void test_cache_update_peer() {
    printf("TEST: Update existing peer...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t peer = create_test_peer("test-host-1", "192.168.1.100",
                                               9876, "host");
    
    discovery_cache_add_peer(&ctx, &peer);
    uint32_t original_contact_count = ctx.discovery.peer_cache[0].contact_count;
    
    /* Wait a bit */
    usleep(10000); /* 10ms */
    
    /* Add same peer again (should update) */
    peer.last_seen_time_us = get_timestamp_us();
    int ret = discovery_cache_add_peer(&ctx, &peer);
    assert(ret == 0);
    assert(ctx.discovery.num_cached_peers == 1); /* Still only one peer */
    assert(ctx.discovery.peer_cache[0].contact_count > original_contact_count);
    
    printf("  ✓ Peer updated successfully\n");
}

/* Test 3: Get peer from cache */
void test_cache_get_peer() {
    printf("TEST: Get peer from cache...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t peer1 = create_test_peer("test-host-1", "192.168.1.100",
                                                9876, "host");
    peer_cache_entry_t peer2 = create_test_peer("test-host-2", "192.168.1.101",
                                                9877, "client");
    
    discovery_cache_add_peer(&ctx, &peer1);
    discovery_cache_add_peer(&ctx, &peer2);
    
    peer_cache_entry_t *found = discovery_cache_get_peer(&ctx, "test-host-2");
    assert(found != NULL);
    assert(strcmp(found->hostname, "test-host-2") == 0);
    assert(strcmp(found->ip_address, "192.168.1.101") == 0);
    assert(found->port == 9877);
    assert(strcmp(found->capability, "client") == 0);
    
    /* Try to get non-existent peer */
    peer_cache_entry_t *not_found = discovery_cache_get_peer(&ctx, "nonexistent");
    assert(not_found == NULL);
    
    printf("  ✓ Peer retrieval works correctly\n");
}

/* Test 4: Remove peer from cache */
void test_cache_remove_peer() {
    printf("TEST: Remove peer from cache...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t peer1 = create_test_peer("test-host-1", "192.168.1.100",
                                                9876, "host");
    peer_cache_entry_t peer2 = create_test_peer("test-host-2", "192.168.1.101",
                                                9877, "client");
    
    discovery_cache_add_peer(&ctx, &peer1);
    discovery_cache_add_peer(&ctx, &peer2);
    assert(ctx.discovery.num_cached_peers == 2);
    
    int ret = discovery_cache_remove_peer(&ctx, "test-host-1");
    assert(ret == 0);
    assert(ctx.discovery.num_cached_peers == 1);
    assert(strcmp(ctx.discovery.peer_cache[0].hostname, "test-host-2") == 0);
    
    /* Try to remove non-existent peer */
    ret = discovery_cache_remove_peer(&ctx, "nonexistent");
    assert(ret == -1);
    
    printf("  ✓ Peer removal works correctly\n");
}

/* Test 5: Get all cached peers */
void test_cache_get_all() {
    printf("TEST: Get all cached peers...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t entries[10];
    
    /* Add multiple peers */
    for (int i = 0; i < 5; i++) {
        char hostname[64];
        char ip[32];
        snprintf(hostname, sizeof(hostname), "test-host-%d", i);
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        peer_cache_entry_t peer = create_test_peer(hostname, ip, 9876 + i, "host");
        discovery_cache_add_peer(&ctx, &peer);
    }
    
    int count = discovery_cache_get_all(&ctx, entries, 10);
    assert(count == 5);
    assert(strcmp(entries[0].hostname, "test-host-0") == 0);
    assert(strcmp(entries[4].hostname, "test-host-4") == 0);
    
    printf("  ✓ Get all peers works correctly\n");
}

/* Test 6: Get only online peers */
void test_cache_get_online() {
    printf("TEST: Get only online peers...\n");
    
    rootstream_ctx_t ctx = {0};
    peer_cache_entry_t entries[10];
    
    /* Add peers with different online status */
    for (int i = 0; i < 5; i++) {
        char hostname[64];
        char ip[32];
        snprintf(hostname, sizeof(hostname), "test-host-%d", i);
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        peer_cache_entry_t peer = create_test_peer(hostname, ip, 9876 + i, "host");
        peer.is_online = (i % 2 == 0); /* Only even-indexed peers are online */
        discovery_cache_add_peer(&ctx, &peer);
    }
    
    int count = discovery_cache_get_online(&ctx, entries, 10);
    assert(count == 3); /* Peers 0, 2, 4 are online */
    
    /* Verify all returned peers are online */
    for (int i = 0; i < count; i++) {
        assert(entries[i].is_online == true);
    }
    
    printf("  ✓ Get online peers works correctly\n");
}

/* Test 7: Cache expiry */
void test_cache_expiry() {
    printf("TEST: Cache expiry...\n");
    
    rootstream_ctx_t ctx = {0};
    
    /* Add peers with different ages */
    for (int i = 0; i < 3; i++) {
        char hostname[64];
        char ip[32];
        snprintf(hostname, sizeof(hostname), "test-host-%d", i);
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        peer_cache_entry_t peer = create_test_peer(hostname, ip, 9876 + i, "host");
        peer.ttl_seconds = 1;  /* Very short TTL for testing */
        
        if (i == 0) {
            /* Make first peer very old */
            peer.last_seen_time_us = get_timestamp_us() - 2000000ULL; /* 2 seconds ago */
        } else {
            peer.last_seen_time_us = get_timestamp_us();
        }
        
        discovery_cache_add_peer(&ctx, &peer);
    }
    
    assert(ctx.discovery.num_cached_peers == 3);
    
    /* Expire old entries */
    discovery_cache_expire_old_entries(&ctx);
    
    /* First peer should be removed */
    assert(ctx.discovery.num_cached_peers == 2);
    assert(strcmp(ctx.discovery.peer_cache[0].hostname, "test-host-1") == 0);
    
    printf("  ✓ Cache expiry works correctly\n");
}

/* Test 8: Statistics tracking */
void test_discovery_stats() {
    printf("TEST: Discovery statistics tracking...\n");
    
    rootstream_ctx_t ctx = {0};
    
    assert(ctx.discovery.total_discoveries == 0);
    assert(ctx.discovery.total_losses == 0);
    
    /* Add peers */
    for (int i = 0; i < 3; i++) {
        char hostname[64];
        char ip[32];
        snprintf(hostname, sizeof(hostname), "test-host-%d", i);
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        peer_cache_entry_t peer = create_test_peer(hostname, ip, 9876 + i, "host");
        discovery_cache_add_peer(&ctx, &peer);
    }
    
    assert(ctx.discovery.total_discoveries == 3);
    
    /* Remove one peer */
    discovery_cache_remove_peer(&ctx, "test-host-1");
    assert(ctx.discovery.total_losses == 1);
    
    printf("  ✓ Statistics tracking works correctly\n");
}

/* Test 9: Cache cleanup */
void test_cache_cleanup() {
    printf("TEST: Cache cleanup...\n");
    
    rootstream_ctx_t ctx = {0};
    
    /* Add multiple peers */
    for (int i = 0; i < 5; i++) {
        char hostname[64];
        char ip[32];
        snprintf(hostname, sizeof(hostname), "test-host-%d", i);
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        peer_cache_entry_t peer = create_test_peer(hostname, ip, 9876 + i, "host");
        discovery_cache_add_peer(&ctx, &peer);
    }
    
    assert(ctx.discovery.num_cached_peers == 5);
    
    discovery_cache_cleanup(&ctx);
    
    assert(ctx.discovery.num_cached_peers == 0);
    
    printf("  ✓ Cache cleanup works correctly\n");
}

int main() {
    printf("\n=== Discovery Cache Unit Tests ===\n\n");
    
    test_cache_add_peer();
    test_cache_update_peer();
    test_cache_get_peer();
    test_cache_remove_peer();
    test_cache_get_all();
    test_cache_get_online();
    test_cache_expiry();
    test_discovery_stats();
    test_cache_cleanup();
    
    printf("\n=== All Discovery Cache Tests Passed ✓ ===\n");
    return 0;
}
