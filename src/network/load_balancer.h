/*
 * load_balancer.h - Multi-stream load balancing
 */

#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Load balancer handle */
typedef struct load_balancer load_balancer_t;

/* Create load balancer */
load_balancer_t* load_balancer_create(void);

/* Destroy load balancer */
void load_balancer_destroy(load_balancer_t *balancer);

/* Register stream */
int load_balancer_add_stream(load_balancer_t *balancer, 
                             uint32_t stream_id, 
                             uint32_t initial_bitrate_kbps);

/* Remove stream */
int load_balancer_remove_stream(load_balancer_t *balancer, uint32_t stream_id);

/* Allocate bandwidth to streams */
int load_balancer_allocate_bandwidth(load_balancer_t *balancer, 
                                     uint32_t total_bandwidth_mbps);

/* Get allocated bitrate for stream */
uint32_t load_balancer_get_stream_bitrate(load_balancer_t *balancer, 
                                          uint32_t stream_id);

/* Fair share algorithm */
int load_balancer_allocate_fair_share(load_balancer_t *balancer);

/* Get active stream count */
uint32_t load_balancer_get_stream_count(load_balancer_t *balancer);

#ifdef __cplusplus
}
#endif

#endif /* LOAD_BALANCER_H */
