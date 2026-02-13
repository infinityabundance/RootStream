/**
 * PHASE 19: Web Dashboard - Unit Tests
 * 
 * Tests for API server, WebSocket server, and authentication
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/web/api_server.h"
#include "../src/web/websocket_server.h"
#include "../src/web/auth_manager.h"
#include "../src/web/rate_limiter.h"
#include "../src/web/models.h"

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        if (test_##name()) { \
            printf("  ✓ PASSED\n"); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAILED\n"); \
        } \
    } while (0)

/* API Server Tests */
static int test_api_server_init(void) {
    api_server_config_t config = {
        .port = 8080,
        .enable_https = false,
        .cert_file = NULL,
        .key_file = NULL,
        .max_connections = 100,
        .timeout_seconds = 30
    };

    api_server_t *server = api_server_init(&config);
    if (!server) {
        return 0;
    }

    api_server_cleanup(server);
    return 1;
}

static int test_api_server_start_stop(void) {
    api_server_config_t config = {
        .port = 8080,
        .enable_https = false,
        .cert_file = NULL,
        .key_file = NULL,
        .max_connections = 100,
        .timeout_seconds = 30
    };

    api_server_t *server = api_server_init(&config);
    if (!server) {
        return 0;
    }

    if (api_server_start(server) != 0) {
        api_server_cleanup(server);
        return 0;
    }

    if (api_server_stop(server) != 0) {
        api_server_cleanup(server);
        return 0;
    }

    api_server_cleanup(server);
    return 1;
}

/* WebSocket Server Tests */
static int test_websocket_server_init(void) {
    websocket_server_config_t config = {
        .port = 8081,
        .enable_wss = false,
        .cert_file = NULL,
        .key_file = NULL
    };

    websocket_server_t *server = websocket_server_init(&config);
    if (!server) {
        return 0;
    }

    websocket_server_cleanup(server);
    return 1;
}

static int test_websocket_server_broadcast(void) {
    websocket_server_config_t config = {
        .port = 8081,
        .enable_wss = false,
        .cert_file = NULL,
        .key_file = NULL
    };

    websocket_server_t *server = websocket_server_init(&config);
    if (!server) {
        return 0;
    }

    // Start server first
    if (websocket_server_start(server) != 0) {
        websocket_server_cleanup(server);
        return 0;
    }

    metrics_snapshot_t metrics = {
        .fps = 60,
        .rtt_ms = 15,
        .jitter_ms = 2,
        .gpu_util = 45,
        .gpu_temp = 65,
        .cpu_util = 30,
        .bandwidth_mbps = 25.5,
        .packets_sent = 150000,
        .packets_lost = 12,
        .bytes_sent = 50000000,
        .timestamp_us = 1234567890
    };

    // Should not fail even without connected clients
    int result = websocket_server_broadcast_metrics(server, &metrics);

    websocket_server_stop(server);
    websocket_server_cleanup(server);
    return result == 0;
}

/* Authentication Manager Tests */
static int test_auth_manager_init(void) {
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        return 0;
    }

    auth_manager_cleanup(auth);
    return 1;
}

static int test_auth_manager_add_user(void) {
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        return 0;
    }

    int result = auth_manager_add_user(auth, "testuser", "password123", ROLE_OPERATOR);
    
    auth_manager_cleanup(auth);
    return result == 0;
}

static int test_auth_manager_authenticate(void) {
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        return 0;
    }

    // Add user
    auth_manager_add_user(auth, "testuser", "password123", ROLE_OPERATOR);

    // Try to authenticate
    char token[512];
    int result = auth_manager_authenticate(auth, "testuser", "password123", token, sizeof(token));

    if (result != 0) {
        auth_manager_cleanup(auth);
        return 0;
    }

    // Token should not be empty
    if (strlen(token) == 0) {
        auth_manager_cleanup(auth);
        return 0;
    }

    auth_manager_cleanup(auth);
    return 1;
}

