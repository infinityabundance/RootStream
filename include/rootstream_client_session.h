/*
 * rootstream_client_session.h — Reusable streaming client session API
 *
 * OVERVIEW
 * --------
 * This header defines the callback-based client session API that decouples
 * the streaming backend (network → crypto → reassembly → decode) from the
 * display layer (SDL2, KDE/Qt, Android SurfaceView, iOS CALayer, etc.).
 *
 * Before this API (pre-PHASE-94), all session logic was embedded inside
 * service_run_client() in service.c, welded directly to SDL2 display and
 * the main loop.  That made it impossible for the KDE client to reuse the
 * real protocol/decoder stack without duplicating it.
 *
 * After this API:
 *   - service.c::service_run_client() wraps rs_client_session_* (same
 *     behaviour, zero regression for the CLI/SDL path).
 *   - KDE client creates a session on a worker QThread, registers callbacks,
 *     and receives decoded frames into its Qt renderer.
 *   - Android, iOS, Windows clients can do the same.
 *
 * ARCHITECTURE
 *
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │                     rootstream_core                          │
 *   │                                                              │
 *   │  network.c → crypto.c → packet_validate.c → decoder/...c    │
 *   │                              ↓                               │
 *   │                  rs_client_session_t (client_session.c)      │
 *   │                    on_video_fn ──→ caller's video renderer   │
 *   │                    on_audio_fn ──→ caller's audio output     │
 *   └──────────────────────────────────────────────────────────────┘
 *
 * THREADING MODEL
 * ---------------
 * rs_client_session_run() is a blocking loop.  It must be called on a
 * dedicated thread (not the UI thread).  The callbacks are invoked FROM
 * that same thread.
 *
 * Callers must ensure their callbacks are thread-safe.  For Qt, the
 * recommended pattern is:
 *
 *   QMetaObject::invokeMethod(renderer, "submitFrame",
 *                             Qt::QueuedConnection,
 *                             Q_ARG(QByteArray, frameData));
 *
 * Or copy the frame data and post a signal (see StreamBackendConnector.cpp).
 *
 * OWNERSHIP
 * ---------
 * rs_video_frame_t and rs_audio_frame_t are valid ONLY for the duration of
 * the callback.  The caller MUST copy any data it needs to retain.  This is
 * intentional: the backend reuses decode buffers to avoid per-frame allocation.
 *
 * NULL SAFETY
 * -----------
 * All public functions are NULL-safe.  Calling any function with a NULL
 * session pointer returns immediately (0 / false / NULL as appropriate).
 */

#ifndef ROOTSTREAM_CLIENT_SESSION_H
#define ROOTSTREAM_CLIENT_SESSION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Pixel formats ────────────────────────────────────────────────── */

/**
 * Pixel formats that the decoder may produce.
 * The actual format depends on the decode backend and the source bitstream.
 */
typedef enum {
    RS_PIXFMT_NV12  = 0,   /**< YUV 4:2:0, Y plane + interleaved UV plane
                              *  Most common for VA-API decoded frames.
                              *  plane0 = Y (stride0), plane1 = UV (stride1) */
    RS_PIXFMT_YUV420 = 1,  /**< YUV 4:2:0 planar (I420)
                              *  plane0=Y, plane1=U, plane2=V (stride0, stride1, stride2) */
    RS_PIXFMT_RGBA  = 2,   /**< 32-bit RGBA, 4 bytes/pixel, plane1=NULL
                              *  Used by software decoder / test paths */
    RS_PIXFMT_BGRA  = 3,   /**< 32-bit BGRA (Windows / Direct3D format) */
    RS_PIXFMT_P010  = 4,   /**< 10-bit NV12 (HDR content) */
} rs_pixfmt_t;

/* ── Video frame descriptor ───────────────────────────────────────── */

/**
 * rs_video_frame_t — describes one decoded video frame.
 *
 * LIFETIME: valid only for the duration of the on_video callback.
 * Copy plane data if you need it after the callback returns.
 *
 * For NV12 (most common):
 *   plane0 = Y  (luma),   stride0 = bytes per row of Y
 *   plane1 = UV (chroma), stride1 = bytes per row of UV (same as stride0)
 *   plane2 = NULL
 *
 * For RGBA:
 *   plane0 = pixel data, stride0 = width * 4
 *   plane1 = plane2 = NULL
 */
typedef struct {
    int            width;       /**< Frame width in pixels                   */
    int            height;      /**< Frame height in pixels                  */
    rs_pixfmt_t    pixfmt;      /**< Pixel format of plane0/plane1/plane2    */
    const uint8_t *plane0;      /**< Primary plane (Y for NV12, RGBA data)   */
    const uint8_t *plane1;      /**< Secondary plane (UV for NV12), or NULL  */
    const uint8_t *plane2;      /**< Tertiary plane (V for I420), or NULL    */
    int            stride0;     /**< Bytes per row for plane0                */
    int            stride1;     /**< Bytes per row for plane1 (0 if NULL)    */
    int            stride2;     /**< Bytes per row for plane2 (0 if NULL)    */
    uint64_t       pts_us;      /**< Presentation timestamp (microseconds)   */
    bool           is_keyframe; /**< True if this is an intra-coded frame     */
} rs_video_frame_t;

/* ── Audio frame descriptor ───────────────────────────────────────── */

/**
 * rs_audio_frame_t — describes one decoded audio buffer.
 *
 * LIFETIME: valid only for the duration of the on_audio callback.
 * Copy samples if you need them after the callback returns.
 */
typedef struct {
    int16_t       *samples;      /**< Interleaved PCM, int16 little-endian   */
    size_t         num_samples;  /**< Total samples (frames × channels)      */
    int            channels;     /**< Number of audio channels               */
    int            sample_rate;  /**< Samples per second (e.g. 48000)        */
    uint64_t       pts_us;       /**< Presentation timestamp (microseconds)  */
} rs_audio_frame_t;

