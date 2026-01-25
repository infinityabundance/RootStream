/*
 * tray_stub.c - Headless tray implementation
 *
 * Used when building with HEADLESS=1 to allow compiling without GTK3.
 * Every entry point logs a clear message and returns safely.
 */

#include "../include/rootstream.h"
#include <stdio.h>

int tray_init(rootstream_ctx_t *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argc;
    (void)argv;
    fprintf(stderr, "ERROR: Tray UI unavailable in headless build\n");
    fprintf(stderr, "FIX: Install GTK3 dev packages or rebuild without HEADLESS=1\n");
    return -1;
}

void tray_update_status(rootstream_ctx_t *ctx, tray_status_t status) {
    (void)ctx;
    (void)status;
}

void tray_show_qr_code(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_show_peers(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_run(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}
