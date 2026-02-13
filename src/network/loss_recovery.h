/*
 * loss_recovery.h - Packet loss recovery with NACK and FEC
 */

#ifndef LOSS_RECOVERY_H
#define LOSS_RECOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Recovery strategy */
typedef enum {
    RECOVERY_NACK_ONLY,     /* Negative acknowledgments only */
    RECOVERY_FEC_XOR,       /* Simple XOR parity FEC */
    RECOVERY_HYBRID,        /* NACK + FEC */
} recovery_strategy_t;

/* Loss recovery handle */
typedef struct loss_recovery loss_recovery_t;

/* Create loss recovery manager */
loss_recovery_t* loss_recovery_create(recovery_strategy_t strategy);

/* Destroy loss recovery manager */
void loss_recovery_destroy(loss_recovery_t *recovery);

/* Request retransmission of lost packet */
int loss_recovery_request_retransmit(loss_recovery_t *recovery, uint32_t lost_sequence);

/* Process NACK queue (called periodically) */
int loss_recovery_process_nack_queue(loss_recovery_t *recovery);

/* FEC: Encode group of packets with parity */
int loss_recovery_encode_fec_group(loss_recovery_t *recovery,
                                   const uint8_t **data_packets,
                                   size_t packet_size,
                                   uint8_t packet_count,
                                   uint8_t *parity_packet);

/* FEC: Decode and recover lost packet */
int loss_recovery_decode_fec_group(loss_recovery_t *recovery,
                                   const uint8_t **received_packets,
                                   const bool *packet_present,
                                   uint8_t packet_count,
                                   size_t packet_size,
                                   uint8_t *recovered_packet);

/* Update recovery strategy based on network conditions */
int loss_recovery_update_strategy(loss_recovery_t *recovery, 
                                  float packet_loss_percent);

/* Get statistics */
uint32_t loss_recovery_get_retransmits(loss_recovery_t *recovery);
uint32_t loss_recovery_get_fec_recoveries(loss_recovery_t *recovery);

#ifdef __cplusplus
}
#endif

#endif /* LOSS_RECOVERY_H */
