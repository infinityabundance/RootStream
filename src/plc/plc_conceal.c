/*
 * plc_conceal.c — Packet Loss Concealment strategy implementations
 */

#include "plc_conceal.h"

#include <string.h>
#include <math.h>

const char *plc_strategy_name(plc_strategy_t s) {
    switch (s) {
    case PLC_STRATEGY_ZERO:     return "zero";
    case PLC_STRATEGY_REPEAT:   return "repeat";
    case PLC_STRATEGY_FADE_OUT: return "fade_out";
    default:                     return "unknown";
    }
}

int plc_conceal(const plc_history_t *history,
                 plc_strategy_t       strategy,
                 int                  consecutive_losses,
                 float                fade_factor,
                 const plc_frame_t   *ref_frame,
                 plc_frame_t         *out) {
    if (!out) return -1;

    memset(out, 0, sizeof(*out));

    if (strategy == PLC_STRATEGY_ZERO) {
        /* Silence: copy metadata from history or ref_frame */
        plc_frame_t ref;
        if (!plc_history_is_empty(history)) {
            plc_history_get_last(history, &ref);
        } else if (ref_frame) {
            ref = *ref_frame;
        } else {
            return -1; /* No metadata available */
        }
        out->sample_rate = ref.sample_rate;
        out->channels    = ref.channels;
        out->num_samples = ref.num_samples;
        out->seq_num     = ref.seq_num + (uint32_t)consecutive_losses;
        out->timestamp_us = ref.timestamp_us;
        /* samples already zeroed */
        return 0;
    }

    /* REPEAT and FADE_OUT both need a last frame */
    plc_frame_t last;
    if (plc_history_is_empty(history)) {
        if (!ref_frame) return -1;
        last = *ref_frame;
    } else {
        plc_history_get_last(history, &last);
    }

    *out = last;
    out->seq_num     = last.seq_num + (uint32_t)consecutive_losses;
    out->timestamp_us = last.timestamp_us;

    if (strategy == PLC_STRATEGY_FADE_OUT) {
        /* Amplitude = fade_factor ^ consecutive_losses */
        float amp = 1.0f;
        for (int i = 0; i < consecutive_losses; i++) amp *= fade_factor;
        size_t n = (size_t)out->channels * (size_t)out->num_samples;
        for (size_t i = 0; i < n; i++) {
            float s = (float)out->samples[i] * amp;
            /* Clamp to int16 range */
            if (s >  32767.0f) s =  32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            out->samples[i] = (int16_t)s;
        }
    }
    /* REPEAT: *out already holds an unmodified copy of last */
    return 0;
}
