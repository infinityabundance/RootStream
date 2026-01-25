/*
 * qrcode_stub.c - Stubbed QR functions for NO_QR builds
 *
 * Used when qrencode/libpng headers are unavailable.
 */

#include "../include/rootstream.h"
#include <stdio.h>

int qrcode_generate(const char *data, const char *output_file) {
    (void)data;
    (void)output_file;
    fprintf(stderr, "ERROR: QR generation unavailable (NO_QR build)\n");
    fprintf(stderr, "FIX: Install libqrencode/libpng dev packages and rebuild\n");
    return -1;
}

int qrcode_display(rootstream_ctx_t *ctx, const char *rootstream_code) {
    (void)ctx;
    (void)rootstream_code;
    fprintf(stderr, "ERROR: QR display unavailable (NO_QR build)\n");
    return -1;
}

void qrcode_print_terminal(const char *data) {
    (void)data;
    fprintf(stderr, "ERROR: QR terminal output unavailable (NO_QR build)\n");
}
