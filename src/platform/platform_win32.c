/*
 * platform_win32.c - Windows Platform Implementation
 *
 * Implements platform abstraction for Windows systems.
 * Uses Winsock2 for networking, QueryPerformanceCounter for timing,
 * and Win32 API for file operations.
 */

#ifdef _WIN32

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <shlobj.h>

/* For FlushFileBuffers */
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

/* Performance counter frequency (cached) */
static LARGE_INTEGER perf_freq = {0};
static bool perf_initialized = false;

/* Thread-local error message buffer for rs_socket_strerror */
static __declspec(thread) char error_buf[256];

/* ============================================================================
 * Platform Initialization
 * ============================================================================ */

int rs_platform_init(void) {
    /* Initialize performance counter frequency */
    if (!perf_initialized) {
        if (!QueryPerformanceFrequency(&perf_freq)) {
            fprintf(stderr, "ERROR: QueryPerformanceFrequency failed\n");
            return -1;
        }
        perf_initialized = true;
    }
    return 0;
}

void rs_platform_cleanup(void) {
    /* Nothing to clean up on Windows platform level */
}

/* ============================================================================
 * Network Implementation (Winsock2)
 * ============================================================================ */

static bool wsa_initialized = false;

int rs_net_init(void) {
    if (wsa_initialized) {
        return 0;
    }

    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        fprintf(stderr, "ERROR: WSAStartup failed with error %d\n", result);
        return -1;
    }

    /* Verify we got Winsock 2.2 */
    if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
        fprintf(stderr, "ERROR: Could not find Winsock 2.2\n");
        WSACleanup();
        return -1;
    }

    wsa_initialized = true;
    return 0;
}

void rs_net_cleanup(void) {
    if (wsa_initialized) {
        WSACleanup();
        wsa_initialized = false;
    }
}

rs_socket_t rs_socket_create(int af, int type, int protocol) {
    return socket(af, type, protocol);
}

int rs_socket_close(rs_socket_t sock) {
    return closesocket(sock) == 0 ? 0 : -1;
}

int rs_socket_bind(rs_socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sock, addr, addrlen);
}

int rs_socket_setopt(rs_socket_t sock, int level, int optname,
                     const void *optval, size_t optlen) {
    return setsockopt(sock, level, optname, (const char *)optval, (int)optlen);
}

int rs_socket_poll(rs_socket_t sock, int timeout_ms) {
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
    /* Use FormatMessage to get Windows error string */
    DWORD result = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        error_buf,
        sizeof(error_buf),
        NULL
    );

    if (result == 0) {
        snprintf(error_buf, sizeof(error_buf), "Unknown error %d", err);
    } else {
        /* Remove trailing newline if present */
        size_t len = strlen(error_buf);
        while (len > 0 && (error_buf[len - 1] == '\n' || error_buf[len - 1] == '\r')) {
            error_buf[--len] = '\0';
        }
    }

    return error_buf;
}

/* ============================================================================
 * Timing Implementation (QueryPerformanceCounter)
 * ============================================================================ */

uint64_t rs_timestamp_ms(void) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    /* Convert to milliseconds: (counter * 1000) / freq */
    return (uint64_t)(counter.QuadPart * 1000 / perf_freq.QuadPart);
}

uint64_t rs_timestamp_us(void) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    /* Convert to microseconds: (counter * 1000000) / freq */
    return (uint64_t)(counter.QuadPart * 1000000 / perf_freq.QuadPart);
}

void rs_sleep_ms(uint32_t ms) {
    Sleep(ms);
}

void rs_sleep_us(uint32_t us) {
    /* Windows Sleep has millisecond resolution, so we do our best */
    if (us >= 1000) {
        Sleep(us / 1000);
    } else if (us > 0) {
        /* For sub-millisecond, use busy wait with high-resolution timer */
        LARGE_INTEGER start, current;
        QueryPerformanceCounter(&start);
        uint64_t target_ticks = (uint64_t)us * perf_freq.QuadPart / 1000000;

        do {
            QueryPerformanceCounter(&current);
        } while ((uint64_t)(current.QuadPart - start.QuadPart) < target_ticks);
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

    /* Get %APPDATA% path */
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        snprintf(config_dir, sizeof(config_dir), "%s\\RootStream", appdata);
    } else {
        /* Fallback to current directory */
        snprintf(config_dir, sizeof(config_dir), ".\\RootStream");
    }

    return config_dir;
}

int rs_mkdir(const char *path, int mode) {
    (void)mode;  /* Windows doesn't use Unix permissions */
    return _mkdir(path) == 0 || errno == EEXIST ? 0 : -1;
}

int rs_chmod(const char *path, int mode) {
    (void)path;
    (void)mode;
    /* Windows doesn't use Unix-style permissions the same way
     * For basic functionality, we just return success */
    return 0;
}

bool rs_file_exists(const char *path) {
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

int rs_unlink(const char *path) {
    return DeleteFileA(path) ? 0 : -1;
}

int rs_fsync(void *fp) {
    FILE *f = (FILE *)fp;
    int fd = _fileno(f);
    if (fd == -1) {
        return -1;
    }
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) {
        return -1;
    }
    return FlushFileBuffers(h) ? 0 : -1;
}

int rs_gethostname(char *buf, size_t len) {
    /* gethostname is available via Winsock */
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
