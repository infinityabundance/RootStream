/*
 * qos_manager.h - Quality of Service Traffic Prioritization
 * 
 * Classifies and prioritizes network packets
 */

#ifndef QOS_MANAGER_H
#define QOS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Packet priority levels */
typedef enum {
    PRIORITY_LOW = 0,       /* Control packets */
    PRIORITY_MEDIUM = 1,    /* Audio */
    PRIORITY_HIGH = 2,      /* Video P-frames */
    PRIORITY_CRITICAL = 3,  /* Video keyframes */
} packet_priority_t;

/* QoS manager handle */
typedef struct qos_manager qos_manager_t;

/* Create QoS manager */
qos_manager_t* qos_manager_create(void);

/* Destroy QoS manager */
void qos_manager_destroy(qos_manager_t *manager);

/* Register traffic class */
int qos_manager_register_traffic_class(qos_manager_t *manager,
                                       const char *name,
                                       packet_priority_t priority,
                                       uint32_t max_rate_kbps);

/* Classify packet */
packet_priority_t qos_manager_classify_packet(qos_manager_t *manager,
                                              const uint8_t *packet_data,
                                              size_t packet_len);

/* Set DSCP/TOS field on socket */
int qos_manager_set_dscp_field(qos_manager_t *manager, int socket, uint8_t dscp);

/* Check if packet should be dropped (when congested) */
bool qos_manager_should_drop_packet(qos_manager_t *manager,
                                    packet_priority_t priority,
                                    size_t queue_depth);

/* Statistics */
uint32_t qos_manager_get_packets_dropped(qos_manager_t *manager, 
                                         packet_priority_t priority);
uint32_t qos_manager_get_queue_depth(qos_manager_t *manager, 
                                     packet_priority_t priority);

#ifdef __cplusplus
}
#endif

#endif /* QOS_MANAGER_H */
