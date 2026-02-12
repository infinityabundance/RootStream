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
#include <signal.h>
#include <errno.h>

#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
  #define usleep(us) Sleep((us) / 1000)
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <fcntl.h>
#endif

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
#ifdef _WIN32
    /* Windows uses Windows Services, not UNIX daemons */
    /* For now, just run in foreground on Windows */
    return 0;
#else
    /* Check if already running under systemd */
    if (getenv("INVOCATION_ID")) {
        /* Running under systemd, don't daemonize */
        return 0;
    }

    /* First fork */
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "ERROR: service_daemonize fork(1) failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    if (pid > 0) {
        /* Parent exits */
        exit(0);
    }

    /* Child continues */

    /* Create new session */
    if (setsid() < 0) {
        fprintf(stderr, "ERROR: service_daemonize setsid() failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    /* Ignore SIGHUP */
    signal(SIGHUP, SIG_IGN);

    /* Second fork */
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "ERROR: service_daemonize fork(2) failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    if (pid > 0) {
        /* First child exits */
        exit(0);
    }

    /* Second child (daemon) continues */

    /* Change working directory */
    if (chdir("/") < 0) {
        fprintf(stderr, "ERROR: service_daemonize chdir(\"/\") failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
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
    } else {
        fprintf(stderr, "WARNING: service_daemonize failed to open /dev/null\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
    }

    return 0;
#endif
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

    /* Initialize capture with fallback chain */
    const capture_backend_t backends[] = {
        {
            .name = "DRM/KMS",
            .init_fn = rootstream_capture_init_drm,
            .capture_fn = rootstream_capture_frame_drm,
            .cleanup_fn = rootstream_capture_cleanup_drm,
        },
        {
            .name = "X11 SHM",
            .init_fn = rootstream_capture_init_x11,
            .capture_fn = rootstream_capture_frame_x11,
            .cleanup_fn = rootstream_capture_cleanup_x11,
        },
        {
            .name = "Dummy Pattern",
            .init_fn = rootstream_capture_init_dummy,
            .capture_fn = rootstream_capture_frame_dummy,
            .cleanup_fn = rootstream_capture_cleanup_dummy,
        },
        {NULL, NULL, NULL, NULL}  /* Sentinel */
    };

    int backend_idx = 0;
    while (backends[backend_idx].name) {
        printf("INFO: Attempting capture backend: %s\n", backends[backend_idx].name);
        if (backends[backend_idx].init_fn(ctx) == 0) {
            printf("✓ Capture backend '%s' initialized successfully\n", backends[backend_idx].name);
            ctx->capture_backend = &backends[backend_idx];
            break;
        } else {
            printf("WARNING: Capture backend '%s' failed, trying next...\n", backends[backend_idx].name);
            backend_idx++;
        }
    }

    if (!ctx->capture_backend) {
        fprintf(stderr, "ERROR: All capture backends failed!\n");
        return -1;
    }

    /* Auto-detect encoder: Try NVENC first (if available), fall back to VA-API */
    extern bool rootstream_encoder_nvenc_available(void);

    /* Use codec from settings (default H.264 for Phase 6 compatibility) */
    codec_type_t codec = (strcmp(ctx->settings.video_codec, "h265") == 0 ||
                         strcmp(ctx->settings.video_codec, "hevc") == 0) ?
                         CODEC_H265 : CODEC_H264;

    if (rootstream_encoder_nvenc_available()) {
        printf("INFO: NVENC detected, trying NVIDIA encoder...\n");
        if (rootstream_encoder_init(ctx, ENCODER_NVENC, codec) == 0) {
            printf("✓ Using NVENC encoder\n");
        } else {
            printf("WARNING: NVENC init failed, falling back to VA-API\n");
            if (rootstream_encoder_init(ctx, ENCODER_VAAPI, codec) < 0) {
                fprintf(stderr, "ERROR: Both NVENC and VA-API failed\n");
                return -1;
            }
        }
    } else {
        if (rootstream_encoder_init(ctx, ENCODER_VAAPI, codec) < 0) {
            fprintf(stderr, "ERROR: Encoder init failed\n");
            return -1;
        }
    }

    if (rootstream_input_init(ctx) < 0) {
        fprintf(stderr, "WARNING: Input init failed (continuing without input)\n");
    }

    /* Initialize audio capture and Opus encoder */
    if (ctx->settings.audio_enabled) {
        if (audio_capture_init(ctx) < 0) {
            fprintf(stderr, "WARNING: Audio capture init failed (continuing without audio)\n");
        } else if (rootstream_opus_encoder_init(ctx) < 0) {
            fprintf(stderr, "WARNING: Opus encoder init failed (continuing without audio)\n");
            audio_capture_cleanup(ctx);
        }
    } else {
        printf("INFO: Audio disabled in settings\n");
    }

    /* Announce service */
    if (ctx->discovery.running) {
        if (discovery_announce(ctx) < 0) {
            fprintf(stderr, "ERROR: Discovery announce failed (service startup)\n");
            fprintf(stderr, "DETAILS: Service will continue without mDNS advertisement\n");
        }
    } else {
        printf("INFO: Discovery disabled (no service announcement)\n");
    }

    /* Allocate encoding buffer */
    size_t enc_buf_size = ctx->encoder.max_output_size;
    if (enc_buf_size == 0) {
        enc_buf_size = (size_t)ctx->display.width * ctx->display.height;
    }
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
        if (ctx->capture_backend->capture_fn(ctx, &ctx->current_frame) < 0) {
            fprintf(stderr, "ERROR: Capture failed (display=%s)\n", ctx->display.name);
            fprintf(stderr, "DETAILS: %s\n", rootstream_get_error());
            usleep(16000);
            continue;
        }
        uint64_t capture_end_us = get_timestamp_us();

        /* Encode frame */
        size_t enc_size = 0;
        bool is_keyframe = false;
        uint64_t encode_start_us = get_timestamp_us();
        if (rootstream_encode_frame_ex(ctx, &ctx->current_frame,
                                      enc_buf, &enc_size, &is_keyframe) < 0) {
            fprintf(stderr, "ERROR: Encode failed (frame=%lu)\n", ctx->frames_captured);
            continue;
        }
        uint64_t encode_end_us = get_timestamp_us();

        /* Write to recording file if active */
        if (ctx->recording.active) {
            /* Use real keyframe detection from encoder */
            if (recording_write_frame(ctx, enc_buf, enc_size, is_keyframe) < 0) {
                fprintf(stderr, "WARNING: Failed to write frame to recording\n");
            }
        }

        /* Capture and encode audio */
        int16_t audio_samples[rootstream_opus_get_frame_size() * rootstream_opus_get_channels()];
        uint8_t audio_buf[4000];  /* Max Opus packet size */
        size_t audio_size = 0;
        size_t num_samples = 0;

        if (ctx->settings.audio_enabled &&
            audio_capture_frame(ctx, audio_samples, &num_samples) == 0) {
            if (rootstream_opus_encode(ctx, audio_samples, audio_buf, &audio_size) < 0) {
                /* Audio encode failed, continue with video only */
                audio_size = 0;
            }
        }

        /* Send to all connected peers */
        uint64_t send_start_us = get_timestamp_us();
        for (int i = 0; i < ctx->num_peers; i++) {
            peer_t *peer = &ctx->peers[i];
            if (peer->state == PEER_CONNECTED && peer->is_streaming) {
                /* Send video */
                if (enc_size > 0 &&
                    rootstream_net_send_video(ctx, peer, enc_buf, enc_size,
                                              ctx->current_frame.timestamp) < 0) {
                    fprintf(stderr, "ERROR: Video send failed (peer=%s)\n", peer->hostname);
                }

                /* Send audio if available */
                if (audio_size > 0) {
                    audio_packet_header_t header = {
                        .timestamp_us = get_timestamp_us(),
                        .sample_rate = 48000,
                        .channels = 2,
                        .samples = (uint16_t)num_samples
                    };

                    uint8_t payload[sizeof(audio_packet_header_t) + 4000];
                    memcpy(payload, &header, sizeof(header));
                    memcpy(payload + sizeof(header), audio_buf, audio_size);

                    if (rootstream_net_send_encrypted(ctx, peer, PKT_AUDIO,
                                                      payload, sizeof(header) + audio_size) < 0) {
                        fprintf(stderr, "ERROR: Audio send failed (peer=%s)\n", peer->hostname);
                    }
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
        rootstream_net_tick(ctx);

        /* Rate limiting */
        uint32_t refresh_rate = ctx->display.refresh_rate ? ctx->display.refresh_rate : 60;
        usleep(1000000 / refresh_rate);
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

    /* Initialize decoder */
    if (rootstream_decoder_init(ctx) < 0) {
        fprintf(stderr, "ERROR: Decoder initialization failed\n");
        return -1;
    }

    /* Initialize display (SDL2 window) */
    if (display_init(ctx, "RootStream Client", 1920, 1080) < 0) {
        fprintf(stderr, "ERROR: Display initialization failed\n");
        rootstream_decoder_cleanup(ctx);
        return -1;
    }

    /* Initialize audio playback and Opus decoder */
    if (ctx->settings.audio_enabled) {
        if (audio_playback_init(ctx) < 0) {
            fprintf(stderr, "WARNING: Audio playback init failed (continuing without audio)\n");
        } else if (rootstream_opus_decoder_init(ctx) < 0) {
            fprintf(stderr, "WARNING: Opus decoder init failed (continuing without audio)\n");
            audio_playback_cleanup(ctx);
        }
    } else {
        printf("INFO: Audio disabled in settings\n");
    }

    printf("✓ Client initialized - ready to receive video and audio\n");
    if (ctx->latency.enabled) {
        printf("INFO: Client latency logging enabled (interval=%lums, samples=%zu)\n",
               ctx->latency.report_interval_ms, ctx->latency.capacity);
    }

    /* Allocate decode buffer */
    frame_buffer_t decoded_frame = {0};

    /* Main receive loop */
    while (service_running && ctx->running) {
        uint64_t loop_start_us = get_timestamp_us();

        /* Poll SDL events (window close, keyboard, mouse) */
        if (display_poll_events(ctx) != 0) {
            printf("INFO: User requested quit\n");
            break;
        }

        /* Receive packets (16ms timeout for ~60fps responsiveness) */
        uint64_t recv_start_us = get_timestamp_us();
        rootstream_net_recv(ctx, 16);
        uint64_t recv_end_us = get_timestamp_us();
        rootstream_net_tick(ctx);

        /* Check if we received a video frame */
        if (ctx->current_frame.data && ctx->current_frame.size > 0) {
            /* Decode frame */
            uint64_t decode_start_us = get_timestamp_us();
            if (rootstream_decode_frame(ctx, ctx->current_frame.data,
                                       ctx->current_frame.size,
                                       &decoded_frame) == 0) {
                uint64_t decode_end_us = get_timestamp_us();

                /* Present to display */
                uint64_t present_start_us = get_timestamp_us();
                display_present_frame(ctx, &decoded_frame);
                uint64_t present_end_us = get_timestamp_us();

                /* Record latency stats */
                if (ctx->latency.enabled) {
                    latency_sample_t sample = {
                        .capture_us = recv_end_us - recv_start_us,  /* Network receive time */
                        .encode_us = decode_end_us - decode_start_us, /* Decode time */
                        .send_us = present_end_us - present_start_us, /* Present time */
                        .total_us = present_end_us - loop_start_us
                    };
                    latency_record(&ctx->latency, &sample);
                }
            } else {
                fprintf(stderr, "WARNING: Frame decode failed\n");
            }

            /* Clear frame for next iteration */
            ctx->current_frame.size = 0;
        }
    }

    /* Cleanup */
    if (decoded_frame.data) {
        free(decoded_frame.data);
    }

    if (ctx->settings.audio_enabled) {
        audio_playback_cleanup(ctx);
        rootstream_opus_cleanup(ctx);
    }
    display_cleanup(ctx);
    rootstream_decoder_cleanup(ctx);

    printf("✓ Client shutdown complete\n");

    return 0;
}
