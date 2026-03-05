/*
 * probe_scheduler.h — Probe send scheduler
 *
 * The scheduler decides WHEN and HOW MANY probe packets to send.
 * It models two knobs:
 *   - interval_us: minimum µs between bursts
 *   - burst_size:  number of packets per burst
 *
 * The caller drives the scheduler with a monotonic clock (µs) and
 * receives a decision: "send a packet now" or "not yet".
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PROBE_SCHEDULER_H
#define ROOTSTREAM_PROBE_SCHEDULER_H

#include "probe_packet.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default probe interval: 200 ms */
#define PROBE_DEFAULT_INTERVAL_US  200000ULL
/** Default burst size: 3 packets */
#define PROBE_DEFAULT_BURST_SIZE   3

/** Scheduler decision */
typedef enum {
    PROBE_SCHED_WAIT = 0,   /**< Too early — do not send yet */
    PROBE_SCHED_SEND = 1,   /**< Fill and send a packet now */
} probe_sched_decision_t;

/** Opaque probe scheduler */
typedef struct probe_scheduler_s probe_scheduler_t;

/**
 * probe_scheduler_create — allocate scheduler
 *
 * @param interval_us  Minimum µs between burst starts
 * @param burst_size   Packets per burst (>= 1)
 * @return             Non-NULL handle, or NULL on error
 */
probe_scheduler_t *probe_scheduler_create(uint64_t interval_us,
                                            int      burst_size);

/**
 * probe_scheduler_destroy — free scheduler
 *
 * @param s  Scheduler to destroy
 */
void probe_scheduler_destroy(probe_scheduler_t *s);

/**
 * probe_scheduler_tick — advance scheduler clock
 *
 * Returns PROBE_SCHED_SEND and fills @pkt_out when a packet should be
 * sent, PROBE_SCHED_WAIT otherwise.
 *
 * The first call after creation always returns SEND (burst_seq=0).
 *
 * @param s        Scheduler
 * @param now_us   Current time in µs
 * @param pkt_out  Output packet to fill (only valid when SEND is returned)
 * @return         PROBE_SCHED_WAIT or PROBE_SCHED_SEND
 */
probe_sched_decision_t probe_scheduler_tick(probe_scheduler_t *s,
                                               uint64_t           now_us,
                                               probe_packet_t    *pkt_out);

/**
 * probe_scheduler_set_interval — update burst interval
 *
 * @param s           Scheduler
 * @param interval_us New interval in µs (> 0)
 * @return            0 on success, -1 on invalid args
 */
int probe_scheduler_set_interval(probe_scheduler_t *s, uint64_t interval_us);

/**
 * probe_scheduler_burst_count — total bursts initiated so far
 *
 * @param s  Scheduler
 * @return   Burst count
 */
uint64_t probe_scheduler_burst_count(const probe_scheduler_t *s);

/**
 * probe_scheduler_packet_count — total packets sent so far
 *
 * @param s  Scheduler
 * @return   Packet count
 */
uint64_t probe_scheduler_packet_count(const probe_scheduler_t *s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PROBE_SCHEDULER_H */