/* ── Callback types ───────────────────────────────────────────────── */

/**
 * rs_on_video_frame_fn — called for each decoded video frame.
 *
 * @param user   Opaque pointer supplied to rs_client_session_set_video_callback()
 * @param frame  Decoded frame descriptor (valid during callback only)
 *
 * IMPORTANT: This callback is called from the session run thread.
 *            Return quickly.  Copy frame data rather than processing in-place.
 */
typedef void (*rs_on_video_frame_fn)(void *user, const rs_video_frame_t *frame);

/**
 * rs_on_audio_frame_fn — called for each decoded audio buffer.
 *
 * @param user   Opaque pointer supplied to rs_client_session_set_audio_callback()
 * @param frame  Audio frame descriptor (valid during callback only)
 */
typedef void (*rs_on_audio_frame_fn)(void *user, const rs_audio_frame_t *frame);

/**
 * rs_on_state_change_fn — called when session connection state changes.
 *
 * @param user    Opaque pointer
 * @param state   Human-readable state string ("connecting", "connected",
 *                "disconnected", "error: <reason>")
 */
typedef void (*rs_on_state_change_fn)(void *user, const char *state);

/* ── Session configuration ────────────────────────────────────────── */

/**
 * rs_client_config_t — session connection parameters.
 *
 * Strings are NOT copied by rs_client_session_create() — the caller must
 * ensure they remain valid for the lifetime of the session.
 * (Typically stack-allocated configs are fine since create() is synchronous.)
 */
typedef struct {
    const char *peer_host;      /**< Peer hostname or IP address            */
    int         peer_port;      /**< Peer port number                       */
    const char *peer_code;      /**< Optional peer pairing code             */
    bool        audio_enabled;  /**< Enable audio decode + callback         */
    bool        low_latency;    /**< Request low-latency decode mode        */
} rs_client_config_t;

/* ── Session handle ───────────────────────────────────────────────── */

/** Opaque session handle returned by rs_client_session_create() */
typedef struct rs_client_session_s rs_client_session_t;

/* ── Lifecycle ────────────────────────────────────────────────────── */

/**
 * rs_client_session_create — allocate and initialise a new client session.
 *
 * Does NOT connect to the peer yet — connection happens inside
 * rs_client_session_run().
 *
 * @param cfg  Connection configuration (strings must outlive the session)
 * @return     Non-NULL handle on success, NULL on OOM or invalid cfg
 */
rs_client_session_t *rs_client_session_create(const rs_client_config_t *cfg);

/**
 * rs_client_session_destroy — stop if running and free all resources.
 *
 * Safe to call while rs_client_session_run() is executing — it will
 * request a stop and then clean up.  Callers typically:
 *   1. Call rs_client_session_request_stop() from another thread
 *   2. Join the session thread
 *   3. Call rs_client_session_destroy()
 *
 * Safe to call with NULL.
 */
void rs_client_session_destroy(rs_client_session_t *s);

/* ── Callback registration ────────────────────────────────────────── */

/**
 * rs_client_session_set_video_callback — register the video frame callback.
 *
 * Must be called before rs_client_session_run().  Replacing the callback
 * while running is NOT thread-safe.
 *
 * @param s     Session handle
 * @param cb    Callback function (may be NULL to disable video output)
 * @param user  Opaque pointer forwarded to every cb invocation
 */
void rs_client_session_set_video_callback(rs_client_session_t    *s,
                                          rs_on_video_frame_fn    cb,
                                          void                   *user);

/**
 * rs_client_session_set_audio_callback — register the audio callback.
 *
 * @param s     Session handle
 * @param cb    Callback (may be NULL to disable audio output)
 * @param user  Opaque pointer forwarded to every cb invocation
 */
void rs_client_session_set_audio_callback(rs_client_session_t    *s,
                                          rs_on_audio_frame_fn    cb,
                                          void                   *user);

/**
 * rs_client_session_set_state_callback — register the state-change callback.
 *
 * @param s     Session handle
 * @param cb    Callback (may be NULL)
 * @param user  Opaque pointer forwarded to every cb invocation
 */
void rs_client_session_set_state_callback(rs_client_session_t    *s,
                                          rs_on_state_change_fn   cb,
                                          void                   *user);

/* ── Run / stop ───────────────────────────────────────────────────── */

/**
 * rs_client_session_run — connect to peer and run the receive/decode loop.
 *
 * BLOCKS until the session ends (peer disconnects, error, or stop requested).
 * Must be called on a dedicated thread — NOT the UI thread.
 *
 * @param s  Session handle
 * @return   0 on clean stop, negative on error
 */
int rs_client_session_run(rs_client_session_t *s);

/**
 * rs_client_session_request_stop — signal the run loop to exit.
 *
 * Thread-safe.  rs_client_session_run() will return within one poll cycle
 * (typically <20ms) after this is called.
 *
 * @param s  Session handle
 */
void rs_client_session_request_stop(rs_client_session_t *s);

/* ── Introspection ────────────────────────────────────────────────── */

/**
 * rs_client_session_is_running — true while run() is executing.
 *
 * Thread-safe (atomic read).
 */
bool rs_client_session_is_running(const rs_client_session_t *s);

/**
 * rs_client_session_decoder_name — return the decoder backend name string
 * ("VA-API", "Media Foundation", "FFmpeg", "Software", …).
 *
 * Valid only after rs_client_session_run() has started and the decoder has
 * been initialised.  Returns "unknown" before that point.
 */
const char *rs_client_session_decoder_name(const rs_client_session_t *s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CLIENT_SESSION_H */
