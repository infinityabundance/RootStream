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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/rootstream.h"
#include "../include/rootstream_client_session.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define usleep(us) Sleep((us) / 1000)
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static volatile bool service_running = true;

static void service_signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        service_running = false;
    }
}

/*
 * Check peer health and initiate reconnection if needed (PHASE 4)
 */
static void check_peer_health(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    uint64_t now = get_timestamp_ms();
    for (int i = 0; i < ctx->num_peers; i++) {
        peer_t *peer = &ctx->peers[i];

        /* Check for stale connections (30 second timeout) */
        if (peer->state == PEER_CONNECTED) {
            if (peer->last_received > 0 && now - peer->last_received > 30000) {
                printf("WARNING: Peer %s timeout (no packets in 30s)\n", peer->hostname);
                peer->state = PEER_DISCONNECTED;
                if (peer->reconnect_ctx) {
                    peer_try_reconnect(ctx, peer);
                }
            }
        }
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

/* Wrapper function for VA-API init (converts 2-param to 3-param signature) */
static int vaapi_init_wrapper(rootstream_ctx_t *ctx, codec_type_t codec) {
    return rootstream_encoder_init(ctx, ENCODER_VAAPI, codec);
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

    /* Initialize capture with fallback chain (static to persist beyond function scope) */
    static const capture_backend_t backends[] = {
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
        {NULL, NULL, NULL, NULL} /* Sentinel */
    };

    int backend_idx = 0;
    while (backends[backend_idx].name) {
        printf("INFO: Attempting capture backend: %s\n", backends[backend_idx].name);
        if (backends[backend_idx].init_fn(ctx) == 0) {
            printf("✓ Capture backend '%s' initialized successfully\n", backends[backend_idx].name);
            ctx->capture_backend = &backends[backend_idx];
            ctx->active_backend.capture_name = backends[backend_idx].name;
            break;
        } else {
            printf("WARNING: Capture backend '%s' failed, trying next...\n",
                   backends[backend_idx].name);
            backend_idx++;
        }
    }

    if (!ctx->capture_backend) {
        fprintf(stderr, "ERROR: All capture backends failed!\n");
        return -1;
    }

    /* Multi-tier encoder selection with automatic fallback
     * Priority order: NVENC → VA-API → FFmpeg → Raw
     * Each encoder is tried in sequence until one succeeds
     */

    /* Define encoder backends in priority order */
    const encoder_backend_t encoder_backends[] = {
        {
            .name = "NVENC (NVIDIA GPU)",
            .init_fn = rootstream_encoder_init_nvenc,
            .encode_fn = rootstream_encode_frame_nvenc,
            .encode_ex_fn = NULL,
            .cleanup_fn = rootstream_encoder_cleanup_nvenc,
            .is_available_fn = rootstream_encoder_nvenc_available,
        },
        {
            .name = "VA-API (Intel/AMD GPU)",
            .init_fn = vaapi_init_wrapper,
            .encode_fn = rootstream_encode_frame,
            .encode_ex_fn = rootstream_encode_frame_ex,
            .cleanup_fn = rootstream_encoder_cleanup,
            .is_available_fn = rootstream_encoder_vaapi_available,
        },
        {
            .name = "FFmpeg/x264 (Software)",
            .init_fn = rootstream_encoder_init_ffmpeg,
            .encode_fn = rootstream_encode_frame_ffmpeg,
            .encode_ex_fn = rootstream_encode_frame_ex_ffmpeg,
            .cleanup_fn = rootstream_encoder_cleanup_ffmpeg,
            .is_available_fn = rootstream_encoder_ffmpeg_available,
        },
        {
            .name = "Raw Pass-through (Debug)",
            .init_fn = rootstream_encoder_init_raw,
            .encode_fn = rootstream_encode_frame_raw,
            .encode_ex_fn = NULL,
            .cleanup_fn = rootstream_encoder_cleanup_raw,
            .is_available_fn = NULL, /* Always available */
        },
        {NULL} /* Sentinel */
    };

    /* Determine codec from settings */
    codec_type_t codec = (strcmp(ctx->settings.video_codec, "h265") == 0 ||
                          strcmp(ctx->settings.video_codec, "hevc") == 0)
                             ? CODEC_H265
                             : CODEC_H264;

    printf("INFO: Initializing video encoder (codec: %s)\n",
           codec == CODEC_H265 ? "H.265/HEVC" : "H.264/AVC");

    /* Try each backend in sequence */
    int encoder_idx = 0;
    bool encoder_initialized = false;

    while (encoder_backends[encoder_idx].name) {
        const encoder_backend_t *backend = &encoder_backends[encoder_idx];

        printf("INFO: Attempting encoder: %s\n", backend->name);

        /* Check if backend is available */
        if (backend->is_available_fn && !backend->is_available_fn()) {
            printf("  → Not available on this system\n");
            encoder_idx++;
            continue;
        }

        /* Try to initialize */
        int init_result = backend->init_fn(ctx, codec);

        if (init_result == 0) {
            printf("✓ Encoder backend '%s' initialized successfully\n", backend->name);
            ctx->encoder_backend = backend;
            ctx->active_backend.encoder_name = backend->name;
            encoder_initialized = true;

            /* Warn if using fallback (software or raw) */
            if (encoder_idx >= 2) {
                printf("⚠ WARNING: Using %s\n", encoder_idx == 2 ? "software encoder (slow)"
                                                                 : "raw encoder (huge bandwidth)");
                if (encoder_idx == 2) {
                    printf("  Recommended: Install GPU drivers or libva/libva-drm\n");
                    printf("  Performance may be limited to lower bitrate/fps\n");
                }
            }
            break;
        } else {
            printf("WARNING: Encoder backend '%s' init failed, trying next...\n", backend->name);
            encoder_idx++;
        }
    }

    if (!encoder_initialized || !ctx->encoder_backend) {
        fprintf(stderr, "ERROR: All encoder backends failed!\n");
        fprintf(stderr, "Tried: ");
        for (int i = 0; encoder_backends[i].name; i++) {
            fprintf(stderr, "%s%s", i > 0 ? ", " : "", encoder_backends[i].name);
        }
        fprintf(stderr, "\n");
        return -1;
    }

    /* Initialize input with fallback (PHASE 6) */
    printf("INFO: Initializing input backend...\n");

    /* Try uinput first (primary) */
    if (rootstream_input_init(ctx) == 0) {
        printf("✓ Input backend 'uinput' initialized\n");
        ctx->active_backend.input_name = "uinput";
    } else {
        /* Try xdotool fallback */
        printf("INFO: uinput unavailable, trying xdotool...\n");
        if (input_init_xdotool(ctx) == 0) {
            ctx->active_backend.input_name = "xdotool";
        } else {
            /* Fall back to logging mode */
            printf("INFO: xdotool unavailable, using logging mode...\n");
            if (input_init_logging(ctx) == 0) {
                ctx->active_backend.input_name = "logging (debug)";
            } else {
                fprintf(stderr, "WARNING: All input backends failed\n");
            }
        }
    }

    /* Initialize audio capture with fallback */
    if (ctx->settings.audio_enabled) {
        printf("INFO: Initializing audio capture...\n");

        /* Backend list has static storage duration - safe to store pointers */
        static const audio_capture_backend_t capture_backends[] = {
            {
                .name = "ALSA",
                .init_fn = audio_capture_init_alsa,
                .capture_fn = audio_capture_frame_alsa,
                .cleanup_fn = audio_capture_cleanup_alsa,
                .is_available_fn = audio_capture_alsa_available,
            },
            {
                .name = "PulseAudio",
                .init_fn = audio_capture_init_pulse,
                .capture_fn = audio_capture_frame_pulse,
                .cleanup_fn = audio_capture_cleanup_pulse,
                .is_available_fn = audio_capture_pulse_available,
            },
            {
                .name = "PipeWire",
                .init_fn = audio_capture_init_pipewire,
                .capture_fn = audio_capture_frame_pipewire,
                .cleanup_fn = audio_capture_cleanup_pipewire,
                .is_available_fn = audio_capture_pipewire_available,
            },
            {
                .name = "Dummy (Silent)",
                .init_fn = audio_capture_init_dummy,
                .capture_fn = audio_capture_frame_dummy,
                .cleanup_fn = audio_capture_cleanup_dummy,
                .is_available_fn = NULL, /* Always available */
            },
            {NULL}};

        int capture_idx = 0;
        while (capture_backends[capture_idx].name) {
            printf("INFO: Attempting audio capture backend: %s\n",
                   capture_backends[capture_idx].name);

            if (capture_backends[capture_idx].is_available_fn &&
                !capture_backends[capture_idx].is_available_fn()) {
                printf("  → Not available on this system\n");
                capture_idx++;
                continue;
            }

            if (capture_backends[capture_idx].init_fn(ctx) == 0) {
                printf("✓ Audio capture backend '%s' initialized\n",
                       capture_backends[capture_idx].name);
                ctx->audio_capture_backend = &capture_backends[capture_idx];
                ctx->active_backend.audio_cap_name = capture_backends[capture_idx].name;
                break;
            } else {
                printf("WARNING: Audio capture backend '%s' failed, trying next...\n",
                       capture_backends[capture_idx].name);
                capture_idx++;
            }
        }

        if (!ctx->audio_capture_backend) {
            printf("WARNING: All audio capture backends failed, streaming video only\n");
            ctx->active_backend.audio_cap_name = "disabled";
        } else {
            /* Initialize Opus encoder */
            printf("INFO: Initializing Opus encoder...\n");
            if (rootstream_opus_encoder_init(ctx) < 0) {
                printf("WARNING: Opus encoder init failed, audio disabled\n");
                if (ctx->audio_capture_backend && ctx->audio_capture_backend->cleanup_fn) {
                    ctx->audio_capture_backend->cleanup_fn(ctx);
                }
                ctx->audio_capture_backend = NULL;
                ctx->active_backend.audio_cap_name = "disabled";
            }
        }
    } else {
        printf("INFO: Audio disabled in settings\n");
        ctx->audio_capture_backend = NULL;
        ctx->active_backend.audio_cap_name = "disabled";
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
        fprintf(stderr, "ERROR: Failed to allocate encode buffer (%zu bytes)\n", enc_buf_size);
        return -1;
    }

    /* Report active backends (PHASE 0) */
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  RootStream Backend Status                     ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("Capture:       %s\n", ctx->active_backend.capture_name);
    printf("Encoder:       %s\n", ctx->active_backend.encoder_name);
    printf("Audio Cap:     %s\n", ctx->active_backend.audio_cap_name);
    printf("Audio Play:    %s\n", ctx->active_backend.audio_play_name);
    printf("\n");

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
        if (rootstream_encode_frame_ex(ctx, &ctx->current_frame, enc_buf, &enc_size, &is_keyframe) <
            0) {
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
        uint8_t audio_buf[4000]; /* Max Opus packet size */
        size_t audio_size = 0;
        size_t num_samples = 0;

        if (ctx->audio_capture_backend && ctx->audio_capture_backend->capture_fn) {
            int audio_result =
                ctx->audio_capture_backend->capture_fn(ctx, audio_samples, &num_samples);
            if (audio_result < 0) {
                /* Audio capture failed, continue with video only */
                num_samples = 0;
            }
        }

        /* Encode audio if we have samples */
        if (num_samples > 0) {
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
                if (enc_size > 0 && rootstream_net_send_video(ctx, peer, enc_buf, enc_size,
                                                              ctx->current_frame.timestamp) < 0) {
                    fprintf(stderr, "ERROR: Video send failed (peer=%s)\n", peer->hostname);
                }

                /* Send audio if available */
                if (audio_size > 0) {
                    audio_packet_header_t header = {.timestamp_us = get_timestamp_us(),
                                                    .sample_rate = 48000,
                                                    .channels = 2,
                                                    .samples = (uint16_t)num_samples};

                    uint8_t payload[sizeof(audio_packet_header_t) + 4000];
                    memcpy(payload, &header, sizeof(header));
                    memcpy(payload + sizeof(header), audio_buf, audio_size);

                    if (rootstream_net_send_encrypted(ctx, peer, PKT_AUDIO, payload,
                                                      sizeof(header) + audio_size) < 0) {
                        fprintf(stderr, "ERROR: Audio send failed (peer=%s)\n", peer->hostname);
                    }
                }
            }
        }
        uint64_t send_end_us = get_timestamp_us();

        if (ctx->latency.enabled) {
            latency_sample_t sample = {.capture_us = capture_end_us - loop_start_us,
                                       .encode_us = encode_end_us - encode_start_us,
                                       .send_us = send_end_us - send_start_us,
                                       .total_us = send_end_us - loop_start_us};
            latency_record(&ctx->latency, &sample);
        }

        /* Process incoming packets */
        rootstream_net_recv(ctx, 1);
        rootstream_net_tick(ctx);

        /* Check peer health and reconnect if needed (PHASE 4) */
        check_peer_health(ctx);

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
/*
 * service_run_client — SDL2 CLI streaming client.
 *
 * PHASE-94 REFACTOR
 * -----------------
 * This function is now a thin wrapper over rs_client_session_*.
 * All receive/decode/audio logic has moved to src/client_session.c so that
 * the KDE client (and any future client) can reuse the same pipeline.
 *
 * The SDL2 display path is preserved exactly:
 *   - video is delivered to display_present_frame() via on_video callback
 *   - audio is delivered to the pre-existing audio playback backend via
 *     on_audio callback
 *
 * Before PHASE-94 this function contained ~200 lines that did everything
 * from decoder init to the receive loop to SDL event polling.  That code
 * now lives in src/client_session.c.
 *
 * If you are looking for the streaming loop, read src/client_session.c.
 */

/* SDL video callback — receives one decoded frame and presents it to the
 * SDL2 display window.  Called from the session run thread. */
typedef struct sdl_client_ctx {
    rootstream_ctx_t *ctx; /* Shared with the session's ctx */
} sdl_client_ctx_t;

static void sdl_on_video_frame(void *user, const rs_video_frame_t *frame) {
    /* user = sdl_client_ctx_t*
     * We need to map rs_video_frame_t back to frame_buffer_t so we can
     * call the existing display_present_frame() without changing its
     * interface.  This is a temporary bridge for the PHASE-94 MVP;
     * a future phase will update display_present_frame() to accept
     * rs_video_frame_t directly. */
    sdl_client_ctx_t *sdl = (sdl_client_ctx_t *)user;
    if (!sdl || !frame || !frame->plane0)
        return;

    frame_buffer_t fb;
    memset(&fb, 0, sizeof(fb));
    fb.width = frame->width;
    fb.height = frame->height;
    /* point data at plane0 — display_present_frame() treats the buffer as
     * a packed pixel array.  NV12 and RGBA both work here because the SDL
     * renderer already handles colour-space conversion. */
    fb.data = (uint8_t *)frame->plane0;
    fb.size = (size_t)(frame->stride0 * frame->height);
    if (frame->pixfmt == RS_PIXFMT_NV12) {
        /* Include UV plane in total size so the SDL renderer can
         * upload the full NV12 frame to a two-plane texture. */
        fb.size += (size_t)(frame->stride1 * frame->height / 2);
        fb.format = FRAME_FORMAT_NV12;
    } else {
        fb.format = FRAME_FORMAT_RGBA;
    }

    display_present_frame(sdl->ctx, &fb);
}

static void sdl_on_audio_frame(void *user, const rs_audio_frame_t *frame) {
    /* Deliver decoded PCM to whichever audio backend was initialised.
     * The audio backend was selected by service_run_client() before calling
     * rs_client_session_run(). */
    sdl_client_ctx_t *sdl = (sdl_client_ctx_t *)user;
    if (!sdl || !frame || !frame->samples)
        return;

    const audio_playback_backend_t *be = sdl->ctx->audio_playback_backend;
    if (be && be->playback_fn) {
        be->playback_fn(sdl->ctx, frame->samples, frame->num_samples);
    }
}

int service_run_client(rootstream_ctx_t *ctx) {
    if (!ctx)
        return -1;

    /* Install signal handlers (same as before PHASE-94) */
    signal(SIGTERM, service_signal_handler);
    signal(SIGINT, service_signal_handler);

    printf("INFO: Starting RootStream client service\n");

    /* ── Initialise SDL2 display ────────────────────────────────────────
     * The SDL window must be created before the session starts so that
     * display_present_frame() has a valid display context from the first
     * callback invocation. */
    if (display_init(ctx, "RootStream Client", 1920, 1080) < 0) {
        fprintf(stderr, "ERROR: Display initialization failed\n");
        return -1;
    }
    ctx->active_backend.display_name = "SDL2";

    /* ── Initialise audio playback with fallback chain ──────────────────
     * Same fallback chain as before PHASE-94 (ALSA → PulseAudio →
     * PipeWire → Dummy).  The selected backend is stored in ctx so
     * sdl_on_audio_frame() can forward PCM data to it. */
    if (ctx->settings.audio_enabled) {
        printf("INFO: Initializing audio playback...\n");

        static const audio_playback_backend_t playback_backends[] = {
            {
                .name = "ALSA",
                .init_fn = audio_playback_init_alsa,
                .playback_fn = audio_playback_write_alsa,
                .cleanup_fn = audio_playback_cleanup_alsa,
                .is_available_fn = audio_playback_alsa_available,
            },
            {
                .name = "PulseAudio",
                .init_fn = audio_playback_init_pulse,
                .playback_fn = audio_playback_write_pulse,
                .cleanup_fn = audio_playback_cleanup_pulse,
                .is_available_fn = audio_playback_pulse_available,
            },
            {
                .name = "PipeWire",
                .init_fn = audio_playback_init_pipewire,
                .playback_fn = audio_playback_write_pipewire,
                .cleanup_fn = audio_playback_cleanup_pipewire,
                .is_available_fn = audio_playback_pipewire_available,
            },
            {
                .name = "Dummy (Silent)",
                .init_fn = audio_playback_init_dummy,
                .playback_fn = audio_playback_write_dummy,
                .cleanup_fn = audio_playback_cleanup_dummy,
                .is_available_fn = NULL,
            },
            {NULL}};

        int idx = 0;
        while (playback_backends[idx].name) {
            printf("INFO: Attempting audio playback backend: %s\n", playback_backends[idx].name);
            if (playback_backends[idx].is_available_fn &&
                !playback_backends[idx].is_available_fn()) {
                printf("  → Not available on this system\n");
                idx++;
                continue;
            }
            if (playback_backends[idx].init_fn(ctx) == 0) {
                printf("✓ Audio playback backend '%s' initialized\n", playback_backends[idx].name);
                ctx->audio_playback_backend = &playback_backends[idx];
                ctx->active_backend.audio_play_name = playback_backends[idx].name;
                break;
            }
            printf("WARNING: Audio playback backend '%s' failed, trying next...\n",
                   playback_backends[idx].name);
            idx++;
        }

        if (!ctx->audio_playback_backend) {
            printf("WARNING: All audio playback backends failed, watching video only\n");
            ctx->active_backend.audio_play_name = "disabled";
        }
    } else {
        printf("INFO: Audio disabled in settings\n");
        ctx->active_backend.audio_play_name = "disabled";
    }

    printf("✓ SDL2 display and audio initialised\n");

    /* ── Create session and wire callbacks ──────────────────────────────
     * Build an rs_client_config_t from the existing rootstream_ctx_t.
     * The ctx itself is not passed to the session — the session allocates
     * its own ctx internally.  We share display/audio via the sdl_ctx shim.
     *
     * NOTE: This is a temporary bridge for PHASE-94 MVP.  A future phase
     * will unify rootstream_ctx_t with rs_client_session_t so there is only
     * one context object. */
    rs_client_config_t cfg = {
        .peer_host = ctx->peer_host,
        .peer_port = ctx->peer_port,
        .audio_enabled = ctx->settings.audio_enabled,
        .low_latency = true,
    };

    rs_client_session_t *session = rs_client_session_create(&cfg);
    if (!session) {
        fprintf(stderr, "ERROR: Failed to create client session\n");
        display_cleanup(ctx);
        return -1;
    }

    /* Wire the SDL shim callbacks so decoded frames reach the SDL window */
    sdl_client_ctx_t sdl_ctx = {.ctx = ctx};
    rs_client_session_set_video_callback(session, sdl_on_video_frame, &sdl_ctx);

    if (ctx->audio_playback_backend) {
        rs_client_session_set_audio_callback(session, sdl_on_audio_frame, &sdl_ctx);
    }

    /* Print backend status banner */
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  RootStream Client Backend Status              ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("Decoder:       (will be set by session)\n");
    printf("Display:       %s\n", ctx->active_backend.display_name);
    printf("Audio Play:    %s\n", ctx->active_backend.audio_play_name);
    printf("\n");

    /* ── Run the session (blocking) ─────────────────────────────────────
     * Poll SDL events on the main thread while the session run loop
     * processes network/decode on this same thread.
     *
     * For the SDL path, session and SDL event polling share the same thread
     * because service_run_client() was always single-threaded.  The KDE
     * client runs the session on a worker QThread instead. */
    int rc = rs_client_session_run(session);

    printf("Decoder:       %s\n", rs_client_session_decoder_name(session));

    /* ── Cleanup ─────────────────────────────────────────────────────── */
    rs_client_session_destroy(session);

    if (ctx->audio_playback_backend && ctx->audio_playback_backend->cleanup_fn) {
        ctx->audio_playback_backend->cleanup_fn(ctx);
    }
    display_cleanup(ctx);

    printf("✓ Client shutdown complete\n");
    return rc;
}
