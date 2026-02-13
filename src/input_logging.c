/*
 * input_logging.c - Debug input logging (no injection)
 * 
 * Perfect for headless testing and validation.
 * Just prints input events without attempting injection.
 * Never fails - always available.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int input_init_logging(rootstream_ctx_t *ctx) {
    (void)ctx;
    printf("✓ Input logging backend initialized (debug mode)\n");
    printf("  Input events will be logged but NOT injected\n");
    return 0;
}

int input_inject_key_logging(uint32_t keycode, bool press) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

    printf("[%s] INPUT: Key %u %s\n", time_buf, keycode, press ? "DOWN" : "UP");
    return 0;
}

int input_inject_mouse_logging(int x, int y, uint32_t buttons) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

    printf("[%s] INPUT: Mouse at %d,%d buttons=0x%x\n", time_buf, x, y, buttons);
    return 0;
}

void input_cleanup_logging(rootstream_ctx_t *ctx) {
    (void)ctx;
}

bool input_logging_available(void) {
    return true;  /* Always available */
}
