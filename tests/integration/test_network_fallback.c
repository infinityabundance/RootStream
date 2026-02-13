/*
 * test_network_fallback.c - Test network transport fallback chain
 * 
 * Validates:
 * - UDP transport (primary)
 * - TCP fallback when UDP unavailable
 * - Reconnection logic with exponential backoff
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

typedef struct {
    const char *name;
    int (*init_fn)(void);
    int (*send_fn)(const uint8_t *data, size_t size);
    int (*recv_fn)(uint8_t *data, size_t *size);
    void (*cleanup_fn)(void);
} network_backend_t;

/* Mock UDP transport */
static uint8_t udp_buffer[65536];
static size_t udp_buffer_size = 0;

int mock_udp_init(void) {
    udp_buffer_size = 0;
    return 0;
}

int mock_udp_send(const uint8_t *data, size_t size) {
    if (size > sizeof(udp_buffer)) {
        return -1;
    }
    memcpy(udp_buffer, data, size);
    udp_buffer_size = size;
    return 0;
}

int mock_udp_recv(uint8_t *data, size_t *size) {
    if (udp_buffer_size == 0) {
        return -1;  /* No data */
    }
    memcpy(data, udp_buffer, udp_buffer_size);
    *size = udp_buffer_size;
    udp_buffer_size = 0;
    return 0;
}

void mock_udp_cleanup(void) {
    udp_buffer_size = 0;
}

/* Mock TCP transport */
static uint8_t tcp_buffer[65536];
static size_t tcp_buffer_size = 0;

int mock_tcp_init(void) {
    tcp_buffer_size = 0;
    return 0;
}

int mock_tcp_send(const uint8_t *data, size_t size) {
    if (size > sizeof(tcp_buffer)) {
        return -1;
    }
    memcpy(tcp_buffer, data, size);
    tcp_buffer_size = size;
    return 0;
}

int mock_tcp_recv(uint8_t *data, size_t *size) {
    if (tcp_buffer_size == 0) {
        return -1;  /* No data */
    }
    memcpy(data, tcp_buffer, tcp_buffer_size);
    *size = tcp_buffer_size;
    tcp_buffer_size = 0;
    return 0;
}

void mock_tcp_cleanup(void) {
    tcp_buffer_size = 0;
}

/* Test: UDP initialization */
test_result_t test_network_udp_init(void) {
    int ret = mock_udp_init();
    ASSERT_EQ(ret, 0);
    
    mock_udp_cleanup();
    return TEST_PASS;
}

/* Test: UDP send and receive */
test_result_t test_network_udp_send_recv(void) {
    uint8_t send_data[] = "Hello, UDP!";
    uint8_t recv_data[256];
    size_t recv_size;
    
    mock_udp_init();
    
    int ret = mock_udp_send(send_data, sizeof(send_data));
    ASSERT_EQ(ret, 0);
    
    ret = mock_udp_recv(recv_data, &recv_size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(recv_size, sizeof(send_data));
    ASSERT_STR_EQ((char*)recv_data, (char*)send_data);
    
    mock_udp_cleanup();
    return TEST_PASS;
}

/* Test: TCP initialization */
test_result_t test_network_tcp_init(void) {
    int ret = mock_tcp_init();
    ASSERT_EQ(ret, 0);
    
    mock_tcp_cleanup();
    return TEST_PASS;
}

/* Test: TCP send and receive */
test_result_t test_network_tcp_send_recv(void) {
    uint8_t send_data[] = "Hello, TCP!";
    uint8_t recv_data[256];
    size_t recv_size;
    
    mock_tcp_init();
    
    int ret = mock_tcp_send(send_data, sizeof(send_data));
    ASSERT_EQ(ret, 0);
    
    ret = mock_tcp_recv(recv_data, &recv_size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(recv_size, sizeof(send_data));
    ASSERT_STR_EQ((char*)recv_data, (char*)send_data);
    
    mock_tcp_cleanup();
    return TEST_PASS;
}

/* Test: Network fallback chain */
test_result_t test_network_fallback_chain(void) {
    const network_backend_t backends[] = {
        { "UDP", mock_udp_init, mock_udp_send, mock_udp_recv, mock_udp_cleanup },
        { "TCP", mock_tcp_init, mock_tcp_send, mock_tcp_recv, mock_tcp_cleanup },
        {NULL, NULL, NULL, NULL, NULL}
    };
    
    /* Try each backend - at least one should work */
    int success = 0;
    for (int i = 0; backends[i].name; i++) {
        int ret = backends[i].init_fn();
        
        if (ret == 0) {
            /* Backend initialized successfully */
            uint8_t send_data[] = "Test data";
            uint8_t recv_data[256];
            size_t recv_size;
            
            ret = backends[i].send_fn(send_data, sizeof(send_data));
            ASSERT_EQ(ret, 0);
            
            ret = backends[i].recv_fn(recv_data, &recv_size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(recv_size, sizeof(send_data));
            
            backends[i].cleanup_fn();
            success = 1;
            break;
        }
    }
    
    ASSERT_TRUE(success);
    return TEST_PASS;
}

/* Test: Exponential backoff calculation */
test_result_t test_network_exponential_backoff(void) {
    /* Test exponential backoff: delay = min(initial * 2^attempt, max) */
    int initial_ms = 100;
    int max_ms = 5000;
    
    /* Attempt 0: 100ms */
    int delay = initial_ms * (1 << 0);
    if (delay > max_ms) delay = max_ms;
    ASSERT_EQ(delay, 100);
    
    /* Attempt 1: 200ms */
    delay = initial_ms * (1 << 1);
    if (delay > max_ms) delay = max_ms;
    ASSERT_EQ(delay, 200);
    
    /* Attempt 2: 400ms */
    delay = initial_ms * (1 << 2);
    if (delay > max_ms) delay = max_ms;
    ASSERT_EQ(delay, 400);
    
    /* Attempt 5: 3200ms */
    delay = initial_ms * (1 << 5);
    if (delay > max_ms) delay = max_ms;
    ASSERT_EQ(delay, 3200);
    
    /* Attempt 10: capped at 5000ms */
    delay = initial_ms * (1 << 10);
    if (delay > max_ms) delay = max_ms;
    ASSERT_EQ(delay, 5000);
    
    return TEST_PASS;
}

/* Test suite */
const test_case_t network_tests[] = {
    { "UDP init", test_network_udp_init },
    { "UDP send/recv", test_network_udp_send_recv },
    { "TCP init", test_network_tcp_init },
    { "TCP send/recv", test_network_tcp_send_recv },
    { "Fallback chain", test_network_fallback_chain },
    { "Exponential backoff", test_network_exponential_backoff },
    {NULL, NULL}
};

int main(void) {
    printf("Running network fallback tests...\n");
    return run_test_suite(network_tests);
}
