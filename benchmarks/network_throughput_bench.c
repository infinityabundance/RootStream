/*
 * network_throughput_bench.c — Benchmark TCP loopback throughput
 *
 * Creates a loopback TCP connection (server thread + client thread),
 * transfers 10 MB of data in 64 KB chunks, and measures throughput (MB/s)
 * and round-trip latency (µs) using POSIX sockets and clock_gettime.
 *
 * Output format:
 *   BENCH tcp_loopback: throughput=X MB/s latency=Xus
 *
 * Exit: 0 if throughput >= 100 MB/s, 1 otherwise.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TRANSFER_BYTES  (10 * 1024 * 1024)   /* 10 MB */
#define CHUNK_SIZE      (64 * 1024)           /* 64 KB */
#define LOOPBACK_PORT   17329
#define TARGET_MBPS     100.0

typedef struct {
    int listen_fd;
    int conn_fd;
} server_ctx_t;

static long timespec_diff_us(const struct timespec *a, const struct timespec *b) {
    return (long)(b->tv_sec - a->tv_sec) * 1000000L +
           (b->tv_nsec - a->tv_nsec) / 1000L;
}

/* Server thread: accept one connection, receive TRANSFER_BYTES, close */
static void *server_thread(void *arg) {
    server_ctx_t *ctx = (server_ctx_t *)arg;

    ctx->conn_fd = accept(ctx->listen_fd, NULL, NULL);
    if (ctx->conn_fd < 0) {
        perror("accept");
        return NULL;
    }

    uint8_t *buf = malloc(CHUNK_SIZE);
    if (!buf) return NULL;

    ssize_t total = 0;
    while (total < TRANSFER_BYTES) {
        ssize_t n = recv(ctx->conn_fd, buf, CHUNK_SIZE, 0);
        if (n <= 0) break;
        total += n;
    }

    free(buf);
    close(ctx->conn_fd);
    ctx->conn_fd = -1;
    return NULL;
}

int main(void) {
    /* ------------------------------------------------------------------ */
    /* Set up listening socket                                              */
    /* ------------------------------------------------------------------ */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons(LOOPBACK_PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(listen_fd); return 1;
    }
    if (listen(listen_fd, 1) < 0) {
        perror("listen"); close(listen_fd); return 1;
    }

    /* ------------------------------------------------------------------ */
    /* Launch server thread                                                 */
    /* ------------------------------------------------------------------ */
    server_ctx_t ctx = { .listen_fd = listen_fd, .conn_fd = -1 };
    pthread_t srv_tid;
    pthread_create(&srv_tid, NULL, server_thread, &ctx);

    /* ------------------------------------------------------------------ */
    /* Connect client socket                                                */
    /* ------------------------------------------------------------------ */
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { perror("socket"); return 1; }

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(client_fd); close(listen_fd); return 1;
    }

    /* ------------------------------------------------------------------ */
    /* Send TRANSFER_BYTES and measure latency of first round-trip chunk   */
    /* ------------------------------------------------------------------ */
    uint8_t *send_buf = malloc(CHUNK_SIZE);
    if (!send_buf) { close(client_fd); close(listen_fd); return 1; }
    memset(send_buf, 0xAB, CHUNK_SIZE);

    struct timespec t_start, t_end;
    long first_chunk_us = 0;

    clock_gettime(CLOCK_MONOTONIC, &t_start);

    ssize_t total_sent = 0;
    int first = 1;
    while (total_sent < TRANSFER_BYTES) {
        size_t to_send = CHUNK_SIZE;
        if ((ssize_t)to_send > TRANSFER_BYTES - total_sent)
            to_send = (size_t)(TRANSFER_BYTES - total_sent);

        struct timespec chunk_start;
        if (first) clock_gettime(CLOCK_MONOTONIC, &chunk_start);

        ssize_t n = send(client_fd, send_buf, to_send, 0);
        if (n <= 0) break;

        if (first) {
            struct timespec chunk_end;
            clock_gettime(CLOCK_MONOTONIC, &chunk_end);
            first_chunk_us = timespec_diff_us(&chunk_start, &chunk_end);
            first = 0;
        }
        total_sent += n;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    close(client_fd);
    pthread_join(srv_tid, NULL);
    close(listen_fd);
    free(send_buf);

    /* ------------------------------------------------------------------ */
    /* Compute results                                                      */
    /* ------------------------------------------------------------------ */
    double elapsed_s = (double)timespec_diff_us(&t_start, &t_end) / 1e6;
    double mbps = ((double)total_sent / (1024.0 * 1024.0)) / elapsed_s;

    printf("BENCH tcp_loopback: throughput=%.1f MB/s latency=%ldus\n",
           mbps, first_chunk_us);

    return (mbps >= TARGET_MBPS) ? 0 : 1;
}
