/*
 * codec_fallback.c — Ordered codec fallback chain implementation
 *
 * FALLBACK CHAIN DESIGN
 * ---------------------
 * The fallback chains are simple const arrays of CREG_VCODEC_* IDs.
 * cfb_select_best() iterates the array in order and stops at the first
 * entry whose encode_available is true in the registry.
 *
 * Two-pass HW_FIRST algorithm:
 *   Pass 1: scan for a codec that is available AND hw_preferred.
 *   Pass 2: if pass 1 finds nothing, scan for any available codec.
 * This guarantees that a HW accelerated codec is always preferred over
 * software when CFB_OPT_HW_FIRST is set, without requiring the chains
 * themselves to know about HW vs SW.
 *
 * FALLBACK RESULT METADATA
 * ------------------------
 * cfb_result_t.is_fallback = true means the caller's preferred codec was
 * not available and a substitute was chosen.  Callers should log a warning
 * (visible in the HUD / web dashboard) whenever is_fallback is true so
 * the user understands why a different codec is being used.
 */

#include "codec_fallback.h"

#include <stdlib.h>
#include <string.h>

/* ── Built-in chain definitions ───────────────────────────────────── */

/* Quality-first: newest/most efficient codec first.
 * AV2 and VVC are listed first; they are always skipped on systems
 * that lack the libraries (encode_available=false), making this chain
 * degrade gracefully to AV1 → VP9 → H.265 → H.264. */
const uint8_t cfb_chain_quality[] = {
    CREG_VCODEC_AV2,  /* AV2: future, best compression (2026+) */
    CREG_VCODEC_VVC,  /* H.266/VVC: ~50% better than H.265      */
    CREG_VCODEC_AV1,  /* AV1: ~30% better than H.265, open      */
    CREG_VCODEC_VP9,  /* VP9: royalty-free, widely HW-supported  */
    CREG_VCODEC_H265, /* H.265: broadly available HW             */
    CREG_VCODEC_H264, /* H.264: universal fallback               */
};
const int cfb_chain_quality_len = (int)(sizeof(cfb_chain_quality) / sizeof(cfb_chain_quality[0]));

/* Compatibility-first: widest device support first.
 * Use when the remote client is an older device or unknown platform. */
const uint8_t cfb_chain_compat[] = {
    CREG_VCODEC_H264, /* H.264: supported by every device since 2010 */
    CREG_VCODEC_H265, /* H.265: supported by most devices since 2014 */
    CREG_VCODEC_VP9,  /* VP9: widely supported in browsers/Android   */
    CREG_VCODEC_AV1,  /* AV1: growing hardware support (2021+)       */
    CREG_VCODEC_VVC,  /* H.266/VVC: limited adoption yet             */
};
const int cfb_chain_compat_len = (int)(sizeof(cfb_chain_compat) / sizeof(cfb_chain_compat[0]));

/* Modern-balanced: good quality, wide hardware support. */
const uint8_t cfb_chain_modern[] = {
    CREG_VCODEC_AV1,  /* AV1: best quality with growing HW support   */
    CREG_VCODEC_VP9,  /* VP9: royalty-free, GPU accelerated          */
    CREG_VCODEC_H265, /* H.265: good HW support (NVENC, VAAPI, QSV)  */
    CREG_VCODEC_H264, /* H.264: universal baseline                   */
};
const int cfb_chain_modern_len = (int)(sizeof(cfb_chain_modern) / sizeof(cfb_chain_modern[0]));

/* Discord libdave-first chain. */
const uint8_t cfb_chain_discord[] = {
    CREG_VCODEC_LIBDAVE, /* Discord libdave: optimised for game streaming */
    CREG_VCODEC_AV1,     /* AV1 fallback                                  */
    CREG_VCODEC_VP9,     /* VP9 fallback                                  */
    CREG_VCODEC_H265,    /* H.265 fallback                                */
    CREG_VCODEC_H264,    /* Universal fallback                            */
};
const int cfb_chain_discord_len = (int)(sizeof(cfb_chain_discord) / sizeof(cfb_chain_discord[0]));

/* ── Internal helpers ─────────────────────────────────────────────── */

