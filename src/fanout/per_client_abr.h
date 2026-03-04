/*
 * per_client_abr.h — Per-client adaptive bitrate controller
 *
 * Wraps the global ABR model to maintain per-session state.  Each
 * client independently tracks its bandwidth estimate and drives its
 * own bitrate ramp-up/down schedule, allowing the fanout manager to
 * serve heterogeneous clients simultaneously.
 */

#ifndef ROOTSTREAM_PER_CLIENT_ABR_H
#define ROOTSTREAM_PER_CLIENT_ABR_H

#include "session_table.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ABR decision returned to the fanout manager */
typedef struct {
    uint32_t target_bitrate_kbps;  /**< Recommended encoding bitrate */
    bool     allow_upgrade;         /**< True if bitrate can increase */
    bool     force_keyframe;        /**< True if client needs resync */
} abr_decision_t;

/** Opaque per-client ABR controller */
typedef struct per_client_abr_s per_client_abr_t;

/**
 * per_client_abr_create — allocate ABR state for one client
 *
 * @param initial_bitrate_kbps  Starting bitrate (e.g. 1000)
 * @param max_bitrate_kbps      Upper limit negotiated at handshake
 * @return                      Non-NULL handle, or NULL on OOM
 */
per_client_abr_t *per_client_abr_create(uint32_t initial_bitrate_kbps,
                                         uint32_t max_bitrate_kbps);

/**
 * per_client_abr_destroy — free ABR state
 *
 * @param abr  ABR controller to destroy
 */
void per_client_abr_destroy(per_client_abr_t *abr);

/**
 * per_client_abr_update — feed new network measurements and get a decision
 *
 * Called once per feedback interval (typically 1–2 s).
 *
 * @param abr       ABR controller
 * @param rtt_ms    Latest RTT measurement
 * @param loss_rate Packet loss fraction 0.0–1.0
 * @param bw_kbps   Measured delivery bandwidth (kbps)
 * @return          Bitrate decision for the next interval
 */
abr_decision_t per_client_abr_update(per_client_abr_t *abr,
                                      uint32_t          rtt_ms,
                                      float             loss_rate,
                                      uint32_t          bw_kbps);

/**
 * per_client_abr_get_bitrate — return current bitrate target
 *
 * @param abr  ABR controller
 * @return     Current target bitrate in kbps
 */
uint32_t per_client_abr_get_bitrate(const per_client_abr_t *abr);

/**
 * per_client_abr_force_keyframe — signal that the client needs a keyframe
 *
 * @param abr  ABR controller
 */
void per_client_abr_force_keyframe(per_client_abr_t *abr);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PER_CLIENT_ABR_H */
