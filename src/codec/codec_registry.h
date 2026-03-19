/*
 * codec_registry.h — Central codec capability registry
 *
 * OVERVIEW
 * --------
 * The codec registry is the single source of truth for what video/audio
 * codecs are available on the current host at runtime.  Every encoder and
 * decoder in the RootStream codebase registers its capabilities here; the
 * session-setup path queries the registry to select the best available
 * codec for each session.
 *
 * DESIGN RATIONALE
 * ----------------
 * Instead of scattered `#ifdef HAVE_X / avcodec_find_encoder()` checks
 * throughout every subsystem, we centralise capability detection here.
 * This:
 *   1. Makes the selection logic testable (inject a mock probe function).
 *   2. Provides a single dashboard view of what is available.
 *   3. Allows the fallback chain (codec_fallback.h) to iterate the
 *      registry without knowing the individual probe implementations.
 *
 * CODEC ID SPACE
 * --------------
 * IDs 0–4   existing (RAW, H264, H265, AV1, VP9)
 * IDs 5–6   this phase (VVC/H.266, AV2)
 * IDs 7–15  reserved
 * IDs 16–31 audio codecs (defined separately in stream_config.h)
 *
 * THREAD-SAFETY
 * -------------
 * NOT thread-safe.  The registry is populated once at startup before any
 * encoding threads are created.  After population, read-only access from
 * any thread is safe.
 */

#ifndef ROOTSTREAM_CODEC_REGISTRY_H
#define ROOTSTREAM_CODEC_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Codec IDs — extend stream_config SCFG_VCODEC_* ──────────────── */

/** Video codec IDs used throughout the codec layer.
 *  Must match SCFG_VCODEC_* values in stream_config.h for on-wire compat. */
#define CREG_VCODEC_RAW 0     /**< Uncompressed / pass-through               */
#define CREG_VCODEC_H264 1    /**< H.264 / AVC (hardware + software)          */
#define CREG_VCODEC_H265 2    /**< H.265 / HEVC (hardware + software)         */
#define CREG_VCODEC_AV1 3     /**< AV1 (libaom / SVT-AV1 / hardware)          */
#define CREG_VCODEC_VP9 4     /**< VP9 (libvpx / hardware VAAPI/NVENC)        */
#define CREG_VCODEC_VVC 5     /**< H.266 / VVC (VVenC + VVdeC)               */
#define CREG_VCODEC_AV2 6     /**< AV2 (future spec / libdave gateway)        */
#define CREG_VCODEC_LIBDAVE 7 /**< Discord libdave packetised media codec     */
#define CREG_VCODEC_MAX 8     /**< Sentinel — one past last valid video codec */

/* ── Encoder backends ─────────────────────────────────────────────── */

/** Encoder backend flags — a codec may support multiple backends.
 *  Multiple flags may be OR'd together. */
#define CREG_BACKEND_NONE 0x00       /**< No backend available               */
#define CREG_BACKEND_SW 0x01         /**< Pure CPU software encoding         */
#define CREG_BACKEND_VAAPI 0x02      /**< Linux VA-API hardware acceleration  */
#define CREG_BACKEND_NVENC 0x04      /**< NVIDIA NVENC hardware encoding     */
#define CREG_BACKEND_QSV 0x08        /**< Intel QuickSync Video              */
#define CREG_BACKEND_VIDEOTB 0x10    /**< Apple VideoToolbox (macOS/iOS)     */
#define CREG_BACKEND_MEDIACODEC 0x20 /**< Android MediaCodec                 */
#define CREG_BACKEND_V4L2 0x40       /**< V4L2 M2M (Raspberry Pi, etc.)      */

/** Per-codec capability entry.
 *  Populated by each codec module calling creg_register() at startup. */
