/*
 * network.c - Low-latency UDP streaming protocol
 * 
 * Custom protocol designed for game streaming:
 * - UDP for minimal latency
 * - Simple forward error correction
 * - Small packet size (MTU-friendly)
 * - No retransmission (better to drop than delay)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define PACKET_MAGIC 0x524F4F54  /* "ROOT" */

/*
 * Simple checksum for packet validation
 */
static uint16_t calc_checksum(const void *data, size_t len) {
    const uint8_t *bytes = data;
    uint32_t sum = 0;
    
    for (size_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    
    return (uint16_t)((sum & 0xFFFF) + (sum >> 16));
}

/*
 * Initialize network for host (server)
 */
int rootstream_net_init(rootstream_ctx_t *ctx, const char *bind_addr, uint16_t port) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    /* Create UDP socket */
    ctx->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock_fd < 0) {
        fprintf(stderr, "Cannot create socket: %s\n", strerror(errno));
        return -1;
    }

    /* Set socket options */
    int opt = 1;
    setsockopt(ctx->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Increase buffer sizes for high throughput */
    int buf_size = 2 * 1024 * 1024;  /* 2 MB */
    setsockopt(ctx->sock_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    setsockopt(ctx->sock_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));

    /* Set low latency options */
    int tos = 0x10;  /* IPTOS_LOWDELAY */
    setsockopt(ctx->sock_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

    /* Bind to address */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (bind_addr && strcmp(bind_addr, "0.0.0.0") != 0) {
        inet_pton(AF_INET, bind_addr, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(ctx->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(ctx->sock_fd);
        fprintf(stderr, "Cannot bind socket: %s\n", strerror(errno));
        return -1;
    }

    ctx->sequence = 0;
    
    printf("✓ Network initialized on %s:%d (UDP)\n", 
           bind_addr ? bind_addr : "0.0.0.0", port);

    return 0;
}

/*
 * Send a packet
 */
int rootstream_net_send(rootstream_ctx_t *ctx, uint8_t type, const void *data, size_t size) {
    if (!ctx || !data || size == 0) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    /* Check if we need to fragment */
    if (size > MAX_PACKET_SIZE - sizeof(packet_header_t)) {
        /* Fragment into multiple packets */
        size_t remaining = size;
        const uint8_t *ptr = data;
        
        while (remaining > 0) {
            size_t chunk_size = (remaining > MAX_PACKET_SIZE - sizeof(packet_header_t)) 
                               ? MAX_PACKET_SIZE - sizeof(packet_header_t) 
                               : remaining;
            
            if (rootstream_net_send(ctx, type, ptr, chunk_size) < 0)
                return -1;
            
            ptr += chunk_size;
            remaining -= chunk_size;
        }
        
        return 0;
    }

    /* Build packet */
    uint8_t packet[MAX_PACKET_SIZE];
    packet_header_t *hdr = (packet_header_t*)packet;
    
    hdr->magic = PACKET_MAGIC;
    hdr->version = 1;
    hdr->type = type;
    hdr->sequence = ctx->sequence++;
    hdr->timestamp = (uint32_t)(time(NULL) & 0xFFFFFFFF);
    hdr->payload_size = size;
    
    /* Copy payload */
    memcpy(packet + sizeof(packet_header_t), data, size);
    
    /* Calculate checksum */
    hdr->checksum = calc_checksum(packet + sizeof(packet_header_t), size);

    /* Send packet */
    ssize_t sent;
    if (ctx->peer_addr.ss_family != 0) {
        /* Client mode - send to peer */
        sent = sendto(ctx->sock_fd, packet, sizeof(packet_header_t) + size, 0,
                     (struct sockaddr*)&ctx->peer_addr, sizeof(struct sockaddr_in));
    } else {
        /* Host mode - send to last received address (simplified) */
        /* Real implementation would track multiple clients */
        return -1;
    }

    if (sent < 0) {
        fprintf(stderr, "Send failed: %s\n", strerror(errno));
        return -1;
    }

    ctx->bytes_sent += sent;
    return sent;
}

/*
 * Receive a packet with timeout
 */
int rootstream_net_recv(rootstream_ctx_t *ctx, void *buf, size_t size, int timeout_ms) {
    if (!ctx || !buf) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    /* Poll for data */
    struct pollfd pfd = {
        .fd = ctx->sock_fd,
        .events = POLLIN
    };

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        fprintf(stderr, "Poll failed: %s\n", strerror(errno));
        return -1;
    }
    
    if (ret == 0) {
        /* Timeout */
        return 0;
    }

    /* Receive packet */
    uint8_t packet[MAX_PACKET_SIZE];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    
    ssize_t recv_size = recvfrom(ctx->sock_fd, packet, sizeof(packet), 0,
                                (struct sockaddr*)&from, &fromlen);
    
    if (recv_size < (ssize_t)sizeof(packet_header_t)) {
        fprintf(stderr, "Received packet too small\n");
        return -1;
    }

    /* Store peer address for replies */
    memcpy(&ctx->peer_addr, &from, sizeof(from));

    /* Validate packet */
    packet_header_t *hdr = (packet_header_t*)packet;
    
    if (hdr->magic != PACKET_MAGIC) {
        fprintf(stderr, "Invalid packet magic\n");
        return -1;
    }

    if (hdr->version != 1) {
        fprintf(stderr, "Unsupported protocol version\n");
        return -1;
    }

    /* Verify checksum */
    uint16_t calc_csum = calc_checksum(packet + sizeof(packet_header_t), 
                                       hdr->payload_size);
    if (calc_csum != hdr->checksum) {
        fprintf(stderr, "Checksum mismatch (packet corrupted)\n");
        return -1;
    }

    /* Copy payload */
    size_t copy_size = (hdr->payload_size < size) ? hdr->payload_size : size;
    memcpy(buf, packet + sizeof(packet_header_t), copy_size);

    return copy_size;
}
