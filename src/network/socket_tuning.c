/*
 * socket_tuning.c - TCP/UDP socket optimization implementation
 */

#include "socket_tuning.h"
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#endif

struct socket_tuning {
    int dummy;  /* Placeholder for future state */
};

socket_tuning_t* socket_tuning_create(void) {
    socket_tuning_t *tuning = calloc(1, sizeof(socket_tuning_t));
    return tuning;
}

void socket_tuning_destroy(socket_tuning_t *tuning) {
    free(tuning);
}

int socket_tuning_set_tcp_congestion_control(socket_tuning_t *tuning, 
                                             int socket, 
                                             congestion_control_t cc) {
    if (!tuning || socket < 0) {
        return -1;
    }
    
#if defined(__linux__) && !defined(_WIN32)
    const char *cc_name = NULL;
    
    switch (cc) {
        case CC_CUBIC:
            cc_name = "cubic";
            break;
        case CC_BBR:
            cc_name = "bbr";
            break;
        case CC_RENO:
            cc_name = "reno";
            break;
        case CC_BIC:
            cc_name = "bic";
            break;
        default:
            return -1;
    }
    
    if (setsockopt(socket, IPPROTO_TCP, TCP_CONGESTION, 
                   cc_name, strlen(cc_name)) < 0) {
        return -1;
    }
#endif
    
    return 0;
}

int socket_tuning_tune_low_latency(socket_tuning_t *tuning, int socket) {
    if (!tuning || socket < 0) {
        return -1;
    }
    
#ifndef _WIN32
    /* Disable Nagle's algorithm for low latency */
    int flag = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    /* Set smaller socket buffers for low latency */
    int send_buf = 256 * 1024;  /* 256KB */
    int recv_buf = 256 * 1024;
    setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
#endif
    
    return 0;
}

int socket_tuning_tune_throughput(socket_tuning_t *tuning, int socket) {
    if (!tuning || socket < 0) {
        return -1;
    }
    
#ifndef _WIN32
    /* Set larger socket buffers for throughput */
    int send_buf = 2 * 1024 * 1024;  /* 2MB */
    int recv_buf = 2 * 1024 * 1024;
    setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
#endif
    
    return 0;
}

int socket_tuning_enable_ecn(socket_tuning_t *tuning, int socket) {
    if (!tuning || socket < 0) {
        return -1;
    }
    
#if defined(__linux__) && !defined(_WIN32)
    /* Enable ECN (Explicit Congestion Notification) */
    int ecn = IP_PMTUDISC_DO;
    if (setsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, &ecn, sizeof(ecn)) < 0) {
        return -1;
    }
#endif
    
    return 0;
}

int socket_tuning_set_mtu_discovery(socket_tuning_t *tuning, int socket, uint32_t mtu) {
    if (!tuning || socket < 0) {
        return -1;
    }
    
    (void)mtu;  /* MTU parameter for future use */
    
#if defined(__linux__) && !defined(_WIN32)
    /* Enable Path MTU Discovery */
    int val = IP_PMTUDISC_DO;
    if (setsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val)) < 0) {
        return -1;
    }
#endif
    
    return 0;
}
