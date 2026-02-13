/*
 * loss_recovery.c - Packet loss recovery implementation
 */

#include "loss_recovery.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_NACK_QUEUE 100
#define MAX_RETRANSMIT_COUNT 3

typedef struct {
    uint32_t lost_sequence;
    uint64_t lost_time_us;
    uint32_t retransmit_count;
} nack_entry_t;

struct loss_recovery {
    recovery_strategy_t strategy;
    
    nack_entry_t nack_queue[MAX_NACK_QUEUE];
    uint32_t nack_count;
    
    uint32_t total_retransmits;
    uint32_t total_fec_recoveries;
    
    pthread_mutex_t lock;
};

loss_recovery_t* loss_recovery_create(recovery_strategy_t strategy) {
    loss_recovery_t *recovery = calloc(1, sizeof(loss_recovery_t));
    if (!recovery) {
        return NULL;
    }
    
    pthread_mutex_init(&recovery->lock, NULL);
    recovery->strategy = strategy;
    
    return recovery;
}

void loss_recovery_destroy(loss_recovery_t *recovery) {
    if (!recovery) {
        return;
    }
    
    pthread_mutex_destroy(&recovery->lock);
    free(recovery);
}

int loss_recovery_request_retransmit(loss_recovery_t *recovery, uint32_t lost_sequence) {
    if (!recovery) {
        return -1;
    }
    
    pthread_mutex_lock(&recovery->lock);
    
    /* Check if already in queue */
    for (uint32_t i = 0; i < recovery->nack_count; i++) {
        if (recovery->nack_queue[i].lost_sequence == lost_sequence) {
            pthread_mutex_unlock(&recovery->lock);
            return 0;  /* Already queued */
        }
    }
    
    /* Add to queue if space available */
    if (recovery->nack_count < MAX_NACK_QUEUE) {
        nack_entry_t *entry = &recovery->nack_queue[recovery->nack_count++];
        entry->lost_sequence = lost_sequence;
        entry->lost_time_us = 0;  /* Would get from clock */
        entry->retransmit_count = 0;
    }
    
    pthread_mutex_unlock(&recovery->lock);
    return 0;
}

int loss_recovery_process_nack_queue(loss_recovery_t *recovery) {
    if (!recovery) {
        return -1;
    }
    
    pthread_mutex_lock(&recovery->lock);
    
    /* Process NACK queue - send retransmit requests */
    uint32_t processed = 0;
    for (uint32_t i = 0; i < recovery->nack_count; i++) {
        nack_entry_t *entry = &recovery->nack_queue[i];
        
        if (entry->retransmit_count < MAX_RETRANSMIT_COUNT) {
            /* Would send retransmit request here */
            entry->retransmit_count++;
            recovery->total_retransmits++;
        } else {
            /* Give up after max retransmits */
            /* Remove from queue by shifting remaining entries */
            for (uint32_t j = i; j < recovery->nack_count - 1; j++) {
                recovery->nack_queue[j] = recovery->nack_queue[j + 1];
            }
            recovery->nack_count--;
            i--;  /* Re-check this index */
        }
        processed++;
    }
    
    pthread_mutex_unlock(&recovery->lock);
    return processed;
}

int loss_recovery_encode_fec_group(loss_recovery_t *recovery,
                                   const uint8_t **data_packets,
                                   size_t packet_size,
                                   uint8_t packet_count,
                                   uint8_t *parity_packet) {
    if (!recovery || !data_packets || !parity_packet || packet_count == 0) {
        return -1;
    }
    
    /* Simple XOR-based FEC */
    memset(parity_packet, 0, packet_size);
    
    for (uint8_t i = 0; i < packet_count; i++) {
        if (data_packets[i]) {
            for (size_t j = 0; j < packet_size; j++) {
                parity_packet[j] ^= data_packets[i][j];
            }
        }
    }
    
    return 0;
}

int loss_recovery_decode_fec_group(loss_recovery_t *recovery,
                                   const uint8_t **received_packets,
                                   const bool *packet_present,
                                   uint8_t packet_count,
                                   size_t packet_size,
                                   uint8_t *recovered_packet) {
    if (!recovery || !received_packets || !packet_present || !recovered_packet) {
        return -1;
    }
    
    pthread_mutex_lock(&recovery->lock);
    
    /* Count missing packets */
    uint32_t missing_count = 0;
    int missing_idx = -1;
    
    for (uint8_t i = 0; i < packet_count; i++) {
        if (!packet_present[i]) {
            missing_count++;
            missing_idx = i;
        }
    }
    
    /* Can only recover if exactly 1 packet is missing */
    if (missing_count != 1 || missing_idx < 0) {
        pthread_mutex_unlock(&recovery->lock);
        return -1;
    }
    
    /* XOR all received packets to recover missing one */
    memset(recovered_packet, 0, packet_size);
    
    for (uint8_t i = 0; i < packet_count; i++) {
        if (packet_present[i] && received_packets[i]) {
            for (size_t j = 0; j < packet_size; j++) {
                recovered_packet[j] ^= received_packets[i][j];
            }
        }
    }
    
    recovery->total_fec_recoveries++;
    
    pthread_mutex_unlock(&recovery->lock);
    return 0;
}

int loss_recovery_update_strategy(loss_recovery_t *recovery, 
                                  float packet_loss_percent) {
    if (!recovery) {
        return -1;
    }
    
    pthread_mutex_lock(&recovery->lock);
    
    /* Adaptive strategy selection based on packet loss */
    if (packet_loss_percent < 1.0f) {
        recovery->strategy = RECOVERY_NACK_ONLY;
    } else if (packet_loss_percent < 5.0f) {
        recovery->strategy = RECOVERY_HYBRID;
    } else {
        recovery->strategy = RECOVERY_FEC_XOR;
    }
    
    pthread_mutex_unlock(&recovery->lock);
    return 0;
}

uint32_t loss_recovery_get_retransmits(loss_recovery_t *recovery) {
    if (!recovery) {
        return 0;
    }
    
    pthread_mutex_lock(&recovery->lock);
    uint32_t count = recovery->total_retransmits;
    pthread_mutex_unlock(&recovery->lock);
    
    return count;
}

uint32_t loss_recovery_get_fec_recoveries(loss_recovery_t *recovery) {
    if (!recovery) {
        return 0;
    }
    
    pthread_mutex_lock(&recovery->lock);
    uint32_t count = recovery->total_fec_recoveries;
    pthread_mutex_unlock(&recovery->lock);
    
    return count;
}
