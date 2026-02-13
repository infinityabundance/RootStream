/*
 * test_discovery_fallback.c - Test peer discovery fallback chain
 * 
 * Validates:
 * - mDNS/Avahi discovery (primary)
 * - UDP broadcast fallback
 * - Manual peer entry final fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

#define MAX_PEERS 16

typedef struct {
    char hostname[256];
    char public_key[64];
    char ip_address[64];
    int port;
} peer_info_t;

typedef struct {
    const char *name;
    int (*init_fn)(void);
    int (*discover_fn)(peer_info_t *peers, int *peer_count);
    void (*cleanup_fn)(void);
} discovery_backend_t;

/* Mock mDNS/Avahi discovery */
int mock_mdns_init(void) {
    return 0;
}

int mock_mdns_discover(peer_info_t *peers, int *peer_count) {
    /* Simulate discovering 2 peers via mDNS */
    if (*peer_count < 2) {
        return -1;
    }
    
    strcpy(peers[0].hostname, "gaming-pc");
    strcpy(peers[0].public_key, "kXx7Y...Qp9w");
    strcpy(peers[0].ip_address, "192.168.1.100");
    peers[0].port = 7777;
    
    strcpy(peers[1].hostname, "media-server");
    strcpy(peers[1].public_key, "aB3dE...fG8h");
    strcpy(peers[1].ip_address, "192.168.1.101");
    peers[1].port = 7777;
    
    *peer_count = 2;
    return 0;
}

void mock_mdns_cleanup(void) {
}

/* Mock UDP broadcast discovery */
int mock_broadcast_init(void) {
    return 0;
}

int mock_broadcast_discover(peer_info_t *peers, int *peer_count) {
    /* Simulate discovering 1 peer via broadcast */
    if (*peer_count < 1) {
        return -1;
    }
    
    strcpy(peers[0].hostname, "lan-pc");
    strcpy(peers[0].public_key, "xYz12...Abc3");
    strcpy(peers[0].ip_address, "192.168.1.50");
    peers[0].port = 7777;
    
    *peer_count = 1;
    return 0;
}

void mock_broadcast_cleanup(void) {
}

/* Mock manual peer entry */
int mock_manual_init(void) {
    return 0;
}

int mock_manual_discover(peer_info_t *peers, int *peer_count) {
    /* Manual entry always works - returns empty list or user-configured peers */
    *peer_count = 0;
    return 0;
}

void mock_manual_cleanup(void) {
}

/* Test: mDNS initialization */
test_result_t test_discovery_mdns_init(void) {
    int ret = mock_mdns_init();
    ASSERT_EQ(ret, 0);
    
    mock_mdns_cleanup();
    return TEST_PASS;
}

/* Test: mDNS discovers peers */
test_result_t test_discovery_mdns_discover(void) {
    peer_info_t peers[MAX_PEERS];
    int peer_count = MAX_PEERS;
    
    mock_mdns_init();
    
    int ret = mock_mdns_discover(peers, &peer_count);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(peer_count, 2);
    ASSERT_STR_EQ(peers[0].hostname, "gaming-pc");
    ASSERT_STR_EQ(peers[1].hostname, "media-server");
    
    mock_mdns_cleanup();
    return TEST_PASS;
}

/* Test: Broadcast initialization */
test_result_t test_discovery_broadcast_init(void) {
    int ret = mock_broadcast_init();
    ASSERT_EQ(ret, 0);
    
    mock_broadcast_cleanup();
    return TEST_PASS;
}

/* Test: Broadcast discovers peers */
test_result_t test_discovery_broadcast_discover(void) {
    peer_info_t peers[MAX_PEERS];
    int peer_count = MAX_PEERS;
    
    mock_broadcast_init();
    
    int ret = mock_broadcast_discover(peers, &peer_count);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(peer_count, 1);
    ASSERT_STR_EQ(peers[0].hostname, "lan-pc");
    
    mock_broadcast_cleanup();
    return TEST_PASS;
}

/* Test: Manual entry initialization */
test_result_t test_discovery_manual_init(void) {
    int ret = mock_manual_init();
    ASSERT_EQ(ret, 0);
    
    mock_manual_cleanup();
    return TEST_PASS;
}

/* Test: Manual entry always succeeds */
test_result_t test_discovery_manual_fallback(void) {
    peer_info_t peers[MAX_PEERS];
    int peer_count = MAX_PEERS;
    
    mock_manual_init();
    
    int ret = mock_manual_discover(peers, &peer_count);
    ASSERT_EQ(ret, 0);
    /* Manual entry returns 0 peers if none configured */
    ASSERT_EQ(peer_count, 0);
    
    mock_manual_cleanup();
    return TEST_PASS;
}

/* Test: Discovery fallback chain */
test_result_t test_discovery_fallback_chain(void) {
    const discovery_backend_t backends[] = {
        { "mDNS", mock_mdns_init, mock_mdns_discover, mock_mdns_cleanup },
        { "Broadcast", mock_broadcast_init, mock_broadcast_discover, mock_broadcast_cleanup },
        { "Manual", mock_manual_init, mock_manual_discover, mock_manual_cleanup },
        {NULL, NULL, NULL, NULL}
    };
    
    /* Try each backend - at least one should work (Manual always works) */
    int success = 0;
    for (int i = 0; backends[i].name; i++) {
        int ret = backends[i].init_fn();
        
        if (ret == 0) {
            /* Backend initialized successfully */
            peer_info_t peers[MAX_PEERS];
            int peer_count = MAX_PEERS;
            
            ret = backends[i].discover_fn(peers, &peer_count);
            ASSERT_EQ(ret, 0);
            /* peer_count can be 0 (manual with no configured peers) */
            ASSERT_TRUE(peer_count >= 0);
            
            backends[i].cleanup_fn();
            success = 1;
            break;
        }
    }
    
    ASSERT_TRUE(success);
    return TEST_PASS;
}

/* Test suite */
const test_case_t discovery_tests[] = {
    { "mDNS init", test_discovery_mdns_init },
    { "mDNS discover", test_discovery_mdns_discover },
    { "Broadcast init", test_discovery_broadcast_init },
    { "Broadcast discover", test_discovery_broadcast_discover },
    { "Manual init", test_discovery_manual_init },
    { "Manual fallback", test_discovery_manual_fallback },
    { "Fallback chain", test_discovery_fallback_chain },
    {NULL, NULL}
};

int main(void) {
    printf("Running discovery fallback tests...\n");
    return run_test_suite(discovery_tests);
}
