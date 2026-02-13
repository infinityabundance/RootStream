/*
 * test_input_manager.c - Unit tests for input manager (PHASE 15)
 * 
 * Tests input injection, deduplication, latency measurement,
 * and multi-client support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"
#include "../../include/rootstream.h"

#ifdef __linux__
#include <linux/input.h>
#endif

/* Test: Input manager initialization */
test_result_t test_input_manager_init(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Initialize with logging backend (works on all platforms) */
    int result = input_manager_init(&ctx, INPUT_BACKEND_LOGGING);
    ASSERT_EQ(result, 0);
    ASSERT_NOT_NULL(ctx.input_manager);
    ASSERT_TRUE(ctx.input_manager->initialized);
    
    input_manager_cleanup(&ctx);
    ASSERT_NULL(ctx.input_manager);
    
    return TEST_PASS;
}

/* Test: Client registration */
test_result_t test_input_manager_client_registration(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    /* Register first client */
    int result = input_manager_register_client(&ctx, 1, "TestClient1");
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ctx.input_manager->active_client_count, 1);
    
    /* Register second client */
    result = input_manager_register_client(&ctx, 2, "TestClient2");
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ctx.input_manager->active_client_count, 2);
    
    /* Unregister first client */
    result = input_manager_unregister_client(&ctx, 1);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ctx.input_manager->active_client_count, 1);
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Input packet submission */
test_result_t test_input_manager_submit_packet(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    input_manager_register_client(&ctx, 1, "TestClient");
    
    /* Submit a keyboard event */
    input_event_pkt_t event = {0};
#ifdef __linux__
    event.type = EV_KEY;
    event.code = KEY_A;
    event.value = 1;  /* Press */
#else
    event.type = 1;
    event.code = 30;
    event.value = 1;
#endif
    
    int result = input_manager_submit_packet(&ctx, &event, 1, 1, 1000);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 1);
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Duplicate detection */
test_result_t test_input_manager_duplicate_detection(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    input_manager_register_client(&ctx, 1, "TestClient");
    
    /* Submit same event twice with same sequence number */
    input_event_pkt_t event = {0};
#ifdef __linux__
    event.type = EV_KEY;
    event.code = KEY_A;
#else
    event.type = 1;
    event.code = 30;
#endif
    event.value = 1;
    
    input_manager_submit_packet(&ctx, &event, 1, 100, 1000);
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 1);
    ASSERT_EQ(input_manager_get_duplicates(&ctx), 0);
    
    /* Submit duplicate */
    input_manager_submit_packet(&ctx, &event, 1, 100, 1000);
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 1);  /* Not incremented */
    ASSERT_EQ(input_manager_get_duplicates(&ctx), 1);    /* Duplicate detected */
    
    /* Submit with new sequence number */
    input_manager_submit_packet(&ctx, &event, 1, 101, 2000);
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 2);  /* Incremented */
    ASSERT_EQ(input_manager_get_duplicates(&ctx), 1);    /* Still 1 duplicate */
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Multi-client support */
test_result_t test_input_manager_multi_client(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    /* Register multiple clients */
    input_manager_register_client(&ctx, 1, "Client1");
    input_manager_register_client(&ctx, 2, "Client2");
    input_manager_register_client(&ctx, 3, "Client3");
    
    ASSERT_EQ(ctx.input_manager->active_client_count, 3);
    
    /* Submit events from different clients */
    input_event_pkt_t event = {0};
#ifdef __linux__
    event.type = EV_KEY;
    event.code = KEY_A;
#else
    event.type = 1;
    event.code = 30;
#endif
    event.value = 1;
    
    input_manager_submit_packet(&ctx, &event, 1, 1, 1000);
    input_manager_submit_packet(&ctx, &event, 2, 1, 2000);
    input_manager_submit_packet(&ctx, &event, 3, 1, 3000);
    
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 3);
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Latency measurement */
test_result_t test_input_manager_latency(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    input_manager_register_client(&ctx, 1, "TestClient");
    
    /* Submit events with timestamps */
    input_event_pkt_t event = {0};
#ifdef __linux__
    event.type = EV_KEY;
    event.code = KEY_A;
#else
    event.type = 1;
    event.code = 30;
#endif
    event.value = 1;
    
    uint64_t timestamp = get_timestamp_us() - 10000;  /* 10ms ago */
    input_manager_submit_packet(&ctx, &event, 1, 1, timestamp);
    
    /* Latency should be measurable */
    uint32_t latency = input_manager_get_latency_ms(&ctx);
    ASSERT_TRUE(latency >= 10);  /* At least 10ms */
    ASSERT_TRUE(latency < 1000); /* Less than 1 second */
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Backend selection */
test_result_t test_input_manager_backend_selection(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Test logging backend */
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) == 0) {
        ASSERT_STR_EQ(ctx.active_backend.input_name, "logging");
        input_manager_cleanup(&ctx);
    }
    
    /* Test xdotool backend if available */
    if (input_xdotool_available()) {
        if (input_manager_init(&ctx, INPUT_BACKEND_XDOTOOL) == 0) {
            ASSERT_STR_EQ(ctx.active_backend.input_name, "xdotool");
            input_manager_cleanup(&ctx);
        }
    }
    
    return TEST_PASS;
}

/* Test: Statistics tracking */
test_result_t test_input_manager_statistics(void) {
    rootstream_ctx_t ctx = {0};
    
    if (input_manager_init(&ctx, INPUT_BACKEND_LOGGING) != 0) {
        return TEST_SKIP;
    }
    
    input_manager_register_client(&ctx, 1, "TestClient");
    
    /* Initial stats */
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 0);
    ASSERT_EQ(input_manager_get_duplicates(&ctx), 0);
    
    /* Submit events */
    input_event_pkt_t event = {0};
#ifdef __linux__
    event.type = EV_KEY;
    event.code = KEY_A;
#else
    event.type = 1;
    event.code = 30;
#endif
    event.value = 1;
    
    for (int i = 0; i < 10; i++) {
        input_manager_submit_packet(&ctx, &event, 1, i, 1000 + i);
    }
    
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 10);
    
    /* Submit some duplicates */
    input_manager_submit_packet(&ctx, &event, 1, 5, 2000);  /* Duplicate */
    input_manager_submit_packet(&ctx, &event, 1, 7, 2000);  /* Duplicate */
    
    ASSERT_EQ(input_manager_get_total_inputs(&ctx), 10);  /* No change */
    ASSERT_EQ(input_manager_get_duplicates(&ctx), 2);
    
    input_manager_cleanup(&ctx);
    return TEST_PASS;
}

const test_case_t input_manager_tests[] = {
    { "Input manager initialization", test_input_manager_init },
    { "Client registration", test_input_manager_client_registration },
    { "Input packet submission", test_input_manager_submit_packet },
    { "Duplicate detection", test_input_manager_duplicate_detection },
    { "Multi-client support", test_input_manager_multi_client },
    { "Latency measurement", test_input_manager_latency },
    { "Backend selection", test_input_manager_backend_selection },
    { "Statistics tracking", test_input_manager_statistics },
    {NULL, NULL}
};

int main(void) {
    printf("Running PHASE 15 Input Manager tests...\n");
    return run_test_suite(input_manager_tests);
}
