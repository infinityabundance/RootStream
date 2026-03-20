/*
 * codec_fallback.h — Ordered codec fallback chain
 *
 * OVERVIEW
 * --------
 * When the user requests codec X but X is unavailable (no HW, no library),
 * the fallback chain selects the next best codec that IS available.
 *
 * FALLBACK PHILOSOPHY
 * -------------------
 * Higher-numbered CREG_VCODEC_* codecs (VVC, AV2) are newer and offer
 * better compression but may not be available everywhere.  Older codecs
 * (H.265, H.264) are the safety nets.  The fallback order is:
 *
 *   Preferred order for quality-first streaming:
 *     AV2 > VVC > AV1 > VP9 > H.265 > H.264 > RAW
 *
 *   Preferred order for compatibility-first (e.g., older devices):
 *     H.264 > H.265 > VP9 > AV1 > VVC > AV2
 *
 *   libdave is treated as a parallel option, not a fallback to/from
 *   other codecs — it uses its own transport and packet format.
 *
 * MULTIPLE INDEPENDENT CHAINS
 * ---------------------------
 * The caller constructs a chain by providing an ordered list of codec IDs.
 * cfb_select_best() walks the list and returns the first entry whose
 * encode_available is true in the registry.  This makes the chain fully
 * configurable without changes to this module.
 *
 * HW vs SW PREFERENCE
 * -------------------
 * If HW_FIRST is set in cfb_options_t, cfb_select_best() first looks for
 * a codec with hw_preferred=true.  If none found, it repeats the scan
 * accepting any available codec (hardware or software).
 *
 * THREAD-SAFETY
 * -------------
 * NOT thread-safe.  cfb_select_best() reads the registry; see creg_probe_all()
 * for thread-safety notes.
 */

#ifndef ROOTSTREAM_CODEC_FALLBACK_H
#define ROOTSTREAM_CODEC_FALLBACK_H

#include <stdbool.h>
#include <stdint.h>

#include "codec_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Built-in fallback chain definitions ─────────────────────────── */

/** Maximum number of codecs in a fallback chain */
#define CFB_MAX_CHAIN 16

/** Option flags for cfb_select_best() */
#define CFB_OPT_NONE 0x00
#define CFB_OPT_HW_FIRST 0x01 /**< Prefer HW-accelerated codec if available */
#define CFB_OPT_SW_ONLY 0x02  /**< Force software-only (testing / debugging) */

/** Result of a fallback selection */
typedef struct {
    uint8_t codec_id;   /**< Selected CREG_VCODEC_* codec                */
    bool hw_available;  /**< HW acceleration is available for the codec  */
    bool is_fallback;   /**< true if the preferred codec was not available */
    int chain_position; /**< Zero-based index in the fallback chain       */
} cfb_result_t;

/* ── Pre-defined chains (convenience) ────────────────────────────── */

/**
 * cfb_chain_quality — quality-first chain (best compression first).
 * Order: AV2 → VVC → AV1 → VP9 → H.265 → H.264
 * Use when maximum compression efficiency is the priority.
 */
extern const uint8_t cfb_chain_quality[];
extern const int cfb_chain_quality_len;

/**
 * cfb_chain_compat — compatibility-first chain (widest device support).
 * Order: H.264 → H.265 → VP9 → AV1 → VVC
 * Use for streaming to older devices or when decoder support is uncertain.
 */
extern const uint8_t cfb_chain_compat[];
extern const int cfb_chain_compat_len;

/**
 * cfb_chain_modern — balanced chain (modern devices, good quality).
 * Order: AV1 → VP9 → H.265 → H.264
 * Use for most streaming sessions — widely supported, good quality.
 */
extern const uint8_t cfb_chain_modern[];
extern const int cfb_chain_modern_len;

/**
 * cfb_chain_discord — libdave-first chain.
 * Order: libdave → AV1 → VP9 → H.265 → H.264
 * Use when the remote endpoint is a Discord-compatible receiver.
 */
extern const uint8_t cfb_chain_discord[];
extern const int cfb_chain_discord_len;

/* ── Selection function ───────────────────────────────────────────── */

/**
 * cfb_select_best — walk @chain and return the first available codec.
 *
 * @param r             Codec registry (must have creg_probe_all() called)
 * @param preferred     CREG_VCODEC_* ID the caller wants (may be unavailable)
 * @param chain         Ordered array of CREG_VCODEC_* fallback IDs
 * @param chain_len     Length of @chain
 * @param options       CFB_OPT_* flags
 * @param result        Output: selected codec info (always filled)
 *
 * @return  CREG_VCODEC_* ID of selected codec, or CREG_VCODEC_H264 if
 *          absolutely nothing is available (H.264 software is always compiled in)
 */
uint8_t cfb_select_best(const creg_registry_t *r, uint8_t preferred, const uint8_t *chain,
                        int chain_len, uint8_t options, cfb_result_t *result);

/**
 * cfb_select_for_session — convenience wrapper that picks the appropriate
 * pre-defined chain based on @preferred codec and selects the best.
 *
 * Logic:
 *   - If @preferred == CREG_VCODEC_LIBDAVE → use cfb_chain_discord
 *   - If @preferred >= CREG_VCODEC_VVC    → use cfb_chain_quality
 *   - Otherwise                           → use cfb_chain_modern
 *
 * @param r         Registry
 * @param preferred Requested codec ID
 * @param options   CFB_OPT_* flags
 * @param result    Output: selected codec info
 * @return          Selected CREG_VCODEC_* ID
 */
uint8_t cfb_select_for_session(const creg_registry_t *r, uint8_t preferred, uint8_t options,
                               cfb_result_t *result);

/**
 * cfb_result_codec_name — return the human-readable name for the result.
 *
 * @param r   Registry
 * @param res cfb_result_t from cfb_select_best()
 * @return    Static string (never NULL)
 */
const char *cfb_result_codec_name(const creg_registry_t *r, const cfb_result_t *res);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CODEC_FALLBACK_H */
