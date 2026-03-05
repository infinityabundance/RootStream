/*
 * plc_conceal.h — Packet Loss Concealment strategies
 *
 * When an audio packet is lost, the concealment engine synthesises a
 * substitute frame from the PLC history.  Three strategies are supported:
 *
 *   PLC_STRATEGY_ZERO      — fill with silence (zeros)
 *   PLC_STRATEGY_REPEAT    — repeat the last received frame exactly
 *   PLC_STRATEGY_FADE_OUT  — repeat the last frame with exponential
 *                             amplitude fade-out (preserves pitch,
 *                             reduces codec artefacts on resume)
 *
 * The concealer is stateless: each call produces one complete substitute
 * frame given a history context.
 *
 * Thread-safety: stateless functions — thread-safe.
 */

#ifndef ROOTSTREAM_PLC_CONCEAL_H
#define ROOTSTREAM_PLC_CONCEAL_H

#include "plc_frame.h"
#include "plc_history.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Concealment strategy */
typedef enum {
    PLC_STRATEGY_ZERO     = 0,  /**< Replace with silence */
    PLC_STRATEGY_REPEAT   = 1,  /**< Repeat last good frame */
    PLC_STRATEGY_FADE_OUT = 2,  /**< Repeat with amplitude fade-out */
} plc_strategy_t;

/** Fade-out factor per consecutive loss (0.0–1.0; default 0.9) */
#define PLC_FADE_FACTOR_DEFAULT  0.9f

/**
 * plc_conceal — synthesise one substitute frame for a lost packet
 *
 * The @consecutive_losses count is used by FADE_OUT to scale the
 * amplitude appropriately (level = fade_factor ^ consecutive_losses).
 *
 * @param history              Recent-frame history (may be empty)
 * @param strategy             Concealment strategy to apply
 * @param consecutive_losses   Number of consecutive losses so far (≥ 1)
 * @param fade_factor          Amplitude scale per loss (FADE_OUT only)
 * @param ref_frame            Reference frame supplying metadata when
 *                             history is empty (may be NULL if history
 *                             is non-empty)
 * @param out                  Output substitute frame
 * @return                     0 on success, -1 on error
 */
int plc_conceal(const plc_history_t *history,
                 plc_strategy_t       strategy,
                 int                  consecutive_losses,
                 float                fade_factor,
                 const plc_frame_t   *ref_frame,
                 plc_frame_t         *out);

/**
 * plc_strategy_name — return human-readable strategy name
 *
 * @param s  Strategy
 * @return   Static string (never NULL)
 */
const char *plc_strategy_name(plc_strategy_t s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLC_CONCEAL_H */
