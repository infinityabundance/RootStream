/*
 * loss_window.h — 64-slot sliding sequence-number receive/loss bitmask
 *
 * Tracks a sliding window of the most recent 64 packets by sequence
 * number.  Each packet is marked received or lost.  The window slides
 * forward as new (higher) sequence numbers arrive.
 *
 * Sequence numbers are uint16_t (wrapping).  The window is indexed as:
 *   slot = seq % LOSS_WIN_SIZE
 *
 * When a packet with seq > (base + LOSS_WIN_SIZE) arrives the window
 * slides forward, marking all skipped slots as lost.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_LOSS_WINDOW_H
#define ROOTSTREAM_LOSS_WINDOW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOSS_WIN_SIZE 64 /**< Sliding window width (packets) */

/** Sliding packet loss window */
typedef struct {
    uint64_t received_mask; /**< Bitmask: bit i=1 → slot i received */
    uint16_t base_seq;      /**< Sequence number of slot 0 */
    uint32_t total_seen;    /**< Packets marked (received or lost) */
    uint32_t total_lost;    /**< Packets marked lost */
    bool initialised;
} loss_window_t;

/**
 * lw_init — initialise window
 *
 * @param w  Window to initialise
 * @return   0 on success, -1 on NULL
 */
int lw_init(loss_window_t *w);

/**
 * lw_receive — mark a packet as received
 *
 * Also advances the window if seq is beyond the current range, marking
 * all skipped sequence numbers as lost.
 *
 * @param w    Window
 * @param seq  Received sequence number (uint16_t, wrapping)
 * @return     0 on success, -1 on NULL
 */
int lw_receive(loss_window_t *w, uint16_t seq);

/**
 * lw_loss_rate — compute instantaneous loss rate over the window
 *
 * @param w  Window
 * @return   Loss rate in [0, 1], or 0.0 if uninitialised
 */
double lw_loss_rate(const loss_window_t *w);

/**
 * lw_reset — clear all state
 *
 * @param w  Window
 */
void lw_reset(loss_window_t *w);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LOSS_WINDOW_H */
