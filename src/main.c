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
#include "ai_logging.h"
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
    printf("  --preset PRESET     Recording quality preset (fast/balanced/quality/archival)\n");
    printf("  --no-discovery      Disable mDNS auto-discovery\n");
    printf("  --latency-log       Enable latency percentile logging\n");
    printf("  --latency-interval MS  Latency log interval in ms (default: 1000)\n");
    printf("\n");
    printf("Manual Peer Entry (PHASE 5):\n");
    printf("  --peer-add IP:PORT  Manually add peer by IP address and port\n");
    printf("  --peer-code CODE    Connect using RootStream code from history\n");
    printf("  --peer-list         List saved peer history\n");
    printf("\n");
    printf("Backend Selection (PHASE 6):\n");
    printf("  --gui MODE          Select GUI backend (gtk/tui/cli)\n");
    printf("  --input MODE        Select input backend (uinput/xdotool/logging)\n");
    printf("  --diagnostics       Show system diagnostics and exit\n");
    printf("\n");
    printf("AI Coding Logging (PHASE 9):\n");
    printf("  --ai-coding-logs[=FILE]  Enable AI-assisted development logging\n");
    printf("                           (also activated by AI_COPILOT_MODE=1)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                                    # Start tray app\n", progname);
    printf("  %s --qr                               # Show your code\n", progname);
    printf("  %s connect kXx7Y...@gaming-pc         # Connect to peer\n", progname);
    printf("  %s host --display 1 --bitrate 15000   # Host on 2nd display\n", progname);
    printf("  %s host --record game.mp4             # Record to file (balanced preset)\n", progname);
    printf("  %s host --record game.mp4 --preset fast  # Fast recording preset\n", progname);
    printf("  %s --peer-add 192.168.1.100:9876      # Manually add peer\n", progname);
    printf("  %s --peer-list                        # Show saved peers\n", progname);
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

    /* Initialize tray UI with fallback (PHASE 6) */
    printf("INFO: Initializing GUI backend...\n");
    
    int gui_backend = -1;  /* -1=uninitialized, 0=GTK, 1=TUI, 2=CLI */
    
    /* Check for user override */
    if (ctx->backend_prefs.gui_override) {
        if (strcmp(ctx->backend_prefs.gui_override, "gtk") == 0) {
            printf("INFO: User requested GTK backend\n");
            if (tray_init(ctx, argc, argv) == 0) {
                gui_backend = 0;
                ctx->active_backend.gui_name = "GTK Tray";
            } else {
                fprintf(stderr, "ERROR: GTK backend requested but failed\n");
                return -1;
            }
        } else if (strcmp(ctx->backend_prefs.gui_override, "tui") == 0) {
            printf("INFO: User requested TUI backend\n");
            if (tray_init_tui(ctx, argc, argv) == 0) {
                gui_backend = 1;
                ctx->active_backend.gui_name = "Terminal UI";
            } else {
                fprintf(stderr, "ERROR: TUI backend requested but failed\n");
                return -1;
            }
        } else if (strcmp(ctx->backend_prefs.gui_override, "cli") == 0) {
            printf("INFO: User requested CLI backend\n");
            if (tray_init_cli(ctx, argc, argv) == 0) {
                gui_backend = 2;
                ctx->active_backend.gui_name = "CLI-only";
            } else {
                fprintf(stderr, "ERROR: CLI backend requested but failed\n");
                return -1;
            }
        } else {
            fprintf(stderr, "ERROR: Unknown GUI backend '%s'\n", ctx->backend_prefs.gui_override);
            return -1;
        }
    } else {
        /* Auto-detect with fallback chain */
        /* Try GTK first (primary) */
        if (tray_init(ctx, argc, argv) == 0) {
            printf("✓ GUI backend 'GTK Tray' initialized\n");
            ctx->active_backend.gui_name = "GTK Tray";
            gui_backend = 0;
        } else {
            /* Try Terminal UI fallback */
            printf("INFO: GTK unavailable, trying Terminal UI...\n");
            if (tray_init_tui(ctx, argc, argv) == 0) {
                ctx->active_backend.gui_name = "Terminal UI";
                gui_backend = 1;
            } else {
                /* Fall back to CLI-only mode */
                printf("INFO: Terminal UI unavailable, using CLI-only mode...\n");
                if (tray_init_cli(ctx, argc, argv) == 0) {
                    ctx->active_backend.gui_name = "CLI-only";
                    gui_backend = 2;
                } else {
                    fprintf(stderr, "ERROR: All GUI backends failed\n");
                    return -1;
                }
            }
        }
    }

    /* Run the selected GUI backend (blocks until quit) */
    if (gui_backend == 0) {
        tray_run(ctx);
    } else if (gui_backend == 1) {
        /* For TUI, we need a simple event loop */
        while (ctx->running) {
            tray_update_status_tui(ctx, ctx->tray.status);
            tray_run_tui(ctx);
            usleep(100000);  /* 100ms */
        }
    } else {
        /* For CLI, just keep running until interrupted */
        printf("INFO: Running in CLI-only mode (Ctrl+C to exit)\n");
        while (ctx->running) {
            tray_run_cli(ctx);
            usleep(1000000);  /* 1 second */
        }
    }

    return 0;
}

