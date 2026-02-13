/*
 * qos_manager.c - Quality of Service Traffic Prioritization implementation
 */

#include "qos_manager.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif

#define MAX_TRAFFIC_CLASSES 8
#define MAX_QUEUE_DEPTH 1000

/* DSCP values for different traffic types */
#define DSCP_EF  46   /* Expedited Forwarding - Video keyframes */
#define DSCP_AF41 34  /* Assured Forwarding - Video P-frames */
#define DSCP_AF31 26  /* Assured Forwarding - Audio */
#define DSCP_CS0  0   /* Default - Control */

typedef struct {
    char name[32];
    packet_priority_t priority;
    uint8_t dscp;
    uint32_t max_rate_kbps;
    uint32_t bucket_size_bytes;
    uint32_t packets_dropped;
    uint32_t queue_depth;
} traffic_class_t;

struct qos_manager {
    traffic_class_t classes[MAX_TRAFFIC_CLASSES];
    uint32_t class_count;
    pthread_mutex_t lock;
};

qos_manager_t* qos_manager_create(void) {
    qos_manager_t *manager = calloc(1, sizeof(qos_manager_t));
    if (!manager) {
        return NULL;
    }
    
    pthread_mutex_init(&manager->lock, NULL);
    
    /* Register default traffic classes */
    qos_manager_register_traffic_class(manager, "Control", PRIORITY_LOW, 100);
    qos_manager_register_traffic_class(manager, "Audio", PRIORITY_MEDIUM, 512);
    qos_manager_register_traffic_class(manager, "Video", PRIORITY_HIGH, 10000);
    qos_manager_register_traffic_class(manager, "Video Keyframe", PRIORITY_CRITICAL, 20000);
    
    return manager;
}

void qos_manager_destroy(qos_manager_t *manager) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_destroy(&manager->lock);
    free(manager);
}

int qos_manager_register_traffic_class(qos_manager_t *manager,
                                       const char *name,
                                       packet_priority_t priority,
                                       uint32_t max_rate_kbps) {
    if (!manager || manager->class_count >= MAX_TRAFFIC_CLASSES) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    traffic_class_t *tc = &manager->classes[manager->class_count];
    strncpy(tc->name, name, sizeof(tc->name) - 1);
    tc->priority = priority;
    tc->max_rate_kbps = max_rate_kbps;
    tc->bucket_size_bytes = max_rate_kbps * 125; /* Convert to bytes */
    
    /* Assign DSCP based on priority */
    switch (priority) {
        case PRIORITY_CRITICAL:
            tc->dscp = DSCP_EF;
            break;
        case PRIORITY_HIGH:
            tc->dscp = DSCP_AF41;
            break;
        case PRIORITY_MEDIUM:
            tc->dscp = DSCP_AF31;
            break;
        case PRIORITY_LOW:
        default:
            tc->dscp = DSCP_CS0;
            break;
    }
    
    manager->class_count++;
    
    pthread_mutex_unlock(&manager->lock);
    return 0;
}

packet_priority_t qos_manager_classify_packet(qos_manager_t *manager,
                                              const uint8_t *packet_data,
                                              size_t packet_len) {
    if (!manager || !packet_data || packet_len < 2) {
        return PRIORITY_LOW;
    }
    
    /* Simple classification based on packet type (first byte after header) */
    /* This would need to be adapted to the actual RootStream packet format */
    
    /* For now, use simple heuristics:
     * - Large packets (>10KB) are likely video keyframes
     * - Medium packets (1-10KB) are likely video P-frames
     * - Small packets (<1KB) are likely audio or control
     */
    
    if (packet_len > 10240) {
        return PRIORITY_CRITICAL;  /* Likely keyframe */
    } else if (packet_len > 1024) {
        return PRIORITY_HIGH;  /* Likely video P-frame */
    } else if (packet_len > 100) {
        return PRIORITY_MEDIUM;  /* Likely audio */
    } else {
        return PRIORITY_LOW;  /* Likely control */
    }
}

int qos_manager_set_dscp_field(qos_manager_t *manager, int socket, uint8_t dscp) {
    if (!manager || socket < 0) {
        return -1;
    }
    
#ifndef _WIN32
    /* Set IP TOS/DSCP field (Linux/Unix) */
    int tos = dscp << 2;  /* DSCP is in upper 6 bits */
    if (setsockopt(socket, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
        return -1;
    }
#endif
    
    return 0;
}

bool qos_manager_should_drop_packet(qos_manager_t *manager,
                                    packet_priority_t priority,
                                    size_t queue_depth) {
    if (!manager) {
        return false;
    }
    
    /* Drop policy based on priority and queue depth */
    uint32_t drop_threshold;
    
    switch (priority) {
        case PRIORITY_CRITICAL:
            drop_threshold = MAX_QUEUE_DEPTH;  /* Never drop */
            break;
        case PRIORITY_HIGH:
            drop_threshold = MAX_QUEUE_DEPTH * 3 / 4;  /* Drop at 75% */
            break;
        case PRIORITY_MEDIUM:
            drop_threshold = MAX_QUEUE_DEPTH / 2;  /* Drop at 50% */
            break;
        case PRIORITY_LOW:
        default:
            drop_threshold = MAX_QUEUE_DEPTH / 4;  /* Drop at 25% */
            break;
    }
    
    return queue_depth > drop_threshold;
}

uint32_t qos_manager_get_packets_dropped(qos_manager_t *manager, 
                                         packet_priority_t priority) {
    if (!manager) {
        return 0;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    uint32_t dropped = 0;
    for (uint32_t i = 0; i < manager->class_count; i++) {
        if (manager->classes[i].priority == priority) {
            dropped = manager->classes[i].packets_dropped;
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->lock);
    return dropped;
}

uint32_t qos_manager_get_queue_depth(qos_manager_t *manager, 
                                     packet_priority_t priority) {
    if (!manager) {
        return 0;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    uint32_t depth = 0;
    for (uint32_t i = 0; i < manager->class_count; i++) {
        if (manager->classes[i].priority == priority) {
            depth = manager->classes[i].queue_depth;
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->lock);
    return depth;
}
