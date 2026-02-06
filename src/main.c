/*
 * main.c - RootStream main entry point
 * 
 * Usage modes:
 *   rootstream                    # Start tray app (GUI mode)
 *   rootstream --service          # Run as background service
 *   rootstream --qr               # Show QR code and exit
 *   rootstream connect <code>     # Connect to peer
 *   rootstream host               # Host mode (for testing)
 *   
 * The tray app is the default and recommended way to use RootStream.
 * Service mode is for headless systems or systemd integration.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

static volatile bool keep_running = true;

/*
 * Signal handler for graceful shutdown
 */
static void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        printf("\nINFO: Shutting down gracefully...\n");
        keep_running = false;
    }
}

/*
 * Print usage information
 */
static void print_usage(const char *progname) {
    printf("Usage: %s [OPTIONS] [COMMAND]\n", progname);
    printf("\n");
    printf("RootStream - Secure P2P Game Streaming\n");
    printf("Version %s\n", ROOTSTREAM_VERSION);
    printf("\n");
    printf("Commands:\n");
    printf("  (none)              Start system tray application (default)\n");
    printf("  connect <code>      Connect to peer using RootStream code\n");
    printf("  host                Run in host mode (streaming server)\n");
    printf("  --qr                Display your QR code and exit\n");
    printf("  --list-displays     List available displays and exit\n");
    printf("  --service           Run as background service (no GUI)\n");
    printf("  --version           Show version and exit\n");
    printf("  --help              Show this help\n");
    printf("\n");
    printf("Options:\n");
    printf("  --port PORT         UDP port to use (default: 9876)\n");
    printf("  --display N         Select display index (default: 0)\n");
    printf("  --bitrate KBPS      Video bitrate in kbps (default: 10000)\n");
    printf("  --record FILE       Record stream to file (host mode only)\n");
    printf("  --no-discovery      Disable mDNS auto-discovery\n");
    printf("  --latency-log       Enable latency percentile logging\n");
    printf("  --latency-interval MS  Latency log interval in ms (default: 1000)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                                    # Start tray app\n", progname);
    printf("  %s --qr                               # Show your code\n", progname);
    printf("  %s connect kXx7Y...@gaming-pc         # Connect to peer\n", progname);
    printf("  %s host --display 1 --bitrate 15000   # Host on 2nd display\n", progname);
    printf("\n");
    printf("First time setup:\n");
    printf("  1. Run 'rootstream --qr' to get your code\n");
    printf("  2. Share the QR code or text with another device\n");
    printf("  3. On the other device, run 'rootstream connect <your_code>'\n");
    printf("  4. Devices will auto-connect when on same network\n");
    printf("\n");
    printf("Configuration: ~/.config/rootstream/\n");
    printf("Documentation: https://github.com/yourusername/rootstream\n");
}

/*
 * Print version and build info
 */
static void print_version(void) {
    printf("RootStream %s\n", ROOTSTREAM_VERSION);
    printf("Built: %s %s\n", __DATE__, __TIME__);
    printf("\n");
    printf("Features:\n");
#ifdef HAVE_AVAHI
    printf("  ✓ mDNS Discovery (Avahi)\n");
#else
    printf("  ✗ mDNS Discovery (disabled)\n");
#endif
    printf("  ✓ Ed25519 Encryption\n");
    printf("  ✓ VA-API Encoding\n");
    printf("  ✓ DRM/KMS Capture\n");
    printf("  ✓ QR Code Sharing\n");
    printf("  ✓ GTK3 System Tray\n");
    printf("\n");
}

/*
 * Run in tray mode (default)
 */
static int run_tray_mode(rootstream_ctx_t *ctx, int argc, char **argv, bool no_discovery) {
    printf("INFO: Starting system tray application\n");
    printf("INFO: Right-click the tray icon for options\n");
    printf("INFO: Left-click to show your QR code\n");
    printf("\n");

    /* Initialize network */
    if (rootstream_net_init(ctx, ctx->port) < 0) {
        fprintf(stderr, "ERROR: Network initialization failed\n");
        return -1;
    }

    /* Initialize discovery */
    if (no_discovery) {
        printf("INFO: mDNS discovery disabled by --no-discovery\n");
    } else if (discovery_init(ctx) == 0) {
        if (discovery_announce(ctx) < 0) {
            fprintf(stderr, "ERROR: mDNS announce failed (tray mode)\n");
        }
        if (discovery_browse(ctx) < 0) {
            fprintf(stderr, "ERROR: mDNS browse failed (tray mode)\n");
        }
    }

    /* Initialize tray UI */
    if (tray_init(ctx, argc, argv) < 0) {
        fprintf(stderr, "ERROR: Tray initialization failed\n");
        fprintf(stderr, "FIX: Ensure system tray is available\n");
        return -1;
    }

    /* Run GTK main loop (blocks until quit) */
    tray_run(ctx);

    return 0;
}

/*
 * Run in host mode (streaming server)
 */