/*
 * Run in host mode (streaming server)
 */
static int run_host_mode(rootstream_ctx_t *ctx, int display_idx, bool no_discovery, const char *record_file, const char *record_preset) {
    printf("INFO: Starting host mode\n");
    printf("INFO: Press Ctrl+C to stop\n");
    printf("\n");
    ctx->is_host = true;

    /* Detect and select display (DRM-based) */
    display_info_t displays[MAX_DISPLAYS];
    int num_displays = rootstream_detect_displays(displays, MAX_DISPLAYS);
    
    if (num_displays < 0) {
        printf("WARNING: DRM display detection failed: %s\n", rootstream_get_error());
        printf("INFO: Will attempt fallback capture backends in service_run_host()\n");
        num_displays = 0;  /* Continue with fallback backends */
    }

    if (num_displays > 0) {
        printf("INFO: Found %d DRM display(s)\n", num_displays);
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
    } else {
        /* No DRM displays detected - fallback backends will be used */
        printf("INFO: No DRM displays available, will use fallback capture backend\n");
        /* Initialize display info to defaults for fallback backends */
        ctx->display.fd = -1;
        ctx->display.width = 0;  /* Let backend set this */
        ctx->display.height = 0;
        ctx->display.refresh_rate = 60;
        snprintf(ctx->display.name, sizeof(ctx->display.name), "Fallback");
    }

    printf("\n✓ Selected: %s (%dx%d @ %d Hz)\n\n",
           ctx->display.name, ctx->display.width,
           ctx->display.height, ctx->display.refresh_rate);
    printf("INFO: Target video bitrate: %u kbps\n", ctx->encoder.bitrate / 1000);

    /* Capture and encoder initialization will be handled by service_run_host() with fallback logic */

    if (rootstream_net_init(ctx, ctx->port) < 0) {
        fprintf(stderr, "ERROR: Network init failed\n");
        return -1;
    }

    /* Input initialization will be handled by service_run_host() */

    /* Initialize discovery */
    if (no_discovery) {
        printf("INFO: mDNS discovery disabled by --no-discovery\n");
    } else if (discovery_init(ctx) == 0) {
        printf("INFO: Discovery initialized for host announcements\n");
    }

    /* Initialize recording if requested */
    if (record_file) {
        // Parse preset if provided (defaults to "balanced")
        if (!record_preset || strlen(record_preset) == 0) {
            record_preset = "balanced";
        }
        
        printf("INFO: Recording enabled\n");
        printf("  File: %s\n", record_file);
        printf("  Preset: %s\n", record_preset);
        
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
        {"preset",      required_argument, 0, 'P'},
        {"no-discovery",no_argument,       0, 'n'},
        {"latency-log", no_argument,       0, 'l'},
        {"latency-interval", required_argument, 0, 'i'},
        {"backend-verbose", no_argument,   0, 0},
        {"peer-add",    required_argument, 0, 0},
        {"peer-list",   no_argument,       0, 0},
        {"peer-code",   required_argument, 0, 0},
        {"gui",         required_argument, 0, 0},
        {"input",       required_argument, 0, 0},
        {"diagnostics", no_argument,       0, 0},
        {"ai-coding-logs", optional_argument, 0, 0},
        {0, 0, 0, 0}
    };

    bool show_qr = false;
    bool list_displays = false;
    bool service_mode = false;
    bool no_discovery = false;
    bool show_peer_list = false;
    bool show_diagnostics = false;
    bool enable_ai_logging = false;
    const char *ai_log_file = NULL;
    const char *peer_add = NULL;
    const char *peer_code = NULL;
    const char *gui_override = NULL;
    const char *input_override = NULL;
    uint16_t port = 9876;
    int display_idx = -1;
    int bitrate = 10000;
    const char *record_file = NULL;
    const char *record_preset = NULL;  // Recording preset
    bool latency_log = false;
    uint64_t latency_interval_ms = 1000;
    bool backend_verbose = false;

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "hvqLsp:d:b:r:P:nli:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 0:
                /* Long option without short equivalent */
                if (strcmp(long_options[option_index].name, "backend-verbose") == 0) {
                    backend_verbose = true;
                    printf("INFO: Backend selection verbose mode enabled\n");
                } else if (strcmp(long_options[option_index].name, "peer-add") == 0) {
                    peer_add = optarg;
                } else if (strcmp(long_options[option_index].name, "peer-list") == 0) {
                    show_peer_list = true;
                } else if (strcmp(long_options[option_index].name, "peer-code") == 0) {
                    peer_code = optarg;
                } else if (strcmp(long_options[option_index].name, "gui") == 0) {
                    gui_override = optarg;
                } else if (strcmp(long_options[option_index].name, "input") == 0) {
                    input_override = optarg;
                } else if (strcmp(long_options[option_index].name, "diagnostics") == 0) {
                    show_diagnostics = true;
                } else if (strcmp(long_options[option_index].name, "ai-coding-logs") == 0) {
                    enable_ai_logging = true;
                    ai_log_file = optarg;  /* May be NULL for stderr */
                }
                break;
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
            case 'P':
                record_preset = optarg;
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

    /* Initialize AI logging module (PHASE 9) */
    ai_logging_init(&ctx);
    if (enable_ai_logging) {
        ai_logging_set_enabled(&ctx, true);
        if (ai_log_file) {
            if (ai_logging_set_output(&ctx, ai_log_file) < 0) {
                fprintf(stderr, "WARNING: Failed to set AI log file, using stderr\n");
            }
        }
    }
    
    AI_LOG_CORE("startup: RootStream version=%s", ROOTSTREAM_VERSION);
    AI_LOG_CORE("startup: port=%d bitrate=%d service_mode=%d", port, bitrate, service_mode);

    /* Set backend verbose mode if requested */
    ctx.backend_prefs.verbose = backend_verbose;
    ctx.backend_prefs.gui_override = gui_override;
    ctx.backend_prefs.input_override = input_override;

    if (latency_init(&ctx.latency, 240, latency_interval_ms, latency_log) < 0) {
        fprintf(stderr, "WARNING: Latency logging disabled due to init failure\n");
    }

    ctx.port = port;
    ctx.encoder.bitrate = (uint32_t)bitrate * 1000;
    ctx.is_host = false;

    if (display_idx < 0) {
        display_idx = ctx.settings.display_index;
    }

    /* Handle --diagnostics flag */
    if (show_diagnostics) {
        diagnostics_print_report(&ctx);
        rootstream_cleanup(&ctx);
        return 0;
    }

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

        printf("\nYour RootStream code:\n");
        printf("  %s\n\n", ctx.keypair.rootstream_code);

        rootstream_cleanup(&ctx);
        return 0;
    }

    /* Handle --peer-list flag (PHASE 5) */
    if (show_peer_list) {
        discovery_list_peer_history(&ctx);
        rootstream_cleanup(&ctx);
        return 0;
    }

    /* Handle --peer-add flag (PHASE 5) */
    if (peer_add) {
        if (discovery_manual_add_peer(&ctx, peer_add) < 0) {
            fprintf(stderr, "ERROR: Failed to add peer\n");
            rootstream_cleanup(&ctx);
            return 1;
        }
        printf("INFO: Peer added successfully\n");
        rootstream_cleanup(&ctx);
        return 0;
    }

    /* Handle --peer-code flag (PHASE 5) */
    if (peer_code) {
        if (discovery_manual_add_peer(&ctx, peer_code) < 0) {
            fprintf(stderr, "ERROR: Failed to connect to peer\n");
            rootstream_cleanup(&ctx);
            return 1;
        }
        printf("INFO: Peer connection initiated\n");
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
            ret = run_host_mode(&ctx, display_idx, no_discovery, record_file, record_preset);
        } else {
            /* Default: tray mode */
            ret = run_tray_mode(&ctx, argc, argv, no_discovery);
        }
    } else if (strcmp(command, "host") == 0) {
        /* Host mode */
        ret = run_host_mode(&ctx, display_idx, no_discovery, record_file, record_preset);
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
    AI_LOG_CORE("shutdown: cleaning up");
    rootstream_print_stats(&ctx);
    ai_logging_shutdown(&ctx);
    rootstream_cleanup(&ctx);

    return ret;
}