/* Scan one pass through the chain.
 * require_hw: if true, only accept entries with hw_preferred=true. */
static int scan_chain(const creg_registry_t *r, const uint8_t *chain, int chain_len,
                      bool require_hw, uint8_t options) {
    (void)options; /* reserved for SW_ONLY logic below */

    for (int i = 0; i < chain_len; i++) {
        uint8_t cid = chain[i];

        /* SW_ONLY flag: skip hardware-accelerated entries */
        if ((options & CFB_OPT_SW_ONLY) && creg_hw_preferred(r, cid))
            continue;

        if (!creg_encode_available(r, cid))
            continue;

        if (require_hw && !creg_hw_preferred(r, cid))
            continue;

        return i; /* found: return index in chain */
    }
    return -1; /* not found */
}

/* ── Public API ───────────────────────────────────────────────────── */

uint8_t cfb_select_best(const creg_registry_t *r, uint8_t preferred, const uint8_t *chain,
                        int chain_len, uint8_t options, cfb_result_t *result) {
    /* Initialise result to H.264 as the universal safety net */
    if (result) {
        result->codec_id = CREG_VCODEC_H264;
        result->hw_available = false;
        result->is_fallback = true;
        result->chain_position = -1;
    }

    if (!r || !chain || chain_len <= 0)
        return CREG_VCODEC_H264;

    /* Step 1: check if the preferred codec is immediately available.
     * If so, skip the fallback chain entirely. */
    if (creg_encode_available(r, preferred)) {
        bool hw = creg_hw_preferred(r, preferred);
        /* If HW_FIRST is requested but preferred has no HW, still use it
         * if nothing in the chain has HW either (handled after chain scan). */
        if (result) {
            result->codec_id = preferred;
            result->hw_available = hw;
            result->is_fallback = false;
            result->chain_position = 0;
        }
        /* Still apply SW_ONLY restriction on preferred codec */
        if ((options & CFB_OPT_SW_ONLY) && hw) {
            /* fall through to chain scan with SW_ONLY enforcement */
        } else {
            return preferred;
        }
    }

    /* Step 2: HW-first pass (only if HW_FIRST requested) */
    if (options & CFB_OPT_HW_FIRST) {
        int idx = scan_chain(r, chain, chain_len, true, options);
        if (idx >= 0) {
            uint8_t cid = chain[idx];
            if (result) {
                result->codec_id = cid;
                result->hw_available = true;
                result->is_fallback = (cid != preferred);
                result->chain_position = idx;
            }
            return cid;
        }
    }

    /* Step 3: any-available pass */
    int idx = scan_chain(r, chain, chain_len, false, options);
    if (idx >= 0) {
        uint8_t cid = chain[idx];
        if (result) {
            result->codec_id = cid;
            result->hw_available = creg_hw_preferred(r, cid);
            result->is_fallback = (cid != preferred);
            result->chain_position = idx;
        }
        return cid;
    }

    /* Step 4: absolute last resort — H.264 software.
     * libx264 is always compiled in, so this never fails. */
    if (result) {
        result->codec_id = CREG_VCODEC_H264;
        result->hw_available = false;
        result->is_fallback = true;
        result->chain_position = -1;
    }
    return CREG_VCODEC_H264;
}

uint8_t cfb_select_for_session(const creg_registry_t *r, uint8_t preferred, uint8_t options,
                               cfb_result_t *result) {
    /* Choose chain based on requested codec */
    if (preferred == CREG_VCODEC_LIBDAVE) {
        return cfb_select_best(r, preferred, cfb_chain_discord, cfb_chain_discord_len, options,
                               result);
    }
    if (preferred >= CREG_VCODEC_VVC) {
        return cfb_select_best(r, preferred, cfb_chain_quality, cfb_chain_quality_len, options,
                               result);
    }
    /* Default: modern balanced chain */
    return cfb_select_best(r, preferred, cfb_chain_modern, cfb_chain_modern_len, options, result);
}

const char *cfb_result_codec_name(const creg_registry_t *r, const cfb_result_t *res) {
    if (!res)
        return "unknown";
    const creg_entry_t *e = creg_lookup(r, res->codec_id);
    if (!e)
        return "unknown";
    return e->name;
}
