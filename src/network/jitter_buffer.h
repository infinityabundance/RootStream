/*
 * jitter_buffer.h - Packet jitter buffer for video/audio
 * 
 * Buffers packets to smooth out network jitter
 */

#ifndef JITTER_BUFFER_H
#define JITTER_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Jitter buffer handle */
typedef struct jitter_buffer jitter_buffer_t;

/* Create jitter buffer */
jitter_buffer_t* jitter_buffer_create(uint32_t target_delay_ms);

/* Destroy jitter buffer */
void jitter_buffer_destroy(jitter_buffer_t *buffer);

/* Insert packet into buffer */
int jitter_buffer_insert_packet(jitter_buffer_t *buffer,
                                const uint8_t *data,
                                size_t size,
                                uint32_t sequence,
                                uint64_t rtp_timestamp,
                                bool is_keyframe);

/* Extract next playable packet */
int jitter_buffer_extract_packet(jitter_buffer_t *buffer,
                                 uint8_t **data,
                                 size_t *size,
                                 uint32_t *sequence,
                                 bool *is_keyframe);

/* Update target delay based on network conditions */
int jitter_buffer_update_target_delay(jitter_buffer_t *buffer,
                                      uint32_t rtt_ms,
                                      uint32_t jitter_ms);

/* Statistics */
uint32_t jitter_buffer_get_delay_ms(jitter_buffer_t *buffer);
uint32_t jitter_buffer_get_packet_count(jitter_buffer_t *buffer);
float jitter_buffer_get_loss_rate(jitter_buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* JITTER_BUFFER_H */