typedef struct {
    uint8_t codec_id;         /**< CREG_VCODEC_* constant                    */
    const char *name;         /**< Human-readable short name ("av1", "vvc")  */
    const char *long_name;    /**< Full name ("AOMedia Video 1")              */
    uint8_t encoder_backends; /**< OR'd CREG_BACKEND_* flags (encoder side)  */
    uint8_t decoder_backends; /**< OR'd CREG_BACKEND_* flags (decoder side)  */
    bool encode_available;    /**< At least one encoder backend is live       */
    bool decode_available;    /**< At least one decoder backend is live       */
    bool hw_preferred;        /**< True when a HW backend is available        */

    /** Optional probe function — called by creg_probe() to dynamically
     *  verify availability.  May be NULL (entry is assumed always-available). */
    bool (*probe_fn)(uint8_t codec_id);
} creg_entry_t;

/** Opaque registry handle */
typedef struct creg_registry_s creg_registry_t;

/* ── Lifecycle ────────────────────────────────────────────────────── */

/**
 * creg_create — allocate and zero-initialise the codec registry.
 *
 * Call once at application startup before any codec operations.
 * @return Non-NULL handle, or NULL on OOM.
 */
creg_registry_t *creg_create(void);

/**
 * creg_destroy — free the codec registry.
 *
 * Does not free codec resources — caller must have cleaned those up first.
 */
void creg_destroy(creg_registry_t *r);

/* ── Registration ─────────────────────────────────────────────────── */

/**
 * creg_register — register or update a codec capability entry.
 *
 * Replaces any existing entry with the same codec_id.
 *
 * @param r      Registry
 * @param entry  Capability entry to register (copied by value)
 * @return       0 on success, -1 on full registry or NULL input
 */
int creg_register(creg_registry_t *r, const creg_entry_t *entry);

/* ── Query ────────────────────────────────────────────────────────── */

/**
 * creg_lookup — find a registered codec by ID.
 *
 * @param r         Registry
 * @param codec_id  CREG_VCODEC_* constant
 * @return          Pointer to internal entry (read-only), or NULL if not found
 */
const creg_entry_t *creg_lookup(const creg_registry_t *r, uint8_t codec_id);

/**
 * creg_encode_available — check if any encoder is available for a codec.
 *
 * @return true if encode_available is set for codec_id
 */
bool creg_encode_available(const creg_registry_t *r, uint8_t codec_id);

/**
 * creg_decode_available — check if any decoder is available for a codec.
 */
bool creg_decode_available(const creg_registry_t *r, uint8_t codec_id);

/**
 * creg_hw_preferred — true if a hardware backend is available for the codec.
 */
bool creg_hw_preferred(const creg_registry_t *r, uint8_t codec_id);

/* ── Probing ──────────────────────────────────────────────────────── */

/**
 * creg_probe_all — call each registered codec's probe_fn and update
 * encode_available / decode_available / hw_preferred accordingly.
 *
 * Call once after creg_register() calls complete, before any encode/decode.
 *
 * @return Number of codecs that reported as available.
 */
int creg_probe_all(creg_registry_t *r);

/**
 * creg_register_all_defaults — register all built-in codecs that are
 * compiled in (based on HAVE_* preprocessor flags).
 *
 * This is the preferred startup call — it inits the complete set of
 * codecs the binary was compiled with, then probes for hardware availability.
 *
 * @param r  Registry to populate
 * @return   Number of codecs registered
 */
int creg_register_all_defaults(creg_registry_t *r);

/* ── Enumeration ──────────────────────────────────────────────────── */

/**
 * creg_count — return number of registered codecs.
 */
int creg_count(const creg_registry_t *r);

/**
 * creg_foreach — iterate all registered codec entries.
 *
 * @param r    Registry
 * @param fn   Called for each entry; return false to stop iteration early
 * @param user Opaque pointer forwarded to fn
 */
void creg_foreach(const creg_registry_t *r, bool (*fn)(const creg_entry_t *e, void *user),
                  void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CODEC_REGISTRY_H */
