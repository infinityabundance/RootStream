/*
 * codec_registry.c — Central codec capability registry implementation
 *
 * DESIGN NOTES
 * ------------
 * The registry is a fixed-size array of creg_entry_t indexed by codec_id.
 * Since CREG_VCODEC_MAX = 8 the array is tiny; we use direct index access
 * rather than a hash map.  This is intentional:
 *
 *   - O(1) lookup by codec_id — critical because the hot path (encoder
 *     selection at session start) calls creg_lookup() synchronously.
 *   - No dynamic allocation per entry — the entire registry is one calloc().
 *   - Simple: no hash collisions, no tree rebalancing, no surprises.
 *
 * STARTUP SEQUENCE
 * ----------------
 * 1. Application calls creg_create().
 * 2. Each codec module (av1_encoder.c, vp9_encoder.c, …) calls
 *    creg_register() with its compile-time capability flags set.
 *    Alternatively, creg_register_all_defaults() does all of this in one call.
 * 3. Application calls creg_probe_all() — each codec's probe_fn() is
 *    invoked to verify that the runtime libraries / hardware are actually
 *    present (not just compiled in).
 * 4. The session-setup path calls creg_encode_available() /
 *    creg_hw_preferred() to select a codec and backend.
 * 5. The fallback chain (codec_fallback.c) iterates the registry in
 *    priority order to find the best available option.
 *
 * DEFAULT REGISTRATIONS (creg_register_all_defaults)
 * ---------------------------------------------------
 * The following entries are always registered, even if the corresponding
 * HAVE_* flags are absent — they just report encode/decode_available = false
 * so the fallback chain can skip them gracefully:
 *
 *   CREG_VCODEC_H264    (always: libx264 software fallback)
 *   CREG_VCODEC_H265    (if HAVE_X265 or HAVE_HEVC_VAAPI)
 *   CREG_VCODEC_AV1     (if HAVE_LIBAOM or HAVE_SVT_AV1 or HAVE_AV1_VAAPI)
 *   CREG_VCODEC_VP9     (if HAVE_LIBVPX or HAVE_VP9_VAAPI)
 *   CREG_VCODEC_VVC     (if HAVE_VVENC)
 *   CREG_VCODEC_AV2     (always: stub, always unavailable)
 *   CREG_VCODEC_LIBDAVE (if HAVE_LIBDAVE)
 */

#include "codec_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Internal struct ──────────────────────────────────────────────── */

struct creg_registry_s {
    /* Indexed by codec_id — element 0 = RAW, 1 = H264, …, 7 = LIBDAVE.
     * in_use[i] = true means slot i has a registered entry. */
    creg_entry_t entries[CREG_VCODEC_MAX];
    bool         in_use[CREG_VCODEC_MAX];
    int          count;  /* number of registered entries */
};

/* ── Forward declarations for default probe stubs ─────────────────── */

/* These weak no-op probes are used when a codec module is not compiled in.
 * Each real codec module (av1_encoder.c etc.) provides a strong override
 * via creg_register() at init time, so the actual probe logic lives there. */
static bool probe_always_false(uint8_t codec_id) {
    (void)codec_id;
    return false;  /* stub: codec not compiled in or not yet implemented */
}

static bool probe_always_true(uint8_t codec_id) {
    /* RAW pass-through is always available — no library required. */
    (void)codec_id;
    return true;
}

/* ── Lifecycle ────────────────────────────────────────────────────── */

creg_registry_t *creg_create(void) {
    /* calloc zero-initialises: all in_use=false, count=0 */
    return calloc(1, sizeof(creg_registry_t));
}

void creg_destroy(creg_registry_t *r) {
    free(r);
}

/* ── Registration ─────────────────────────────────────────────────── */

