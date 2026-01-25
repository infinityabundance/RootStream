/*
 * main.c - RootStream main application
 * 
 * Usage:
 *   rootstream host [--display N] [--port PORT]
 *   rootstream client HOST [--port PORT]
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static volatile bool keep_running = true;

static void signal_handler(int sig) {
    (void)sig;
    keep_running = false;
}

/*
 * Print banner
 */
static void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║         RootStream v%s                     ║\n", ROOTSTREAM_VERSION);
    printf("║  Native Linux Game Streaming - No BS         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    printf("\n");
}

/*
 * Host main loop
 */
int rootstream_run_host(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    printf("\n🎮 Host mode - waiting for client...\n\n");

    /* Allocate encoding buffer */
    size_t enc_buf_size = ctx->display.width * ctx->display.height;
    uint8_t *enc_buf = malloc(enc_buf_size);
    if (!enc_buf) {
        fprintf(stderr, "Cannot allocate encoding buffer\n");
        return -1;
    }

    /* Stats */
    time_t last_stats = time(NULL);
    uint64_t last_frames = 0;

    ctx->running = true;
    
    while (keep_running && ctx->running) {
        /* Capture frame */
        if (rootstream_capture_frame(ctx, &ctx->current_frame) < 0) {
            fprintf(stderr, "Capture failed: %s\n", rootstream_get_error());
            usleep(16000);  /* ~60 FPS */
            continue;
        }

        /* Encode frame */
        size_t enc_size = 0;
        if (rootstream_encode_frame(ctx, &ctx->current_frame, 
                                    enc_buf, &enc_size) < 0) {
            fprintf(stderr, "Encoding failed: %s\n", rootstream_get_error());
            continue;
        }

        /* Send encoded frame */
        if (enc_size > 0) {
            if (rootstream_net_send(ctx, PACKET_VIDEO, enc_buf, enc_size) < 0) {
                /* Client might not be connected yet, that's okay */
            }
        }

        /* Process input from client */
        input_event_pkt_t input;
        int recv = rootstream_net_recv(ctx, &input, sizeof(input), 1);
        if (recv > 0) {
            rootstream_input_process(ctx, &input);
        }

        /* Print stats every 5 seconds */
        time_t now = time(NULL);
        if (now - last_stats >= 5) {
            uint64_t fps = (ctx->frames_captured - last_frames) / 5;
            double mbps = (ctx->bytes_sent / 1024.0 / 1024.0 * 8) / 5;
            
            printf("📊 FPS: %lu | Bitrate: %.2f Mbps | Frames: %lu/%lu\n",
                   fps, mbps, ctx->frames_captured, ctx->frames_encoded);
            
            last_frames = ctx->frames_captured;
            ctx->bytes_sent = 0;
            last_stats = now;
        }

        /* Rate limiting to target FPS */
        usleep(1000000 / ctx->display.refresh_rate);
    }

    free(enc_buf);
    printf("\n✓ Host stopped\n");
    return 0;
}

/*
 * Client main loop
 */
int rootstream_run_client(rootstream_ctx_t *ctx, const char *host_addr) {
    if (!ctx || !host_addr) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    printf("\n📺 Client mode - connecting to %s...\n\n", host_addr);

    /* TODO: Implement client (decoder + display) */
    /* This would involve:
     *   - Receiving video packets
     *   - Decoding with VA-API
     *   - Displaying with SDL2 or DRM/KMS
     *   - Capturing local input
     *   - Sending input to host
     */

    printf("Client mode not yet implemented\n");
    printf("(Decoder + display code would go here)\n");

    return 0;
}

/*
 * Initialize context
 */
int rootstream_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    memset(ctx, 0, sizeof(rootstream_ctx_t));
    
    ctx->capture_mode = CAPTURE_DRM_KMS;
    ctx->display.fd = -1;
    ctx->sock_fd = -1;
    ctx->uinput_kbd_fd = -1;
    ctx->uinput_mouse_fd = -1;
    
    return 0;
}

/*
 * Cleanup context
 */
void rootstream_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    rootstream_encoder_cleanup(ctx);
    rootstream_capture_cleanup(ctx);
    rootstream_input_cleanup(ctx);

    if (ctx->sock_fd >= 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
    }
}

