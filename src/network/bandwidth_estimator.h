/*
 * bandwidth_estimator.h - Bandwidth estimation using AIMD algorithm
 */

#ifndef BANDWIDTH_ESTIMATOR_H
#define BANDWIDTH_ESTIMATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Congestion state for AIMD */
typedef enum {
    AIMD_SLOW_START,
    AIMD_CONGESTION_AVOIDANCE,
    AIMD_FAST_RECOVERY,
} aimd_state_t;

/* Bandwidth estimator handle */
typedef struct bandwidth_estimator bandwidth_estimator_t;

/* Create bandwidth estimator */
bandwidth_estimator_t* bandwidth_estimator_create(void);

/* Destroy bandwidth estimator */
void bandwidth_estimator_destroy(bandwidth_estimator_t *estimator);

/* Update delivery rate */
int bandwidth_estimator_update_delivery_rate(bandwidth_estimator_t *estimator,
                                             uint64_t delivered_bytes,
                                             uint64_t delivery_time_us);

/* Detect congestion */
bool bandwidth_estimator_detect_congestion(bandwidth_estimator_t *estimator,
                                          uint32_t rtt_ms, 
                                          float packet_loss_percent);

/* AIMD operations */
int bandwidth_estimator_aimd_increase(bandwidth_estimator_t *estimator);
int bandwidth_estimator_aimd_decrease(bandwidth_estimator_t *estimator);

/* Get estimated bandwidth */
uint32_t bandwidth_estimator_get_estimated_bandwidth_mbps(bandwidth_estimator_t *estimator);

/* Check if in slow start */
bool bandwidth_estimator_is_in_slow_start(bandwidth_estimator_t *estimator);

#ifdef __cplusplus
}
#endif

#endif /* BANDWIDTH_ESTIMATOR_H */