int creg_register(creg_registry_t *r, const creg_entry_t *entry) {
    if (!r || !entry) return -1;
    if (entry->codec_id >= CREG_VCODEC_MAX) return -1;  /* out of range */

    /* Overwrite any existing entry for this codec_id (idempotent) */
    r->entries[entry->codec_id] = *entry;

    /* Track count accurately */
    if (!r->in_use[entry->codec_id]) {
        r->in_use[entry->codec_id] = true;
        r->count++;
    }
    return 0;
}

/* ── Query ────────────────────────────────────────────────────────── */

const creg_entry_t *creg_lookup(const creg_registry_t *r, uint8_t codec_id) {
    if (!r || codec_id >= CREG_VCODEC_MAX) return NULL;
    if (!r->in_use[codec_id]) return NULL;
    return &r->entries[codec_id];
}

bool creg_encode_available(const creg_registry_t *r, uint8_t codec_id) {
    const creg_entry_t *e = creg_lookup(r, codec_id);
    return e ? e->encode_available : false;
}

bool creg_decode_available(const creg_registry_t *r, uint8_t codec_id) {
    const creg_entry_t *e = creg_lookup(r, codec_id);
    return e ? e->decode_available : false;
}

bool creg_hw_preferred(const creg_registry_t *r, uint8_t codec_id) {
    const creg_entry_t *e = creg_lookup(r, codec_id);
    return e ? e->hw_preferred : false;
}

int creg_count(const creg_registry_t *r) {
    return r ? r->count : 0;
}

/* ── Probing ──────────────────────────────────────────────────────── */

int creg_probe_all(creg_registry_t *r) {
    if (!r) return 0;
    int available = 0;

    for (int i = 0; i < CREG_VCODEC_MAX; i++) {
        if (!r->in_use[i]) continue;

        creg_entry_t *e = &r->entries[i];

        /* If a probe function is provided, call it and update availability.
         * If no probe function, trust the compile-time flags set in
         * creg_register_all_defaults(). */
        if (e->probe_fn) {
            bool avail = e->probe_fn((uint8_t)i);
            e->encode_available = avail && (e->encoder_backends != CREG_BACKEND_NONE);
            e->decode_available = avail && (e->decoder_backends != CREG_BACKEND_NONE);
        }

        /* hw_preferred: true if any hardware backend bit is set AND the
         * codec is available for encoding */
        if (e->encode_available) {
            uint8_t hw_mask = CREG_BACKEND_VAAPI | CREG_BACKEND_NVENC |
                              CREG_BACKEND_QSV   | CREG_BACKEND_VIDEOTB |
                              CREG_BACKEND_MEDIACODEC | CREG_BACKEND_V4L2;
            e->hw_preferred = (e->encoder_backends & hw_mask) != 0;
        }

        if (e->encode_available || e->decode_available) available++;
    }
    return available;
}

/* ── Default registrations ────────────────────────────────────────── */

