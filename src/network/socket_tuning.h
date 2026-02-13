/*
 * socket_tuning.h - TCP/UDP socket optimization
 */

#ifndef SOCKET_TUNING_H
#define SOCKET_TUNING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TCP congestion control algorithms */
typedef enum {
    CC_CUBIC,   /* Linux default, good for video */
    CC_BBR,     /* Bottleneck bandwidth and RTT (low latency) */
    CC_RENO,    /* Classic TCP Reno */
    CC_BIC,     /* Binary Increase Congestion */
} congestion_control_t;

/* Socket tuning handle */
typedef struct socket_tuning socket_tuning_t;

/* Create socket tuning manager */
socket_tuning_t* socket_tuning_create(void);

/* Destroy socket tuning manager */
void socket_tuning_destroy(socket_tuning_t *tuning);

/* Set TCP congestion control algorithm */
int socket_tuning_set_tcp_congestion_control(socket_tuning_t *tuning, 
                                             int socket, 
                                             congestion_control_t cc);

/* Tune socket for low latency */
int socket_tuning_tune_low_latency(socket_tuning_t *tuning, int socket);

/* Tune socket for throughput */
int socket_tuning_tune_throughput(socket_tuning_t *tuning, int socket);

/* Enable ECN (Explicit Congestion Notification) */
int socket_tuning_enable_ecn(socket_tuning_t *tuning, int socket);

/* Set MTU path discovery */
int socket_tuning_set_mtu_discovery(socket_tuning_t *tuning, int socket, uint32_t mtu);

#ifdef __cplusplus
}
#endif

#endif /* SOCKET_TUNING_H */
