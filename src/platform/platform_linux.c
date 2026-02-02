/*
 * platform_linux.c - Linux Platform Implementation
 *
 * Implements platform abstraction for Linux systems.
 * Uses POSIX APIs for sockets, timing, and file operations.
 */

#ifndef RS_PLATFORM_LINUX
#define RS_PLATFORM_LINUX 1
#endif

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>

/* ============================================================================
 * Platform Initialization
 * ============================================================================ */

int rs_platform_init(void) {
    /* Linux doesn't need special initialization */
    return 0;
}

void rs_platform_cleanup(void) {
    /* Nothing to clean up on Linux */
}

/* ============================================================================
 * Network Implementation
 * ============================================================================ */

int rs_net_init(void) {
    /* POSIX sockets don't need initialization */
    return 0;
}

void rs_net_cleanup(void) {
    /* Nothing to clean up */
}

rs_socket_t rs_socket_create(int af, int type, int protocol) {
    return socket(af, type, protocol);
}

int rs_socket_close(rs_socket_t sock) {
    return close(sock);
}

int rs_socket_bind(rs_socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sock, addr, addrlen);
}

int rs_socket_setopt(rs_socket_t sock, int level, int optname,
                     const void *optval, size_t optlen) {
    return setsockopt(sock, level, optname, optval, (socklen_t)optlen);
}

int rs_socket_poll(rs_socket_t sock, int timeout_ms) {
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;
    pfd.revents = 0;
    return poll(&pfd, 1, timeout_ms);
}

int rs_socket_sendto(rs_socket_t sock, const void *buf, size_t len, int flags,
                     const struct sockaddr *dest_addr, socklen_t addrlen) {
    return (int)sendto(sock, buf, len, flags, dest_addr, addrlen);
}

int rs_socket_recvfrom(rs_socket_t sock, void *buf, size_t len, int flags,
                       struct sockaddr *src_addr, socklen_t *addrlen) {
    return (int)recvfrom(sock, buf, len, flags, src_addr, addrlen);
}

int rs_socket_error(void) {
    return errno;
}

const char* rs_socket_strerror(int err) {
    return strerror(err);
}

/* ============================================================================
 * Timing Implementation
 * ============================================================================ */

uint64_t rs_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

uint64_t rs_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

void rs_sleep_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void rs_sleep_us(uint32_t us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * File System Implementation
 * ============================================================================ */

const char* rs_config_dir(void) {
    static char config_dir[512] = {0};

    if (config_dir[0] != '\0') {
        return config_dir;
    }

    /* Check XDG_CONFIG_HOME first */
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        snprintf(config_dir, sizeof(config_dir), "%s/rootstream", xdg);
        return config_dir;
    }

    /* Fall back to ~/.config/rootstream */
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }

    if (home) {
        snprintf(config_dir, sizeof(config_dir), "%s/.config/rootstream", home);
    } else {
        /* Last resort */
        snprintf(config_dir, sizeof(config_dir), "/tmp/rootstream");
    }

    return config_dir;
}

int rs_mkdir(const char *path, int mode) {
    return mkdir(path, (mode_t)mode);
}

int rs_chmod(const char *path, int mode) {
    return chmod(path, (mode_t)mode);
}

bool rs_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int rs_unlink(const char *path) {
    return unlink(path);
}

int rs_fsync(void *fp) {
    FILE *f = (FILE *)fp;
    return fsync(fileno(f));
}

int rs_gethostname(char *buf, size_t len) {
    return gethostname(buf, len);
}

/* ============================================================================
 * Path Utilities
 * ============================================================================ */

char rs_path_separator(void) {
    return '/';
}

char* rs_path_join(char *buf, size_t buflen, const char *base, const char *name) {
    if (!buf || buflen == 0 || !base || !name) {
        return NULL;
    }

    int ret = snprintf(buf, buflen, "%s/%s", base, name);
    if (ret < 0 || (size_t)ret >= buflen) {
        return NULL;
    }

    return buf;
}
