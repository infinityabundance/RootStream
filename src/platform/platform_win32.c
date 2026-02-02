/*
 * platform_win32.c - Windows Platform Implementation
 *
 * Implements platform abstraction for Windows systems.
 * Uses Winsock2 for networking, QueryPerformanceCounter for timing.
 */

#ifdef _WIN32

#ifndef RS_PLATFORM_WINDOWS
#define RS_PLATFORM_WINDOWS 1
#endif

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>

/* High-resolution timer state */
static LARGE_INTEGER qpc_frequency;
static bool qpc_initialized = false;

/* Error message buffer (thread-local would be better) */
static char error_buffer[512];

/* ============================================================================
 * Platform Initialization
 * ============================================================================ */

int rs_platform_init(void) {
    /* Initialize QPC frequency */
    if (!qpc_initialized) {
        QueryPerformanceFrequency(&qpc_frequency);
        qpc_initialized = true;
    }

    /* Initialize COM for Media Foundation */
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return -1;
    }

    return rs_net_init();
}

void rs_platform_cleanup(void) {
    rs_net_cleanup();
    CoUninitialize();
}

/* ============================================================================
 * Network Implementation (Winsock2)
 * ============================================================================ */

int rs_net_init(void) {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return -1;
    }
    return 0;
}

void rs_net_cleanup(void) {
    WSACleanup();
}

rs_socket_t rs_socket_create(int af, int type, int protocol) {
    return socket(af, type, protocol);
}

int rs_socket_close(rs_socket_t sock) {
    return closesocket(sock);
}

int rs_socket_bind(rs_socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sock, addr, addrlen);
}

int rs_socket_setopt(rs_socket_t sock, int level, int optname,
                     const void *optval, size_t optlen) {
    /* Windows setsockopt takes const char* for optval */
    return setsockopt(sock, level, optname, (const char *)optval, (int)optlen);
}

int rs_socket_poll(rs_socket_t sock, int timeout_ms) {
    /* Use WSAPoll (Windows Vista+) */
    WSAPOLLFD pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;
    pfd.revents = 0;
    return WSAPoll(&pfd, 1, timeout_ms);
}

int rs_socket_sendto(rs_socket_t sock, const void *buf, size_t len, int flags,
                     const struct sockaddr *dest_addr, socklen_t addrlen) {
    return sendto(sock, (const char *)buf, (int)len, flags, dest_addr, addrlen);
}

int rs_socket_recvfrom(rs_socket_t sock, void *buf, size_t len, int flags,
                       struct sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sock, (char *)buf, (int)len, flags, src_addr, addrlen);
}

int rs_socket_error(void) {
    return WSAGetLastError();
}

const char* rs_socket_strerror(int err) {
    /* Format Windows error message */
    DWORD result = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        error_buffer,
        sizeof(error_buffer),
        NULL
    );

    if (result == 0) {
        snprintf(error_buffer, sizeof(error_buffer), "Unknown error %d", err);
    } else {
        /* Remove trailing newline */
        size_t len = strlen(error_buffer);
        while (len > 0 && (error_buffer[len-1] == '\n' || error_buffer[len-1] == '\r')) {
            error_buffer[--len] = '\0';
        }
    }

    return error_buffer;
}

/* ============================================================================
 * Timing Implementation (QueryPerformanceCounter)
 * ============================================================================ */

uint64_t rs_timestamp_ms(void) {
    if (!qpc_initialized) {
        QueryPerformanceFrequency(&qpc_frequency);
        qpc_initialized = true;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (uint64_t)((counter.QuadPart * 1000) / qpc_frequency.QuadPart);
}

uint64_t rs_timestamp_us(void) {
    if (!qpc_initialized) {
        QueryPerformanceFrequency(&qpc_frequency);
        qpc_initialized = true;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (uint64_t)((counter.QuadPart * 1000000) / qpc_frequency.QuadPart);
}

void rs_sleep_ms(uint32_t ms) {
    Sleep(ms);
}

void rs_sleep_us(uint32_t us) {
    /* Windows Sleep() has ~15ms resolution, so use busy-wait for short delays */
    if (us >= 1000) {
        Sleep(us / 1000);
        us = us % 1000;
    }

    if (us > 0) {
        /* Busy-wait for sub-millisecond precision */
        uint64_t start = rs_timestamp_us();
        while (rs_timestamp_us() - start < us) {
            /* Spin */
        }
    }
}

/* ============================================================================
 * File System Implementation
 * ============================================================================ */

const char* rs_config_dir(void) {
    static char config_dir[MAX_PATH] = {0};

    if (config_dir[0] != '\0') {
        return config_dir;
    }

    /* Get APPDATA folder */
    const char *appdata = getenv("APPDATA");
    if (appdata && appdata[0] != '\0') {
        snprintf(config_dir, sizeof(config_dir), "%s\\RootStream", appdata);
    } else {
        /* Fallback to current directory */
        snprintf(config_dir, sizeof(config_dir), ".\\RootStream");
    }

    return config_dir;
}

int rs_mkdir(const char *path, int mode) {
    (void)mode;  /* Windows doesn't use Unix permissions */
    return _mkdir(path);
}

int rs_chmod(const char *path, int mode) {
    (void)path;
    (void)mode;
    /* Windows uses ACLs, not chmod - no-op for compatibility */
    /* For private key protection, consider using SetFileSecurity() */
    return 0;
}

bool rs_file_exists(const char *path) {
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES);
}

int rs_unlink(const char *path) {
    return DeleteFileA(path) ? 0 : -1;
}

int rs_fsync(void *fp) {
    FILE *f = (FILE *)fp;
    return FlushFileBuffers((HANDLE)_get_osfhandle(_fileno(f))) ? 0 : -1;
}

int rs_gethostname(char *buf, size_t len) {
    /* Use Winsock gethostname */
    return gethostname(buf, (int)len);
}

/* ============================================================================
 * Path Utilities
 * ============================================================================ */

char rs_path_separator(void) {
    return '\\';
}

char* rs_path_join(char *buf, size_t buflen, const char *base, const char *name) {
    if (!buf || buflen == 0 || !base || !name) {
        return NULL;
    }

    int ret = snprintf(buf, buflen, "%s\\%s", base, name);
    if (ret < 0 || (size_t)ret >= buflen) {
        return NULL;
    }

    return buf;
}

#endif /* _WIN32 */
