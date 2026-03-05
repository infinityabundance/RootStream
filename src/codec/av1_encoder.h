/*
 * av1_encoder.h — AV1 encoder: libaom / SVT-AV1 / hardware backends
 *
 * SUPPORTED BACKENDS (in priority order)
 * ----------------------------------------
 * 1. Hardware VAAPI (av1_vaapi) — Intel Arc (Alchemist+), AMD RDNA3+.
 *    Lowest CPU usage, ~60fps 4K possible.
 * 2. Hardware NVENC (av1_nvenc) — NVIDIA RTX 40xx (Ada Lovelace+).
 *    Excellent quality-per-bit, near-lossless option.
 * 3. SVT-AV1 (svt-av1) — Intel open-source software encoder.
 *    Fastest software AV1 encoder; parallelises well across cores.
 *    Build with: cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
 * 4. libaom — reference AV1 encoder/decoder (slow but highest quality).
 *    Use for archival encoding or when SVT-AV1 is not available.
 *
 * COMPILE-TIME GUARDS
 * -------------------
 * Each backend is wrapped in HAVE_* guards so the binary compiles cleanly
 * whether or not the library is installed:
 *   HAVE_LIBAOM    — libaom encoder+decoder
 *   HAVE_SVT_AV1   — SVT-AV1 encoder (encoder only; use dav1d for decode)
 *   HAVE_DAV1D     — dav1d decoder (fast, open-source)
 *   HAVE_AV1_VAAPI — VA-API AV1 encode/decode (requires libva >= 1.19)
 *   HAVE_AV1_NVENC — NVENC AV1 (requires CUDA Toolkit >= 11.8 + RTX 40xx)
 *
 * FALLBACK NOTE
 * -------------
 * If no AV1 backend is available, av1_encoder_available() returns false
 * and the codec registry marks AV1 as encode_available=false, causing the
 * fallback chain to skip directly to VP9 or H.265.
 *
 * THREAD-SAFETY
 * -------------
 * Each av1_encoder_ctx_t is independent.  Multiple sessions may use
 * separate contexts concurrently without locking.
 */

#ifndef ROOTSTREAM_AV1_ENCODER_H
#define ROOTSTREAM_AV1_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Available AV1 encoding backends */
typedef enum {
    AV1_BACKEND_NONE   = 0,  /**< No backend available                     */
    AV1_BACKEND_VAAPI  = 1,  /**< VA-API hardware (Intel/AMD)              */
    AV1_BACKEND_NVENC  = 2,  /**< NVENC hardware (NVIDIA RTX 40xx)         */
    AV1_BACKEND_SVT    = 3,  /**< SVT-AV1 software (fast multi-core)       */
    AV1_BACKEND_LIBAOM = 4,  /**< libaom reference software (slow/quality) */
} av1_backend_t;

/** AV1 encoder tuning presets */
typedef enum {
    AV1_PRESET_SPEED    = 0,  /**< Fastest encode, highest CPU efficiency    */
    AV1_PRESET_BALANCED = 1,  /**< Balance between speed and quality          */
    AV1_PRESET_QUALITY  = 2,  /**< Best quality, slower encode               */
    AV1_PRESET_LOSSLESS = 3,  /**< Near-lossless (very high bitrate)         */
} av1_preset_t;

/** AV1 encoder configuration */
typedef struct {
    int           width;           /**< Frame width in pixels                */
    int           height;          /**< Frame height in pixels               */
    int           fps;             /**< Target frame rate                    */
    uint32_t      bitrate_kbps;    /**< Target bitrate (kilobits/sec)        */
    av1_preset_t  preset;          /**< Speed/quality tradeoff               */
    av1_backend_t preferred_backend; /**< Preferred backend (AUTO selects best) */
    bool          low_latency;     /**< Enable zero-latency mode (disables B-frames) */
    uint8_t       tile_columns;    /**< Parallel encoding tiles (SVT-AV1/libaom) */
    uint8_t       tile_rows;       /**< Parallel encoding tile rows          */
} av1_encoder_config_t;

/** Opaque AV1 encoder context */
typedef struct av1_encoder_ctx_s av1_encoder_ctx_t;

/* ── Capability probing ───────────────────────────────────────────── */

/**
 * av1_encoder_available — check if any AV1 encoding backend is available.
 *
 * This is the probe_fn registered in the codec registry.
 * Performs lightweight runtime checks (library dlopen, device ioctl) —
 * NOT a full encode test.
 *
 * @return true if at least one backend can encode AV1
 */
bool av1_encoder_available(void);

/**
 * av1_encoder_detect_backend — probe all backends and return the best.
 *
 * Priority: VAAPI > NVENC > SVT-AV1 > libaom > NONE.
 * This is the backend the encoder will actually use when initialized with
 * AV1_BACKEND_AUTO (set preferred_backend = AV1_BACKEND_NONE to auto-detect).
 */
av1_backend_t av1_encoder_detect_backend(void);

/* ── Lifecycle ────────────────────────────────────────────────────── */

/**
 * av1_encoder_create — allocate and initialise an AV1 encoder.
 *
 * Calls av1_encoder_detect_backend() if config->preferred_backend is NONE.
 *
 * @param config  Encoder configuration (copied; caller may free after call)
 * @return        Non-NULL context on success, NULL on error
 */
av1_encoder_ctx_t *av1_encoder_create(const av1_encoder_config_t *config);

/**
 * av1_encoder_destroy — close codec and free all resources.
 *
 * Safe to call with NULL.
 */
void av1_encoder_destroy(av1_encoder_ctx_t *ctx);

/* ── Encoding ─────────────────────────────────────────────────────── */

/**
 * av1_encoder_encode — encode one YUV420 frame to an AV1 OBU bitstream.
 *
 * @param ctx          Encoder context
 * @param yuv420       Input frame in YUV420 planar format
 * @param yuv_size     Size of @yuv420 in bytes (width * height * 3/2)
 * @param out          Output buffer for encoded AV1 OBU data
 * @param out_size     In: @out buffer capacity; Out: bytes written
 * @param is_keyframe  Out: true if output is a keyframe (intra-only frame)
 * @return             0 on success, -1 on error
 */
int av1_encoder_encode(av1_encoder_ctx_t *ctx,
                       const uint8_t     *yuv420,
                       size_t             yuv_size,
                       uint8_t           *out,
                       size_t            *out_size,
                       bool              *is_keyframe);

/**
 * av1_encoder_request_keyframe — force the next encoded frame to be an
 * intra (keyframe).  Used when a new client joins the session.
 */
void av1_encoder_request_keyframe(av1_encoder_ctx_t *ctx);

/* ── Introspection ────────────────────────────────────────────────── */

/**
 * av1_encoder_get_backend — return the backend actually in use.
 */
av1_backend_t av1_encoder_get_backend(const av1_encoder_ctx_t *ctx);

/**
 * av1_encoder_get_stats — populate *bitrate_kbps and *fps_actual from the
 * last second of encoded data.  May be NULL pointers (silently skipped).
 */
void av1_encoder_get_stats(const av1_encoder_ctx_t *ctx,
                           uint32_t *bitrate_kbps,
                           float    *fps_actual);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_AV1_ENCODER_H */
