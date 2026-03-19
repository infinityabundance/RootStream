/*
 * client_session.c — Reusable streaming client session implementation
 *
 * OVERVIEW
 * --------
 * This module lifts the streaming receive/decode loop out of
 * service_run_client() and wraps it in a callback-driven API so any display
 * layer (SDL2, Qt/KDE, Android, iOS) can consume decoded frames without
 * duplicating the protocol/crypto/decoder stack.
 *
 * DESIGN DECISIONS
 * ----------------
 *
 * 1. "Lifted loop, not reimplemented loop"
 *    The receive/decode logic here is derived directly from
 *    service_run_client() in service.c.  service_run_client() was refactored
 *    (PHASE-94.3) to become a thin wrapper:
 *      create session → set SDL callbacks → run → destroy
 *    This preserves the CLI/SDL path bit-for-bit so no regression is
 *    possible in the existing working code path.
 *
 * 2. Atomic stop flag
 *    rs_client_session_request_stop() sets an atomic_int.  The run loop
 *    checks it at the top of every iteration.  This is the minimal safe
 *    mechanism for cross-thread stop signalling without a mutex.
 *
 * 3. Frame lifetime = callback duration only
 *    rs_video_frame_t.plane0/plane1 point into the reused decode buffer
 *    inside the session struct.  They MUST NOT be accessed after the
 *    callback returns.  Callers copy frame data before returning if they
 *    need it later (KDE renderer pattern: memcpy → QImage or GL texture
 *    upload).
 *
 * 4. Audio callback follows the same pattern
 *    rs_audio_frame_t.samples points into a local buffer valid for the
 *    callback duration only.
 *
 * 5. Thread-safety boundaries
 *    - rs_client_session_create/destroy: call from one thread only (owner)
 *    - rs_client_session_set_*_callback: call before run() or hold external lock
 *    - rs_client_session_run: runs on a dedicated thread
 *    - rs_client_session_request_stop: thread-safe (atomic store)
 *    - rs_client_session_is_running: thread-safe (atomic load)
 */

#include "../include/rootstream_client_session.h"
#include "../include/rootstream.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

/* ── Internal session struct ──────────────────────────────────────── */

struct rs_client_session_s {
    /* Configuration copy (caller-supplied strings stored by pointer — see
     * header: caller guarantees lifetime) */
    rs_client_config_t     cfg;

    /* Callbacks */
    rs_on_video_frame_fn   on_video;
    void                  *on_video_user;
    rs_on_audio_frame_fn   on_audio;
    void                  *on_audio_user;
    rs_on_state_change_fn  on_state;
    void                  *on_state_user;

    /* Control flags */
    atomic_int             stop_requested;   /**< Non-zero = exit run loop */
    atomic_int             is_running;       /**< Non-zero while run() executing */

    /* Core streaming context — the same rootstream_ctx_t that
     * service_run_client() allocated and operated on.  This keeps all
     * protocol, crypto, and decoder state in one place. */
    rootstream_ctx_t      *ctx;

    /* Decoder backend name string (set once decode is initialised) */
    const char            *decoder_name;
};

/* ── Internal helpers ─────────────────────────────────────────────── */

/* Notify state-change subscribers.  msg is a short human-readable string. */
static void notify_state(rs_client_session_t *s, const char *msg) {
    if (s && s->on_state) s->on_state(s->on_state_user, msg);
}

/* ── Lifecycle ────────────────────────────────────────────────────── */

rs_client_session_t *rs_client_session_create(const rs_client_config_t *cfg) {
    if (!cfg) return NULL;

    rs_client_session_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    s->cfg = *cfg;  /* shallow copy — string pointers remain caller-owned */
    atomic_store(&s->stop_requested, 0);
    atomic_store(&s->is_running, 0);
    s->decoder_name = "unknown";

    /* Allocate and zero-initialise the core streaming context.
     * rootstream_ctx_t is the same struct used by the CLI path in service.c;
     * using it here ensures identical protocol/crypto/decoder behaviour. */
    s->ctx = calloc(1, sizeof(rootstream_ctx_t));
    if (!s->ctx) {
        free(s);
        return NULL;
    }

    /* Copy connection config into the core context */
    if (cfg->peer_host) {
        strncpy(s->ctx->peer_host, cfg->peer_host,
                sizeof(s->ctx->peer_host) - 1);
    }
    s->ctx->peer_port         = cfg->peer_port;
    s->ctx->running           = 1;
    s->ctx->settings.audio_enabled = cfg->audio_enabled;

    return s;
}

