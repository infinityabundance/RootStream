/*
 * main_client.c - Windows Client Entry Point
 *
 * Client-only entry point for Windows builds.
 * Connects to a RootStream host for game streaming.
 */

#ifdef _WIN32

#include "../include/rootstream.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Version info */
#define VERSION "1.0.0"

/* Global context for signal handling */
static rootstream_ctx_t *g_ctx = NULL;

/* Command line options */
typedef struct {
    char peer_code[256];
    uint16_t port;
    bool show_qr;
    bool show_help;
    bool show_version;
} client_options_t;

/* Forward declarations */
static void print_usage(const char *prog);
static void print_version(void);
static int parse_args(int argc, char *argv[], client_options_t *opts);
static void signal_handler(int sig);

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[]) {
    int ret = 0;
    client_options_t opts = {0};
    rootstream_ctx_t ctx = {0};

    /* Default options */
    opts.port = 9876;

    /* Parse command line */
    if (parse_args(argc, argv, &opts) != 0) {
        return 1;
    }

    if (opts.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (opts.show_version) {
        print_version();
        return 0;
    }

    /* Initialize platform */
    if (rs_platform_init() != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    /* Initialize crypto */
    if (crypto_init() != 0) {
        fprintf(stderr, "Failed to initialize crypto\n");
        rs_platform_cleanup();
        return 1;
    }

    /* Initialize context */
    if (rootstream_init(&ctx) != 0) {
        fprintf(stderr, "Failed to initialize RootStream\n");
        rs_platform_cleanup();
        return 1;
    }

    /* Set up signal handler */
    g_ctx = &ctx;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Show QR code if requested */
    if (opts.show_qr) {
        printf("Your RootStream Code:\n");
        printf("  %s\n", ctx.keypair.rootstream_code);
        printf("\nShare this code with hosts to connect.\n");
        rootstream_cleanup(&ctx);
        rs_platform_cleanup();
        return 0;
    }

    /* Connect to peer if specified */
    if (opts.peer_code[0] != '\0') {
        printf("Connecting to: %s\n", opts.peer_code);

        /* Initialize network */
        if (rootstream_net_init(&ctx, opts.port) != 0) {
            fprintf(stderr, "Failed to initialize network\n");
            rootstream_cleanup(&ctx);
            rs_platform_cleanup();
            return 1;
        }

        /* Connect to peer */
        if (rootstream_connect_to_peer(&ctx, opts.peer_code) != 0) {
            fprintf(stderr, "Failed to connect to peer\n");
            rootstream_cleanup(&ctx);
            rs_platform_cleanup();
            return 1;
        }

        printf("Connected! Starting client...\n");

        /* Initialize decoder */
        if (rootstream_decoder_init(&ctx) != 0) {
            fprintf(stderr, "Failed to initialize decoder\n");
            rootstream_cleanup(&ctx);
            rs_platform_cleanup();
            return 1;
        }

        /* Initialize display */
        if (rootstream_display_init(&ctx) != 0) {
            fprintf(stderr, "Failed to initialize display\n");
            rootstream_cleanup(&ctx);
            rs_platform_cleanup();
            return 1;
        }

        /* Initialize audio playback */
        if (rootstream_opus_decoder_init(&ctx) == 0) {
            if (audio_playback_init(&ctx) != 0) {
                fprintf(stderr, "Warning: Audio playback init failed\n");
            }
        }

        /* Run client loop */
        ctx.running = true;
        ret = service_run_client(&ctx);

        /* Cleanup */
        audio_playback_cleanup(&ctx);
        rootstream_opus_cleanup(&ctx);
        rootstream_display_cleanup(&ctx);
        rootstream_decoder_cleanup(&ctx);
    } else {
        print_usage(argv[0]);
        ret = 1;
    }

    rootstream_cleanup(&ctx);
    rs_platform_cleanup();

    return ret;
}

/* ============================================================================
 * Command Line Parsing
 * ============================================================================ */

static void print_usage(const char *prog) {
    printf("RootStream Windows Client v%s\n", VERSION);
    printf("\n");
    printf("Usage: %s [OPTIONS] <peer-code>\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  --port PORT    UDP port (default: 9876)\n");
    printf("  --qr           Show your RootStream code\n");
    printf("  --help         Show this help\n");
    printf("  --version      Show version\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s kXx7Y...@gaming-pc    Connect to host\n", prog);
    printf("  %s --qr                   Show your code\n", prog);
    printf("\n");
    printf("Controls while connected:\n");
    printf("  Escape     Disconnect and exit\n");
    printf("  F11        Toggle fullscreen\n");
}

static void print_version(void) {
    printf("RootStream Windows Client v%s\n", VERSION);
    printf("Platform: Windows\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
}

static int parse_args(int argc, char *argv[], client_options_t *opts) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            opts->show_help = true;
            return 0;
        }
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            opts->show_version = true;
            return 0;
        }
        else if (strcmp(argv[i], "--qr") == 0) {
            opts->show_qr = true;
        }
        else if (strcmp(argv[i], "--port") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --port requires a value\n");
                return -1;
            }
            opts->port = (uint16_t)atoi(argv[++i]);
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
        else {
            /* Peer code */
            strncpy(opts->peer_code, argv[i], sizeof(opts->peer_code) - 1);
        }
    }

    return 0;
}

/* ============================================================================
 * Signal Handling
 * ============================================================================ */

static void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down...\n");
    if (g_ctx) {
        g_ctx->running = false;
    }
}

#endif /* _WIN32 */
