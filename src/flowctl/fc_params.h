/*
 * fc_params.h — Flow Controller: tuning parameters
 *
 * Bundles the four knobs that govern a single flow-control channel:
 *   window_bytes   — maximum bytes in flight at any time
 *   send_budget    — initial send credit (bytes) per epoch
 *   recv_window    — advertised receive window size (bytes) sent to peer
 *   credit_step    — minimum bytes of credit granted per replenish call
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_FC_PARAMS_H
#define ROOTSTREAM_FC_PARAMS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t window_bytes; /**< Maximum bytes in flight */
    uint32_t send_budget;  /**< Initial send credit per epoch (bytes) */
    uint32_t recv_window;  /**< Receive window advertised to peer */
    uint32_t credit_step;  /**< Minimum credit increment per replenish */
} fc_params_t;

/**
 * fc_params_init — initialise parameter block with sane defaults
 *
 * @return 0 on success, -1 if p is NULL or any value is 0
 */
int fc_params_init(fc_params_t *p, uint32_t window_bytes, uint32_t send_budget,
                   uint32_t recv_window, uint32_t credit_step);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FC_PARAMS_H */
