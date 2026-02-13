/*
 * test_network_optimization.c - Unit tests for network optimization
 *
 * Tests:
 * - Network monitor RTT measurement
 * - Adaptive bitrate profile selection
 * - Bandwidth estimation AIMD algorithm
 * - QoS packet classification
 * - Network optimizer integration
 */

#include "../../src/network/network_monitor.h"
#include "../../src/network/adaptive_bitrate.h"
#include "../../src/network/qos_manager.h"
#include "../../src/network/bandwidth_estimator.h"
#include "../../src/network/socket_tuning.h"
#include "../../src/network/network_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("  [TEST] %s... ", #name); \
        fflush(stdout); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
        printf("✓\n"); \
    } \
    static void test_##name(void)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("✗\n"); \
        fprintf(stderr, "    FAILED: %s (line %d)\n", #cond, __LINE__); \
        tests_failed++; \
        tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_LT(a, b) ASSERT((a) < (b))

/* ============================================================================
 * Tests
 * ============================================================================ */

TEST(network_monitor_creation) {
    network_monitor_t *monitor = network_monitor_create();
    ASSERT_NE(monitor, NULL);
    
    /* Check initial conditions */
    network_conditions_t cond = network_monitor_get_conditions(monitor);
    ASSERT_GT(cond.rtt_ms, 0);
    ASSERT_GT(cond.bandwidth_mbps, 0);
    
    network_monitor_destroy(monitor);
}

TEST(network_monitor_rtt_measurement) {
    network_monitor_t *monitor = network_monitor_create();
    ASSERT_NE(monitor, NULL);
    
    /* Simulate packet sent and ack */
    uint64_t send_time = 1000000;  /* 1 second */
    uint64_t ack_time = 1050000;   /* 1.05 seconds (50ms RTT) */
    
    network_monitor_record_packet_sent(monitor, 1, send_time);
    network_monitor_record_packet_ack(monitor, 1, ack_time);
    
    uint32_t rtt = network_monitor_get_rtt_ms(monitor);
    /* RTT should be around 50ms */
    ASSERT_GT(rtt, 0);
    ASSERT_LT(rtt, 100);
    
    network_monitor_destroy(monitor);
}

TEST(network_monitor_packet_loss) {
    network_monitor_t *monitor = network_monitor_create();
    ASSERT_NE(monitor, NULL);
    
    /* Send 10 packets, lose 1 */
    for (uint32_t i = 0; i < 10; i++) {
        network_monitor_record_packet_sent(monitor, i, 1000000 + i * 10000);
    }
    
    /* Ack 9 packets */
    for (uint32_t i = 0; i < 9; i++) {
        network_monitor_record_packet_ack(monitor, i, 1000000 + i * 10000 + 10000);
    }
    
    /* Mark packet 9 as lost */
    network_monitor_record_packet_lost(monitor, 9);
    
    float loss = network_monitor_get_packet_loss(monitor);
    /* Should have some packet loss */
    ASSERT_GT(loss, 0.0f);
    
    network_monitor_destroy(monitor);
}

TEST(abr_controller_creation) {
    network_monitor_t *monitor = network_monitor_create();
    ASSERT_NE(monitor, NULL);
    
    abr_controller_t *abr = abr_controller_create(monitor);
    ASSERT_NE(abr, NULL);
    
    abr_controller_destroy(abr);
    network_monitor_destroy(monitor);
}

TEST(abr_controller_add_profiles) {
    network_monitor_t *monitor = network_monitor_create();
    abr_controller_t *abr = abr_controller_create(monitor);
    
    /* Add some profiles */
    int ret = abr_controller_add_profile(abr, 1000, 640, 480, 30, "H.264", "fast");
    ASSERT_EQ(ret, 0);
    
    ret = abr_controller_add_profile(abr, 5000, 1920, 1080, 60, "H.264", "medium");
    ASSERT_EQ(ret, 0);
    
    /* Get recommended profile */
    const bitrate_profile_t *profile = abr_controller_get_recommended_profile(abr);
    ASSERT_NE(profile, NULL);
    ASSERT_GT(profile->bitrate_kbps, 0);
    
    abr_controller_destroy(abr);
    network_monitor_destroy(monitor);
}

TEST(bandwidth_estimator_aimd) {
    bandwidth_estimator_t *estimator = bandwidth_estimator_create();
    ASSERT_NE(estimator, NULL);
    
    uint32_t initial_bw = bandwidth_estimator_get_estimated_bandwidth_mbps(estimator);
    ASSERT_GT(initial_bw, 0);
    
    /* Test additive increase */
    bandwidth_estimator_aimd_increase(estimator);
    uint32_t increased_bw = bandwidth_estimator_get_estimated_bandwidth_mbps(estimator);
    ASSERT_GT(increased_bw, initial_bw);
    
    /* Test multiplicative decrease */
    bandwidth_estimator_aimd_decrease(estimator);
    uint32_t decreased_bw = bandwidth_estimator_get_estimated_bandwidth_mbps(estimator);
    ASSERT_LT(decreased_bw, increased_bw);
    
    bandwidth_estimator_destroy(estimator);
}

TEST(qos_manager_creation) {
    qos_manager_t *qos = qos_manager_create();
    ASSERT_NE(qos, NULL);
    
    /* Register a traffic class */
    int ret = qos_manager_register_traffic_class(qos, "Test", PRIORITY_HIGH, 5000);
    ASSERT_EQ(ret, 0);
    
    qos_manager_destroy(qos);
}

TEST(qos_manager_packet_classification) {
    qos_manager_t *qos = qos_manager_create();
    ASSERT_NE(qos, NULL);
    
    /* Test packet classification */
    uint8_t small_packet[100] = {0};
    uint8_t large_packet[15000] = {0};
    
    packet_priority_t priority_small = qos_manager_classify_packet(qos, small_packet, sizeof(small_packet));
    packet_priority_t priority_large = qos_manager_classify_packet(qos, large_packet, sizeof(large_packet));
    
    /* Large packets should have higher priority (keyframes) */
    ASSERT_GT(priority_large, priority_small);
    
    qos_manager_destroy(qos);
}

TEST(socket_tuning_creation) {
    socket_tuning_t *tuning = socket_tuning_create();
    ASSERT_NE(tuning, NULL);
    
    socket_tuning_destroy(tuning);
}

TEST(network_optimizer_creation) {
    network_optimizer_t *optimizer = network_optimizer_create();
    ASSERT_NE(optimizer, NULL);
    
    /* Initialize with NULL callbacks */
    int ret = network_optimizer_init(optimizer, NULL);
    ASSERT_EQ(ret, 0);
    
    network_optimizer_destroy(optimizer);
}

TEST(network_optimizer_profiles) {
    network_optimizer_t *optimizer = network_optimizer_create();
    ASSERT_NE(optimizer, NULL);
    
    /* Setup default profiles */
    int ret = network_optimizer_setup_default_profiles(optimizer);
    ASSERT_EQ(ret, 0);
    
    /* Get recommended bitrate */
    uint32_t bitrate = network_optimizer_get_recommended_bitrate(optimizer);
    ASSERT_GT(bitrate, 0);
    
    network_optimizer_destroy(optimizer);
}

TEST(network_optimizer_optimize) {
    network_optimizer_t *optimizer = network_optimizer_create();
    network_optimizer_init(optimizer, NULL);
    network_optimizer_setup_default_profiles(optimizer);
    
    /* Run optimization */
    int ret = network_optimizer_optimize(optimizer);
    ASSERT_EQ(ret, 0);
    
    /* Get conditions */
    network_conditions_t cond = network_optimizer_get_conditions(optimizer);
    ASSERT_GT(cond.rtt_ms, 0);
    ASSERT_GT(cond.bandwidth_mbps, 0);
    
    network_optimizer_destroy(optimizer);
}

TEST(network_optimizer_diagnostics_json) {
    network_optimizer_t *optimizer = network_optimizer_create();
    network_optimizer_init(optimizer, NULL);
    network_optimizer_setup_default_profiles(optimizer);
    
    /* Get diagnostics JSON */
    char *json = network_optimizer_get_diagnostics_json(optimizer);
    ASSERT_NE(json, NULL);
    
    /* Check that JSON contains expected fields */
    ASSERT_NE(strstr(json, "network"), NULL);
    ASSERT_NE(strstr(json, "rtt_ms"), NULL);
    ASSERT_NE(strstr(json, "bandwidth_mbps"), NULL);
    
    free(json);
    network_optimizer_destroy(optimizer);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║     Network Optimization Unit Tests                           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("Running Network Monitor Tests:\n");
    run_test_network_monitor_creation();
    run_test_network_monitor_rtt_measurement();
    run_test_network_monitor_packet_loss();
    
    printf("\nRunning Adaptive Bitrate Tests:\n");
    run_test_abr_controller_creation();
    run_test_abr_controller_add_profiles();
    
    printf("\nRunning Bandwidth Estimator Tests:\n");
    run_test_bandwidth_estimator_aimd();
    
    printf("\nRunning QoS Manager Tests:\n");
    run_test_qos_manager_creation();
    run_test_qos_manager_packet_classification();
    
    printf("\nRunning Socket Tuning Tests:\n");
    run_test_socket_tuning_creation();
    
    printf("\nRunning Network Optimizer Tests:\n");
    run_test_network_optimizer_creation();
    run_test_network_optimizer_profiles();
    run_test_network_optimizer_optimize();
    run_test_network_optimizer_diagnostics_json();

    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d (%.1f%%)\n", tests_passed, 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("  Failed: %d\n", tests_failed);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