static int run_host_mode(rootstream_ctx_t *ctx, int display_idx, bool no_discovery, const char *record_file) {
    printf("INFO: Starting host mode\n");
    printf("INFO: Press Ctrl+C to stop\n");
    printf("\n");

    /* Detect and select display */
    display_info_t displays[MAX_DISPLAYS];
    int num_displays = rootstream_detect_displays(displays, MAX_DISPLAYS);
    
    if (num_displays < 0) {
        fprintf(stderr, "ERROR: %s\n", rootstream_get_error());
        return -1;
    }

    printf("INFO: Found %d display(s)\n", num_displays);
    for (int i = 0; i < num_displays; i++) {
        printf("  [%d] %s - %dx%d @ %d Hz\n", i,
               displays[i].name, displays[i].width,
               displays[i].height, displays[i].refresh_rate);
    }

    if (display_idx < 0 || display_idx >= num_displays) {
        fprintf(stderr, "ERROR: Display index %d out of range (0-%d)\n",
                display_idx, num_displays - 1);
        for (int i = 0; i < num_displays; i++) {
            if (displays[i].fd >= 0) {
                close(displays[i].fd);
            }
        }
        return -1;
    }

    ctx->display = displays[display_idx];
    for (int i = 0; i < num_displays; i++) {
        if (i != display_idx && displays[i].fd >= 0) {
            close(displays[i].fd);
        }
    }

    if (ctx->display.refresh_rate == 0) {
        ctx->display.refresh_rate = 60;
        printf("WARNING: Display refresh rate unknown, defaulting to 60 Hz\n");
    }

    printf("\n✓ Selected: %s (%dx%d @ %d Hz)\n\n",
           ctx->display.name, ctx->display.width,
           ctx->display.height, ctx->display.refresh_rate);
    printf("INFO: Target video bitrate: %u kbps\n", ctx->encoder.bitrate / 1000);

    /* Initialize components */
    if (rootstream_capture_init(ctx) < 0) {
        fprintf(stderr, "ERROR: Capture init failed\n");
        return -1;
    }

    /* Use H.264 by default (host mode doesn't use settings file) */
    if (rootstream_encoder_init(ctx, ENCODER_VAAPI, CODEC_H264) < 0) {
        fprintf(stderr, "ERROR: Encoder init failed\n");
        return -1;
    }

    if (rootstream_net_init(ctx, ctx->port) < 0) {
        fprintf(stderr, "ERROR: Network init failed\n");
        return -1;
    }

    if (rootstream_input_init(ctx) < 0) {
        fprintf(stderr, "ERROR: Input init failed\n");
        return -1;
    }

    /* Initialize discovery */
    if (no_discovery) {
        printf("INFO: mDNS discovery disabled by --no-discovery\n");
    } else if (discovery_init(ctx) == 0) {
        printf("INFO: Discovery initialized for host announcements\n");
    }

    /* Initialize recording if requested */
    if (record_file) {
        if (recording_init(ctx, record_file) < 0) {
            fprintf(stderr, "ERROR: Recording init failed\n");
            return -1;
        }
    }

    printf("✓ All systems ready\n");
    printf("→ Waiting for connections...\n\n");

    /* Run host service */
    int result = service_run_host(ctx);

    /* Cleanup recording */
    if (record_file && ctx->recording.active) {
        recording_cleanup(ctx);
    }

    return result;
}

/*
 * Connect to peer by RootStream code
 */
static int run_connect_mode(rootstream_ctx_t *ctx, const char *peer_code) {
    printf("INFO: Connecting to peer: %s\n", peer_code);

    /* Initialize network */
    if (rootstream_net_init(ctx, ctx->port) < 0) {
        return -1;
    }

    /* Connect to peer */
    if (rootstream_connect_to_peer(ctx, peer_code) < 0) {
        fprintf(stderr, "ERROR: Failed to connect to peer\n");
        return -1;
    }

    printf("✓ Connection initiated\n");
    printf("INFO: Waiting for handshake...\n");

    /* Run client service */
    return service_run_client(ctx);
}

/*
 * Main entry point
 */