static int test_auth_manager_verify_token(void) {
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        return 0;
    }

    // Add user and authenticate
    auth_manager_add_user(auth, "testuser", "password123", ROLE_OPERATOR);
    char token[512];
    auth_manager_authenticate(auth, "testuser", "password123", token, sizeof(token));

    // Verify token
    char username[256];
    user_role_t role;
    int result = auth_manager_verify_token(auth, token, username, sizeof(username), &role);

    if (result != 0) {
        auth_manager_cleanup(auth);
        return 0;
    }

    if (strcmp(username, "testuser") != 0 || role != ROLE_OPERATOR) {
        auth_manager_cleanup(auth);
        return 0;
    }

    auth_manager_cleanup(auth);
    return 1;
}

static int test_auth_manager_wrong_password(void) {
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        return 0;
    }

    // Add user
    auth_manager_add_user(auth, "testuser", "password123", ROLE_OPERATOR);

    // Try to authenticate with wrong password
    char token[512];
    int result = auth_manager_authenticate(auth, "testuser", "wrongpassword", token, sizeof(token));

    auth_manager_cleanup(auth);
    
    // Should fail
    return result != 0;
}

static int test_auth_manager_permissions(void) {
    // Test ADMIN permissions
    if (!auth_manager_can_control_streaming(ROLE_ADMIN)) return 0;
    if (!auth_manager_can_modify_settings(ROLE_ADMIN)) return 0;
    if (!auth_manager_can_manage_users(ROLE_ADMIN)) return 0;

    // Test OPERATOR permissions
    if (!auth_manager_can_control_streaming(ROLE_OPERATOR)) return 0;
    if (!auth_manager_can_modify_settings(ROLE_OPERATOR)) return 0;
    if (auth_manager_can_manage_users(ROLE_OPERATOR)) return 0;

    // Test VIEWER permissions
    if (auth_manager_can_control_streaming(ROLE_VIEWER)) return 0;
    if (auth_manager_can_modify_settings(ROLE_VIEWER)) return 0;
    if (auth_manager_can_manage_users(ROLE_VIEWER)) return 0;

    return 1;
}

/* Rate Limiter Tests */
static int test_rate_limiter_init(void) {
    rate_limiter_t *limiter = rate_limiter_init(100);
    if (!limiter) {
        return 0;
    }

    rate_limiter_cleanup(limiter);
    return 1;
}

static int test_rate_limiter_enforcement(void) {
    rate_limiter_t *limiter = rate_limiter_init(10);
    if (!limiter) {
        return 0;
    }

    // First 10 requests should not be limited
    for (int i = 0; i < 10; i++) {
        if (rate_limiter_is_limited(limiter, "127.0.0.1")) {
            rate_limiter_cleanup(limiter);
            return 0;
        }
    }

    // 11th request should be limited
    if (!rate_limiter_is_limited(limiter, "127.0.0.1")) {
        rate_limiter_cleanup(limiter);
        return 0;
    }

    rate_limiter_cleanup(limiter);
    return 1;
}

static int test_rate_limiter_different_clients(void) {
    rate_limiter_t *limiter = rate_limiter_init(10);
    if (!limiter) {
        return 0;
    }

    // Each client should have separate limits
    for (int i = 0; i < 10; i++) {
        if (rate_limiter_is_limited(limiter, "127.0.0.1")) {
            rate_limiter_cleanup(limiter);
            return 0;
        }
        if (rate_limiter_is_limited(limiter, "192.168.1.1")) {
            rate_limiter_cleanup(limiter);
            return 0;
        }
    }

    rate_limiter_cleanup(limiter);
    return 1;
}

/* Main test runner */
int main(void) {
    printf("\n=== PHASE 19: Web Dashboard Unit Tests ===\n\n");

    // API Server tests
    TEST(api_server_init);
    TEST(api_server_start_stop);

    // WebSocket Server tests
    TEST(websocket_server_init);
    TEST(websocket_server_broadcast);

    // Authentication Manager tests
    TEST(auth_manager_init);
    TEST(auth_manager_add_user);
    TEST(auth_manager_authenticate);
    TEST(auth_manager_verify_token);
    TEST(auth_manager_wrong_password);
    TEST(auth_manager_permissions);

    // Rate Limiter tests
    TEST(rate_limiter_init);
    TEST(rate_limiter_enforcement);
    TEST(rate_limiter_different_clients);

    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    return (tests_run == tests_passed) ? 0 : 1;
}