/*
 * Print statistics
 */
void rootstream_print_stats(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    printf("\n📊 Session Statistics:\n");
    printf("   Frames captured: %lu\n", ctx->frames_captured);
    printf("   Frames encoded:  %lu\n", ctx->frames_encoded);
    printf("   Data sent:       %.2f MB\n", ctx->bytes_sent / 1024.0 / 1024.0);
}

/*
 * Select display
 */
int rootstream_select_display(rootstream_ctx_t *ctx, int display_index) {
    display_info_t displays[MAX_DISPLAYS];
    
    int count = rootstream_detect_displays(displays, MAX_DISPLAYS);
    if (count < 0) {
        fprintf(stderr, "Error: %s\n", rootstream_get_error());
        return -1;
    }

    if (display_index >= count) {
        fprintf(stderr, "Display %d not found (only %d available)\n", 
                display_index, count);
        return -1;
    }

    /* Copy selected display info */
    memcpy(&ctx->display, &displays[display_index], sizeof(display_info_t));
    
    /* Close other display fds */
    for (int i = 0; i < count; i++) {
        if (i != display_index && displays[i].fd >= 0) {
            close(displays[i].fd);
        }
    }

    return 0;
}

/*
 * Main entry point
 */
int main(int argc, char **argv) {
    print_banner();

    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s host [--display N] [--port PORT]\n", argv[0]);
        printf("  %s client HOST [--port PORT]\n", argv[0]);
        printf("\n");
        printf("Examples:\n");
        printf("  %s host                    # Start host on default display\n", argv[0]);
        printf("  %s host --display 1        # Use second display\n", argv[0]);
        printf("  %s client 192.168.1.100    # Connect to host\n", argv[0]);
        printf("\n");
        return 1;
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Parse mode */
    bool is_host = strcmp(argv[1], "host") == 0;
    bool is_client = strcmp(argv[1], "client") == 0;

    if (!is_host && !is_client) {
        fprintf(stderr, "Invalid mode: %s\n", argv[1]);
        return 1;
    }

    /* Parse options */
    int display_idx = 0;
    uint16_t port = 9876;
    const char *host_addr = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--display") == 0 && i + 1 < argc) {
            display_idx = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (is_client && !host_addr) {
            host_addr = argv[i];
        }
    }

    /* Initialize context */
    rootstream_ctx_t ctx;
    if (rootstream_init(&ctx) < 0) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    int ret = 0;

    if (is_host) {
        /* Host mode */
        printf("🔍 Detecting displays...\n");
        
        if (rootstream_select_display(&ctx, display_idx) < 0) {
            ret = 1;
            goto cleanup;
        }

        printf("✓ Selected: %s (%dx%d @ %d Hz)\n\n",
               ctx.display.name, ctx.display.width, 
               ctx.display.height, ctx.display.refresh_rate);

        /* Initialize capture */
        if (rootstream_capture_init(&ctx) < 0) {
            fprintf(stderr, "Capture init failed: %s\n", rootstream_get_error());
            ret = 1;
            goto cleanup;
        }

        /* Initialize encoder */
        if (rootstream_encoder_init(&ctx, ENCODER_VAAPI) < 0) {
            fprintf(stderr, "Encoder init failed: %s\n", rootstream_get_error());
            ret = 1;
            goto cleanup;
        }

        /* Initialize network */
        if (rootstream_net_init(&ctx, "0.0.0.0", port) < 0) {
            fprintf(stderr, "Network init failed: %s\n", rootstream_get_error());
            ret = 1;
            goto cleanup;
        }

        /* Initialize input */
        if (rootstream_input_init(&ctx) < 0) {
            fprintf(stderr, "Input init failed: %s\n", rootstream_get_error());
            ret = 1;
            goto cleanup;
        }

        /* Run host */
        ret = rootstream_run_host(&ctx);

    } else {
        /* Client mode */
        if (!host_addr) {
            fprintf(stderr, "Host address required for client mode\n");
            ret = 1;
            goto cleanup;
        }

        ret = rootstream_run_client(&ctx, host_addr);
    }

cleanup:
    rootstream_print_stats(&ctx);
    rootstream_cleanup(&ctx);
    
    printf("\n");
    return ret;
}
