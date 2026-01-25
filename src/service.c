/*
 * service.c - Background service/daemon
 * 
 * Runs RootStream as a systemd user service:
 * - No GUI required
 * - Starts on login
 * - Auto-restarts on failure
 * - Logs to journald
 * 
 * Modes:
 * - Host: Always ready to stream (auto-accept from known peers)
 * - Client: Automatically connect to known host
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

static volatile bool service_running = true;

static void service_signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        service_running = false;
    }
}

/*
 * Daemonize process (if not running under systemd)
 * 
 * Steps:
 * 1. Fork and exit parent (detach from terminal)
 * 2. Create new session (become session leader)
 * 3. Fork again (prevent acquiring controlling terminal)
 * 4. Change working directory to /
 * 5. Close standard file descriptors
 * 6. Redirect to /dev/null
 */
int service_daemonize(void) {
    /* Check if already running under systemd */
    if (getenv("INVOCATION_ID")) {
        /* Running under systemd, don't daemonize */
        return 0;
    }

    /* First fork */
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "ERROR: Fork failed: %s\n", strerror(errno));
        return -1;
    }

    if (pid > 0) {
        /* Parent exits */
        exit(0);
    }

    /* Child continues */
    
    /* Create new session */
    if (setsid() < 0) {
        return -1;
    }

    /* Ignore SIGHUP */
    signal(SIGHUP, SIG_IGN);

    /* Second fork */
    pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid > 0) {
        /* First child exits */
        exit(0);
    }

    /* Second child (daemon) continues */

    /* Change working directory */
    if (chdir("/") < 0) {
        return -1;
    }

    /* Close file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Redirect to /dev/null */
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        if (null_fd > 2) {
            close(null_fd);
        }
    }

    return 0;
}

/*
 * Run as host service
 * 
 * Continuously streams to any connected peer
 */
int service_run_host(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    /* Install signal handlers */
    signal(SIGTERM, service_signal_handler);
    signal(SIGINT, service_signal_handler);

    printf("INFO: Starting RootStream host service\n");
    if (ctx->latency.enabled) {
        printf("INFO: Latency logging enabled (interval=%lums, samples=%zu)\n",
               ctx->latency.report_interval_ms, ctx->latency.capacity);
    }

    /* Initialize components */
    if (rootstream_capture_init(ctx) < 0) {
        fprintf(stderr, "ERROR: Capture init failed\n");
        fprintf(stderr, "DETAILS: %s\n", rootstream_get_error());
        return -1;
    }

    if (rootstream_encoder_init(ctx, ENCODER_VAAPI) < 0) {
        fprintf(stderr, "ERROR: Encoder init failed\n");
        return -1;
    }

    if (rootstream_input_init(ctx) < 0) {
        fprintf(stderr, "ERROR: Input init failed\n");
        return -1;
    }

    /* Announce service */
    if (ctx->discovery.running) {
        if (discovery_announce(ctx) < 0) {
            fprintf(stderr, "ERROR: Discovery announce failed (service startup)\n");
        }
    } else {
        printf("INFO: Discovery disabled (no service announcement)\n");
    }

    /* Allocate encoding buffer */
    size_t enc_buf_size = ctx->display.width * ctx->display.height;
    uint8_t *enc_buf = malloc(enc_buf_size);
    if (!enc_buf) {
        fprintf(stderr, "ERROR: Failed to allocate encode buffer (%zu bytes)\n",
                enc_buf_size);
        return -1;
    }

    /* Main loop */
    while (service_running && ctx->running) {
        uint64_t loop_start_us = get_timestamp_us();

        /* Capture frame */
        if (rootstream_capture_frame(ctx, &ctx->current_frame) < 0) {
            fprintf(stderr, "ERROR: Capture failed (display=%s)\n", ctx->display.name);
            fprintf(stderr, "DETAILS: %s\n", rootstream_get_error());
            usleep(16000);
            continue;
        }
        uint64_t capture_end_us = get_timestamp_us();

        /* Encode frame */
        size_t enc_size = 0;
        uint64_t encode_start_us = get_timestamp_us();
        if (rootstream_encode_frame(ctx, &ctx->current_frame,
                                   enc_buf, &enc_size) < 0) {
            fprintf(stderr, "ERROR: Encode failed (frame=%lu)\n", ctx->frames_captured);
            continue;
        }
        uint64_t encode_end_us = get_timestamp_us();

        /* Send to all connected peers */
        uint64_t send_start_us = get_timestamp_us();
        for (int i = 0; i < ctx->num_peers; i++) {
            peer_t *peer = &ctx->peers[i];
            if (peer->state == PEER_CONNECTED && peer->is_streaming) {
                if (rootstream_net_send_encrypted(ctx, peer, PKT_VIDEO,
                                                  enc_buf, enc_size) < 0) {
                    fprintf(stderr, "ERROR: Send failed (peer=%s)\n", peer->hostname);
                }
            }
        }
        uint64_t send_end_us = get_timestamp_us();

        if (ctx->latency.enabled) {
            latency_sample_t sample = {
                .capture_us = capture_end_us - loop_start_us,
                .encode_us = encode_end_us - encode_start_us,
                .send_us = send_end_us - send_start_us,
                .total_us = send_end_us - loop_start_us
            };
            latency_record(&ctx->latency, &sample);
        }

        /* Process incoming packets */
        rootstream_net_recv(ctx, 1);

        /* Rate limiting */
        usleep(1000000 / ctx->display.refresh_rate);
    }

    free(enc_buf);
    return 0;
}

/*
 * Run as client service
 * 
 * Automatically connects to configured host
 */
int service_run_client(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    /* Install signal handlers */
    signal(SIGTERM, service_signal_handler);
    signal(SIGINT, service_signal_handler);

    printf("INFO: Starting RootStream client service\n");

    /* TODO: Load saved host from config */
    /* TODO: Implement decoder and display */

    /* Main loop */
    while (service_running && ctx->running) {
        /* Receive and decode */
        rootstream_net_recv(ctx, 100);

        /* Send input events */
        /* TODO */

        usleep(1000);
    }

    return 0;
}