int main(int argc, char **argv) {
    rootstream_ctx_t ctx;
    int ret = 0;

    /* Parse options */
    static struct option long_options[] = {
        {"help",        no_argument,       0, 'h'},
        {"version",     no_argument,       0, 'v'},
        {"qr",          no_argument,       0, 'q'},
        {"list-displays", no_argument,     0, 'L'},
        {"service",     no_argument,       0, 's'},
        {"port",        required_argument, 0, 'p'},
        {"display",     required_argument, 0, 'd'},
        {"bitrate",     required_argument, 0, 'b'},
        {"record",      required_argument, 0, 'r'},
        {"no-discovery",no_argument,       0, 'n'},
        {"latency-log", no_argument,       0, 'l'},
        {"latency-interval", required_argument, 0, 'i'},
        {0, 0, 0, 0}
    };

    bool show_qr = false;
    bool list_displays = false;
    bool service_mode = false;
    bool no_discovery = false;
    uint16_t port = 9876;
    int display_idx = 0;
    int bitrate = 10000;
    const char *record_file = NULL;
    bool latency_log = false;
    uint64_t latency_interval_ms = 1000;

    int opt;
    while ((opt = getopt_long(argc, argv, "hvqLsp:d:b:r:nli:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'q':
                show_qr = true;
                break;
            case 'L':
                list_displays = true;
                break;
            case 's':
                service_mode = true;
                break;
            case 'p':
                port = atoi(optarg);
                if (port == 0) {
                    fprintf(stderr, "ERROR: Invalid port: %s\n", optarg);
                    return 1;
                }
                break;
            case 'd':
                display_idx = atoi(optarg);
                break;
            case 'b':
                bitrate = atoi(optarg);
                if (bitrate < 1000) {
                    fprintf(stderr, "ERROR: Bitrate too low: %d\n", bitrate);
                    return 1;
                }
                break;
            case 'r':
                record_file = optarg;
                break;
            case 'n':
                no_discovery = true;
                break;
            case 'l':
                latency_log = true;
                break;
            case 'i':
                latency_interval_ms = (uint64_t)strtoul(optarg, NULL, 10);
                if (latency_interval_ms == 0) {
                    fprintf(stderr, "ERROR: Invalid latency interval: %s\n", optarg);
                    return 1;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);  /* Ignore broken pipe */

    /* Initialize context */
    if (rootstream_init(&ctx) < 0) {
        fprintf(stderr, "ERROR: Initialization failed\n");
        return 1;
    }

    if (latency_init(&ctx.latency, 240, latency_interval_ms, latency_log) < 0) {
        fprintf(stderr, "WARNING: Latency logging disabled due to init failure\n");
    }

    ctx.port = port;
    ctx.encoder.bitrate = (uint32_t)bitrate * 1000;

    /* Handle --list-displays flag */
    if (list_displays) {
        display_info_t displays[MAX_DISPLAYS];
        int num_displays = rootstream_detect_displays(displays, MAX_DISPLAYS);

        if (num_displays < 0) {
            fprintf(stderr, "ERROR: %s\n", rootstream_get_error());
            rootstream_cleanup(&ctx);
            return 1;
        }

        printf("Available displays:\n\n");
        for (int i = 0; i < num_displays; i++) {
            printf("  [%d] %s\n", i, displays[i].name);
            printf("      Resolution: %dx%d @ %d Hz\n",
                   displays[i].width, displays[i].height,
                   displays[i].refresh_rate);
            printf("\n");

            /* Close FDs */
            if (displays[i].fd >= 0) {
                close(displays[i].fd);
            }
        }

        printf("Use --display N to select a specific display\n");
        printf("Example: rootstream host --display 1\n");

        rootstream_cleanup(&ctx);
        return 0;
    }

    /* Handle --qr flag */
    if (show_qr) {
        printf("Scan this QR code to connect:\n");
        qrcode_print_terminal(ctx.keypair.rootstream_code);

        /* Also save as PNG */
        char qr_path[256];
        snprintf(qr_path, sizeof(qr_path), "%s/rootstream-qr.png",
                config_get_dir());
        if (qrcode_generate(ctx.keypair.rootstream_code, qr_path) == 0) {
            printf("QR code saved to: %s\n", qr_path);
        }

        rootstream_cleanup(&ctx);
        return 0;
    }

    /* Parse command */
    const char *command = (optind < argc) ? argv[optind] : NULL;

    if (service_mode) {
        ctx.is_service = true;
        if (service_daemonize() < 0) {
            fprintf(stderr, "ERROR: Failed to enter service mode\n");
            ret = 1;
            goto cleanup;
        }
    }

    if (command == NULL) {
        if (service_mode) {
            /* Default service behavior: host mode without GUI */
            ret = run_host_mode(&ctx, display_idx, no_discovery, record_file);
        } else {
            /* Default: tray mode */
            ret = run_tray_mode(&ctx, argc, argv, no_discovery);
        }
    } else if (strcmp(command, "host") == 0) {
        /* Host mode */
        ret = run_host_mode(&ctx, display_idx, no_discovery, record_file);
    } else if (strcmp(command, "connect") == 0) {
        /* Connect mode */
        if (optind + 1 >= argc) {
            fprintf(stderr, "ERROR: Missing RootStream code\n");
            fprintf(stderr, "Usage: %s connect <rootstream_code>\n", argv[0]);
            ret = 1;
        } else {
            ret = run_connect_mode(&ctx, argv[optind + 1]);
        }
    } else {
        fprintf(stderr, "ERROR: Unknown command: %s\n", command);
        fprintf(stderr, "Run '%s --help' for usage\n", argv[0]);
        ret = 1;
    }

cleanup:
    /* Print statistics and cleanup */
    rootstream_print_stats(&ctx);
    rootstream_cleanup(&ctx);

    return ret;
}
