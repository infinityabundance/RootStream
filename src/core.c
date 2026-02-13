/*
 * core.c - Shared core initialization and cleanup
 *
 * Cross-platform helpers used by both Linux host and Windows client.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>

/*
 * Initialize RootStream context
 */
int rootstream_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: NULL context\n");
        return -1;
    }

    memset(ctx, 0, sizeof(rootstream_ctx_t));

    /* Set defaults */
    ctx->capture_mode = CAPTURE_DRM_KMS;
    ctx->display.fd = -1;
    ctx->sock_fd = RS_INVALID_SOCKET;
    ctx->uinput_kbd_fd = -1;
    ctx->uinput_mouse_fd = -1;
    ctx->running = true;
    ctx->port = 0;  /* Will use default */

    /* Initialize crypto library */
    if (crypto_init() < 0) {
        fprintf(stderr, "ERROR: Crypto initialization failed\n");
        fprintf(stderr, "FIX: Ensure libsodium is installed\n");
        return -1;
    }

    /* Load or generate keypair */
    if (config_load(ctx) < 0) {
        fprintf(stderr, "ERROR: Configuration load failed\n");
        return -1;
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  RootStream - Secure P2P Game Streaming       ║\n");
    printf("║  Version %-38s║\n", ROOTSTREAM_VERSION);
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Device Identity: %s\n", ctx->keypair.identity);
    char fingerprint[32];
    if (crypto_format_fingerprint(ctx->keypair.public_key,
                                  CRYPTO_PUBLIC_KEY_BYTES,
                                  fingerprint, sizeof(fingerprint)) == 0) {
        printf("Device Fingerprint: %s\n", fingerprint);
    } else {
        fprintf(stderr, "WARNING: Unable to format device fingerprint\n");
    }
    printf("Your RootStream Code:\n");
    printf("  %s\n", ctx->keypair.rootstream_code);
    printf("\n");

    /* Initialize backend tracking (PHASE 0) */
    ctx->active_backend.capture_name = "uninitialized";
    ctx->active_backend.encoder_name = "uninitialized";
    ctx->active_backend.audio_cap_name = "uninitialized";
    ctx->active_backend.audio_play_name = "uninitialized";
    ctx->active_backend.decoder_name = "uninitialized";

    ctx->backend_prefs.capture_override = NULL;
    ctx->backend_prefs.encoder_override = NULL;
    ctx->backend_prefs.verbose = false;

    printf("INFO: Backend infrastructure initialized\n");

    return 0;
}

/*
 * Cleanup all resources
 */
void rootstream_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx) return;

    printf("\nINFO: Cleaning up...\n");

    /* Stop streaming */
    ctx->running = false;

    /* Cleanup components */
    tray_cleanup(ctx);
    discovery_cleanup(ctx);
    rootstream_encoder_cleanup(ctx);
    rootstream_capture_cleanup(ctx);
    rootstream_input_cleanup(ctx);
    latency_cleanup(&ctx->latency);

    /* Close network socket */
    if (ctx->sock_fd != RS_INVALID_SOCKET) {
        rs_socket_close(ctx->sock_fd);
        ctx->sock_fd = RS_INVALID_SOCKET;
    }

    printf("✓ Cleanup complete\n");
}

/*
 * Print session statistics
 */
void rootstream_print_stats(rootstream_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->frames_captured == 0 && ctx->bytes_sent == 0) {
        return;  /* No activity, skip stats */
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  Session Statistics                            ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Frames captured: %lu\n", ctx->frames_captured);
    printf("  Frames encoded:  %lu\n", ctx->frames_encoded);
    printf("  Data sent:       %.2f MB\n", ctx->bytes_sent / 1024.0 / 1024.0);
}
