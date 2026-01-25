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

    /* Initialize components */
    if (rootstream_capture_init(ctx) < 0) {
        return -1;
    }

    if (rootstream_encoder_init(ctx, ENCODER_VAAPI) < 0) {
        return -1;
    }

    if (rootstream_input_init(ctx) < 0) {
        return -1;
    }

    /* Announce service */
    discovery_announce(ctx);

    /* Allocate encoding buffer */
    size_t enc_buf_size = ctx->display.width * ctx->display.height;
    uint8_t *enc_buf = malloc(enc_buf_size);
    if (!enc_buf) {
        return -1;
    }

    /* Main loop */
    while (service_running && ctx->running) {
        /* Capture frame */
        if (rootstream_capture_frame(ctx, &ctx->current_frame) < 0) {
            usleep(16000);
            continue;
        }

        /* Encode frame */
        size_t enc_size = 0;
        if (rootstream_encode_frame(ctx, &ctx->current_frame,
                                   enc_buf, &enc_size) < 0) {
            continue;
        }

        /* Send to all connected peers */
        for (int i = 0; i < ctx->num_peers; i++) {
            peer_t *peer = &ctx->peers[i];
            if (peer->state == PEER_CONNECTED && peer->is_streaming) {
                rootstream_net_send_encrypted(ctx, peer, PKT_VIDEO,
                                             enc_buf, enc_size);
            }
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
