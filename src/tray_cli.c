/*
 * tray_cli.c - CLI-only mode (no UI)
 * 
 * Minimal mode with no interactive UI.
 * Perfect for automation, scripts, or background services.
 * Just command-line options and status messages.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tray_init_cli(rootstream_ctx_t *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    ctx->tray_priv = NULL;
    printf("✓ CLI-only mode initialized (no GUI)\n");
    return 0;
}

void tray_update_status_cli(rootstream_ctx_t *ctx, tray_status_t status) {
    if (!ctx) return;
    
    const char *status_str = "UNKNOWN";
    switch (status) {
        case STATUS_IDLE: status_str = "IDLE"; break;
        case STATUS_HOSTING: status_str = "HOSTING"; break;
        case STATUS_CONNECTED: status_str = "CONNECTED"; break;
        case STATUS_ERROR: status_str = "ERROR"; break;
    }
    
    printf("INFO: Status changed to %s\n", status_str);
}

void tray_show_qr_code_cli(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║          Your RootStream Code                  ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("%s\n", ctx->keypair.rootstream_code);
    printf("\n");
    printf("Share this code with peers to connect.\n");
    printf("\n");
}

void tray_show_peers_cli(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║          Connected Peers (%d)                   ║\n", ctx->num_peers);
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (ctx->num_peers == 0) {
        printf("  No peers connected.\n");
    } else {
        for (int i = 0; i < ctx->num_peers; i++) {
            printf("  %d. %s (%s) - %s\n",
                   i + 1, ctx->peers[i].hostname,
                   ctx->peers[i].hostname,
                   ctx->peers[i].state == PEER_CONNECTED ? "online" : "offline");
        }
    }
    printf("\n");
}

void tray_run_cli(rootstream_ctx_t *ctx) {
    (void)ctx;
    /* CLI mode doesn't need to run any event loop */
}

void tray_cleanup_cli(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    /* Nothing to cleanup for CLI mode */
    ctx->tray_priv = NULL;
}