int creg_register_all_defaults(creg_registry_t *r) {
    if (!r) return 0;
    int n = 0;

    /* ── RAW (pass-through) ──
     * Always available.  Used for debug / zero-copy local capture.
     * No library dependency. */
    {
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_RAW,
            .name             = "raw",
            .long_name        = "Uncompressed / pass-through",
            .encoder_backends = CREG_BACKEND_SW,
            .decoder_backends = CREG_BACKEND_SW,
            .encode_available = true,
            .decode_available = true,
            .hw_preferred     = false,
            .probe_fn         = probe_always_true,
        };
        creg_register(r, &e); n++;
    }

    /* ── H.264 / AVC ──
     * Baseline: libx264 software is always compiled in.
     * Hardware: VAAPI h264_vaapi, NVENC h264_nvenc, QSV h264_qsv.
     * This is the final fallback in all chains. */
    {
        uint8_t enc_backends = CREG_BACKEND_SW;
        uint8_t dec_backends = CREG_BACKEND_SW;
#ifdef HAVE_VAAPI
        enc_backends |= CREG_BACKEND_VAAPI;
        dec_backends |= CREG_BACKEND_VAAPI;
#endif
#ifdef HAVE_NVENC
        enc_backends |= CREG_BACKEND_NVENC;
#endif
#ifdef HAVE_QSV
        enc_backends |= CREG_BACKEND_QSV;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_H264,
            .name             = "h264",
            .long_name        = "H.264 / AVC (libx264 + hardware)",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = true,   /* libx264 always present at link time */
            .decode_available = true,
            .hw_preferred     = false,  /* updated by creg_probe_all() */
            .probe_fn         = NULL,   /* libx264 compile-time guarantee */
        };
        creg_register(r, &e); n++;
    }

    /* ── H.265 / HEVC ──
     * Software: libx265 (if HAVE_X265).
     * Hardware: hevc_vaapi, hevc_nvenc, hevc_qsv. */
    {
        uint8_t enc_backends = CREG_BACKEND_NONE;
        uint8_t dec_backends = CREG_BACKEND_NONE;
        bool    avail        = false;
#ifdef HAVE_X265
        enc_backends |= CREG_BACKEND_SW;
        dec_backends |= CREG_BACKEND_SW;
        avail = true;
#endif
#ifdef HAVE_VAAPI
        enc_backends |= CREG_BACKEND_VAAPI;
        dec_backends |= CREG_BACKEND_VAAPI;
        avail = true;
#endif
#ifdef HAVE_NVENC
        enc_backends |= CREG_BACKEND_NVENC;
        avail = true;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_H265,
            .name             = "h265",
            .long_name        = "H.265 / HEVC (libx265 + hardware)",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = avail,
            .decode_available = avail,
            .hw_preferred     = false,
            .probe_fn         = NULL,
        };
        creg_register(r, &e); n++;
    }

    /* ── AV1 ──
     * Software: libaom-av1 (HAVE_LIBAOM) or SVT-AV1 (HAVE_SVT_AV1).
     * Decoder:  dav1d (HAVE_DAV1D) or libaom decoder.
     * Hardware: av1_vaapi (Intel Arc, AMD RDNA3+), av1_nvenc (RTX 40xx). */
    {
        uint8_t enc_backends = CREG_BACKEND_NONE;
        uint8_t dec_backends = CREG_BACKEND_NONE;
        bool    enc_avail    = false;
        bool    dec_avail    = false;
#ifdef HAVE_LIBAOM
        enc_backends |= CREG_BACKEND_SW;
        dec_backends |= CREG_BACKEND_SW;
        enc_avail = dec_avail = true;
#endif
#ifdef HAVE_SVT_AV1
        enc_backends |= CREG_BACKEND_SW;
        enc_avail = true;
#endif
#ifdef HAVE_DAV1D
        dec_backends |= CREG_BACKEND_SW;
        dec_avail = true;
#endif
#ifdef HAVE_AV1_VAAPI
        enc_backends |= CREG_BACKEND_VAAPI;
        dec_backends |= CREG_BACKEND_VAAPI;
        enc_avail = dec_avail = true;
#endif
#ifdef HAVE_AV1_NVENC
        enc_backends |= CREG_BACKEND_NVENC;
        enc_avail = true;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_AV1,
            .name             = "av1",
            .long_name        = "AV1 (libaom / SVT-AV1 / dav1d / hardware)",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = enc_avail,
            .decode_available = dec_avail,
            .hw_preferred     = false,
            .probe_fn         = NULL,
        };
        creg_register(r, &e); n++;
    }

    /* ── VP9 ──
     * Software: libvpx-vp9 (HAVE_LIBVPX).
     * Hardware: vp9_vaapi (many Intel/AMD GPUs), vp9_nvenc (Pascal+). */
    {
        uint8_t enc_backends = CREG_BACKEND_NONE;
        uint8_t dec_backends = CREG_BACKEND_NONE;
        bool    avail        = false;
#ifdef HAVE_LIBVPX
        enc_backends |= CREG_BACKEND_SW;
        dec_backends |= CREG_BACKEND_SW;
        avail = true;
#endif
#ifdef HAVE_VP9_VAAPI
        enc_backends |= CREG_BACKEND_VAAPI;
        dec_backends |= CREG_BACKEND_VAAPI;
        avail = true;
#endif
#ifdef HAVE_VP9_NVENC
        enc_backends |= CREG_BACKEND_NVENC;
        avail = true;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_VP9,
            .name             = "vp9",
            .long_name        = "VP9 (libvpx + hardware VAAPI/NVENC)",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = avail,
            .decode_available = avail,
            .hw_preferred     = false,
            .probe_fn         = NULL,
        };
        creg_register(r, &e); n++;
    }

    /* ── H.266 / VVC ──
     * Software only at this time: VVenC encoder, VVdeC decoder.
     * Hardware support is emerging (2024+) but not yet widely available.
     * VVC is the highest-quality codec in this chain; ~30–50% better
     * compression than HEVC / ~50–80% better than H.264. */
    {
        uint8_t enc_backends = CREG_BACKEND_NONE;
        uint8_t dec_backends = CREG_BACKEND_NONE;
        bool    avail        = false;
#ifdef HAVE_VVENC
        enc_backends |= CREG_BACKEND_SW;
        dec_backends |= CREG_BACKEND_SW;
        avail = true;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_VVC,
            .name             = "vvc",
            .long_name        = "H.266 / VVC (VVenC + VVdeC)",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = avail,
            .decode_available = avail,
            .hw_preferred     = false,
            .probe_fn         = NULL,
        };
        creg_register(r, &e); n++;
    }

    /* ── AV2 ──
     * AV2 is not yet standardised (expected 2026+).  This entry is a
     * forward-compatibility stub.  It is always registered with
     * encode_available = false so the fallback chain skips it on systems
     * where no AV2 implementation exists yet. */
    {
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_AV2,
            .name             = "av2",
            .long_name        = "AV2 (future spec — stub)",
            .encoder_backends = CREG_BACKEND_NONE,
            .decoder_backends = CREG_BACKEND_NONE,
            .encode_available = false,
            .decode_available = false,
            .hw_preferred     = false,
            .probe_fn         = probe_always_false,
        };
        creg_register(r, &e); n++;
    }

    /* ── libdave (Discord) ──
     * Discord's libdave provides packetised media encoding optimised for
     * low-latency game streaming.  Enabled when HAVE_LIBDAVE is defined.
     * See docs/codecs/libdave_integration.md for integration notes. */
    {
        uint8_t enc_backends = CREG_BACKEND_NONE;
        uint8_t dec_backends = CREG_BACKEND_NONE;
        bool    avail        = false;
#ifdef HAVE_LIBDAVE
        enc_backends |= CREG_BACKEND_SW;
        dec_backends |= CREG_BACKEND_SW;
        avail = true;
#endif
        creg_entry_t e = {
            .codec_id         = CREG_VCODEC_LIBDAVE,
            .name             = "libdave",
            .long_name        = "Discord libdave packetised media",
            .encoder_backends = enc_backends,
            .decoder_backends = dec_backends,
            .encode_available = avail,
            .decode_available = avail,
            .hw_preferred     = false,
            .probe_fn         = NULL,
        };
        creg_register(r, &e); n++;
    }

    /* Run hardware probing for all registered codecs */
    creg_probe_all(r);

    return n;
}

/* ── Enumeration ──────────────────────────────────────────────────── */

void creg_foreach(const creg_registry_t *r,
                  bool (*fn)(const creg_entry_t *e, void *user),
                  void *user)
{
    if (!r || !fn) return;
    for (int i = 0; i < CREG_VCODEC_MAX; i++) {
        if (!r->in_use[i]) continue;
        if (!fn(&r->entries[i], user)) break;  /* false = stop early */
    }
}