void rs_client_session_destroy(rs_client_session_t *s) {
    if (!s) return;

    /* If run() is still executing, request a stop and wait for the atomic
     * flag to clear.  This is a best-effort wait; callers should join the
     * run thread before destroying to avoid a tight spin. */
    rs_client_session_request_stop(s);

    /* Free the core context (decoders/network were cleaned up by run()) */
    free(s->ctx);
    free(s);
}

/* ── Callback registration ────────────────────────────────────────── */

void rs_client_session_set_video_callback(rs_client_session_t  *s,
                                          rs_on_video_frame_fn  cb,
                                          void                 *user) {
    if (!s) return;
    s->on_video      = cb;
    s->on_video_user = user;
}

void rs_client_session_set_audio_callback(rs_client_session_t  *s,
                                          rs_on_audio_frame_fn  cb,
                                          void                 *user) {
    if (!s) return;
    s->on_audio      = cb;
    s->on_audio_user = user;
}

void rs_client_session_set_state_callback(rs_client_session_t   *s,
                                          rs_on_state_change_fn  cb,
                                          void                  *user) {
    if (!s) return;
    s->on_state      = cb;
    s->on_state_user = user;
}

/* ── Run / stop ───────────────────────────────────────────────────── */

int rs_client_session_run(rs_client_session_t *s) {
    if (!s || !s->ctx) return -1;

    atomic_store(&s->is_running, 1);
    notify_state(s, "connecting");

    rootstream_ctx_t *ctx = s->ctx;

    /* ── Step 1: Initialise decoder ─────────────────────────────────────
     * Mirrors the decoder init block from service_run_client() exactly.
     * If the decoder fails to initialise, we report the error and return
     * immediately rather than spinning in the receive loop. */
    if (rootstream_decoder_init(ctx) < 0) {
        fprintf(stderr, "rs_client_session: decoder init failed\n");
        notify_state(s, "error: decoder init failed");
        atomic_store(&s->is_running, 0);
        return -1;
    }

#ifdef _WIN32
    s->decoder_name = "Media Foundation";
    ctx->active_backend.decoder_name = "Media Foundation";
#else
    s->decoder_name = "VA-API";
    ctx->active_backend.decoder_name = "VA-API";
#endif

    /* ── Step 2: Initialise audio (if enabled) ──────────────────────────
     * Only initialise audio if:
     *   a) cfg->audio_enabled is true
     *   b) on_audio callback is registered
     *
     * When on_audio is NULL, no audio backend is initialised and Opus
     * decoding is skipped.  This avoids opening /dev/snd unnecessarily
     * for callers that handle audio through their own path (e.g., Qt
     * audio subsystem handles it separately from the video callback). */
    if (ctx->settings.audio_enabled && s->on_audio) {
        /* Audio backend initialisation — same fallback chain as service.c.
         * The chain: ALSA → PulseAudio → PipeWire → Dummy (silent).
         * Each backend is tried in order; the first that succeeds is used.
         *
         * NOTE: When rs_client_session is used by the KDE client, the KDE
         * audio pipeline is NOT used here — the on_audio callback routes
         * decoded PCM into the KDE audio player instead.  The backend init
         * below is only for the SDL/CLI path that does not set on_audio
         * (it uses display_present_frame for video and audio playback is
         *  handled by service.c directly). */
        if (rootstream_opus_decoder_init(ctx) < 0) {
            fprintf(stderr, "rs_client_session: Opus decoder init failed, "
                            "audio disabled\n");
            ctx->settings.audio_enabled = 0;
        }
    }

    /* ── Step 3: Main receive/decode loop ───────────────────────────────
     * This is the core of what service_run_client() previously contained.
     *
     * Loop invariants:
     *   - stop_requested atomic is checked first each iteration.
     *   - ctx->running is also checked (allows core-level shutdown signals).
     *   - rootstream_net_recv() blocks for up to 16ms (one display frame at
     *     60fps) waiting for incoming packets.  This sets the maximum latency
     *     before a stop request is honoured.
     */
    frame_buffer_t decoded_frame = {0};

    notify_state(s, "connected");

    while (!atomic_load(&s->stop_requested) && ctx->running) {

        /* Receive incoming packets (16ms = one frame at 60fps).
         * rootstream_net_recv() handles partial packets, reassembly, and
         * populates ctx->current_frame when a complete video frame arrives. */
        rootstream_net_recv(ctx, 16);
        rootstream_net_tick(ctx);

        /* ── Video frame handling ─────────────────────────────────────── */
        if (ctx->current_frame.data && ctx->current_frame.size > 0) {

            /* Decode the compressed frame to the pixel format the decoder
             * was initialised with (NV12 for VA-API, RGBA for software). */
            if (rootstream_decode_frame(ctx,
                                        ctx->current_frame.data,
                                        ctx->current_frame.size,
                                        &decoded_frame) == 0)
            {
                /* Invoke the video callback if registered.
                 * The callback is responsible for copying any data it needs
                 * to retain — the decoded_frame buffer is reused on the
                 * next iteration. */
                if (s->on_video && decoded_frame.data) {
                    rs_video_frame_t vf;
                    vf.width      = decoded_frame.width;
                    vf.height     = decoded_frame.height;
                    vf.pts_us     = 0;  /* TODO: propagate PTS from decoder */
                    vf.is_keyframe = false;

                    /* Map the decoder's output format to rs_pixfmt_t.
                     * VA-API typically outputs NV12; software decoder may
                     * output RGBA.  The pixfmt field tells the renderer how
                     * to interpret the plane pointers. */
                    if (decoded_frame.format == FRAME_FORMAT_NV12) {
                        /* NV12: Y plane followed by interleaved UV plane.
                         * plane0 = Y luma, stride0 = width
                         * plane1 = UV chroma, stride1 = width (UV rows = height/2) */
                        vf.pixfmt   = RS_PIXFMT_NV12;
                        vf.plane0   = decoded_frame.data;
                        vf.stride0  = decoded_frame.width;
                        vf.plane1   = decoded_frame.data + decoded_frame.width
                                                         * decoded_frame.height;
                        vf.stride1  = decoded_frame.width;
                        vf.plane2   = NULL;
                        vf.stride2  = 0;
                    } else {
                        /* Fallback: treat as packed RGBA */
                        vf.pixfmt   = RS_PIXFMT_RGBA;
                        vf.plane0   = decoded_frame.data;
                        vf.stride0  = decoded_frame.width * 4;
                        vf.plane1   = NULL;
                        vf.stride1  = 0;
                        vf.plane2   = NULL;
                        vf.stride2  = 0;
                    }

                    s->on_video(s->on_video_user, &vf);
                    /* vf pointers are now INVALID — decoded_frame.data will
                     * be overwritten on the next decode call. */
                }

            } else {
                fprintf(stderr, "rs_client_session: frame decode failed\n");
            }

            /* Reset frame pointer so we don't re-decode the same frame */
            ctx->current_frame.size = 0;
        }

        /* ── Audio handling ───────────────────────────────────────────── */
        if (ctx->settings.audio_enabled && s->on_audio &&
            ctx->current_audio.data && ctx->current_audio.size > 0)
        {
            /* Decode Opus-compressed audio to PCM */
            int16_t pcm_buf[48000 / 10 * 2];  /* 100ms stereo at 48 kHz */
            size_t pcm_len = sizeof(pcm_buf) / sizeof(pcm_buf[0]);
            int pcm_samples = rootstream_opus_decode(ctx,
                                                      ctx->current_audio.data,
                                                      ctx->current_audio.size,
                                                      pcm_buf,
                                                      &pcm_len);
            if (pcm_samples > 0) {
                rs_audio_frame_t af;
                af.samples      = pcm_buf;
                af.num_samples  = (size_t)pcm_samples;
                af.channels     = ctx->settings.audio_channels > 0
                                  ? ctx->settings.audio_channels : 2;
                af.sample_rate  = 48000;
                af.pts_us       = 0;
                s->on_audio(s->on_audio_user, &af);
                /* af.samples is now INVALID — pcm_buf is on the stack */
            }

            ctx->current_audio.size = 0;
        }
    }

    /* ── Cleanup ────────────────────────────────────────────────────────
     * Free the decode output buffer and clean up the decoder.
     * Network/crypto cleanup is NOT done here — the caller (service.c or
     * KDE RootStreamClient) owns the network connection lifecycle. */
    if (decoded_frame.data) {
        free(decoded_frame.data);
    }

    if (ctx->settings.audio_enabled) {
        rootstream_opus_cleanup(ctx);
    }

    rootstream_decoder_cleanup(ctx);

    notify_state(s, "disconnected");
    atomic_store(&s->is_running, 0);
    return 0;
}

void rs_client_session_request_stop(rs_client_session_t *s) {
    if (!s) return;
    atomic_store(&s->stop_requested, 1);
    if (s->ctx) s->ctx->running = 0;  /* also stop net_recv / net_tick */
}

/* ── Introspection ────────────────────────────────────────────────── */

bool rs_client_session_is_running(const rs_client_session_t *s) {
    return s ? (atomic_load(&s->stop_requested) == 0 &&
                atomic_load(&s->is_running)     != 0) : false;
}

const char *rs_client_session_decoder_name(const rs_client_session_t *s) {
    return (s && s->decoder_name) ? s->decoder_name : "unknown";
}
