/*
 * platform.h - Platform Abstraction Layer for RootStream
 *
 * Provides unified API for platform-specific functionality:
 * - Socket operations (POSIX vs Winsock)
 * - High-resolution timing
 * - Configuration paths
 * - File operations
 */

#ifndef ROOTSTREAM_PLATFORM_H
#define ROOTSTREAM_PLATFORM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Platform Detection
 * ============================================================================ */

#if defined(_WIN32) || defined(_WIN64)
    #define RS_PLATFORM_WINDOWS 1
    #define RS_PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define RS_PLATFORM_LINUX 1
    #define RS_PLATFORM_NAME "Linux"
#elif defined(__APPLE__)
    #define RS_PLATFORM_MACOS 1
    #define RS_PLATFORM_NAME "macOS"
#else
    #error "Unsupported platform"
#endif

/* ============================================================================
 * Socket Types and Includes
 * ============================================================================ */

#ifdef RS_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    typedef SOCKET rs_socket_t;
    #define RS_INVALID_SOCKET INVALID_SOCKET
    #define RS_SOCKET_ERROR   SOCKET_ERROR

    /* Windows doesn't have socklen_t in older SDKs */
    #ifndef socklen_t
        typedef int socklen_t;
    #endif

#else /* POSIX (Linux, macOS) */
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <poll.h>
    #include <errno.h>

    typedef int rs_socket_t;
    #define RS_INVALID_SOCKET (-1)
    #define RS_SOCKET_ERROR   (-1)
#endif

/* ============================================================================
 * Platform Initialization
 * ============================================================================ */

/**
 * Initialize platform-specific subsystems.
 * Must be called before any other platform functions.
 *
 * @return 0 on success, -1 on error
 */
int rs_platform_init(void);

/**
 * Clean up platform-specific subsystems.
 * Call before program exit.
 */
void rs_platform_cleanup(void);

/* ============================================================================
 * Network API
 * ============================================================================ */

/**
 * Initialize networking subsystem.
 * On Windows, calls WSAStartup.
 *
 * @return 0 on success, -1 on error
 */
int rs_net_init(void);

/**
 * Clean up networking subsystem.
 * On Windows, calls WSACleanup.
 */
void rs_net_cleanup(void);

/**
 * Create a socket.
 *
 * @param af Address family (AF_INET, AF_INET6)
 * @param type Socket type (SOCK_DGRAM, SOCK_STREAM)
 * @param protocol Protocol (0 for default)
 * @return Socket handle or RS_INVALID_SOCKET on error
 */
rs_socket_t rs_socket_create(int af, int type, int protocol);

/**
 * Close a socket.
 *
 * @param sock Socket to close
 * @return 0 on success, -1 on error
 */
int rs_socket_close(rs_socket_t sock);

/**
 * Bind socket to address.
 *
 * @param sock Socket handle
 * @param addr Address to bind
 * @param addrlen Address length
 * @return 0 on success, -1 on error
 */
int rs_socket_bind(rs_socket_t sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Set socket option.
 *
 * @param sock Socket handle
 * @param level Option level (SOL_SOCKET, IPPROTO_IP, etc.)
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option value length
 * @return 0 on success, -1 on error
 */
int rs_socket_setopt(rs_socket_t sock, int level, int optname,
                     const void *optval, size_t optlen);

/**
 * Poll socket for readability.
 *
 * @param sock Socket to poll
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return >0 if readable, 0 on timeout, -1 on error
 */
int rs_socket_poll(rs_socket_t sock, int timeout_ms);

/**
 * Send data on UDP socket.
 *
 * @param sock Socket handle
 * @param buf Data buffer
 * @param len Data length
 * @param flags Send flags
 * @param dest_addr Destination address
 * @param addrlen Address length
 * @return Bytes sent or -1 on error
 */
int rs_socket_sendto(rs_socket_t sock, const void *buf, size_t len, int flags,
                     const struct sockaddr *dest_addr, socklen_t addrlen);

/**
 * Receive data from UDP socket.
 *
 * @param sock Socket handle
 * @param buf Receive buffer
 * @param len Buffer length
 * @param flags Receive flags
 * @param src_addr Source address (output)
 * @param addrlen Address length (in/out)
 * @return Bytes received or -1 on error
 */
int rs_socket_recvfrom(rs_socket_t sock, void *buf, size_t len, int flags,
                       struct sockaddr *src_addr, socklen_t *addrlen);

/**
 * Get last socket error code.
 *
 * @return Error code (errno on POSIX, WSAGetLastError on Windows)
 */
int rs_socket_error(void);

/**
 * Get error message for socket error code.
 *
 * @param err Error code from rs_socket_error()
 * @return Human-readable error message
 */
const char* rs_socket_strerror(int err);

/* ============================================================================
 * Timing API
 * ============================================================================ */

/**
 * Get current timestamp in milliseconds (monotonic clock).
 *
 * @return Milliseconds since arbitrary epoch
 */
uint64_t rs_timestamp_ms(void);

/**
 * Get current timestamp in microseconds (monotonic clock).
 *
 * @return Microseconds since arbitrary epoch
 */
uint64_t rs_timestamp_us(void);

/**
 * Sleep for specified milliseconds.
 *
 * @param ms Milliseconds to sleep
 */
void rs_sleep_ms(uint32_t ms);

/**
 * Sleep for specified microseconds.
 * Note: Resolution may be limited on some platforms.
 *
 * @param us Microseconds to sleep
 */
void rs_sleep_us(uint32_t us);

/* ============================================================================
 * File System API
 * ============================================================================ */

/**
 * Get platform-specific configuration directory.
 *
 * @return Path to config directory:
 *         - Linux: ~/.config/rootstream
 *         - Windows: %APPDATA%\RootStream
 *         - macOS: ~/Library/Application Support/RootStream
 */
const char* rs_config_dir(void);

/**
 * Create directory with specified permissions.
 *
 * @param path Directory path
 * @param mode Unix permissions (ignored on Windows)
 * @return 0 on success, -1 on error
 */
int rs_mkdir(const char *path, int mode);

/**
 * Change file permissions.
 *
 * @param path File path
 * @param mode Unix permissions (ignored on Windows)
 * @return 0 on success, -1 on error
 */
int rs_chmod(const char *path, int mode);

/**
 * Check if file exists.
 *
 * @param path File path
 * @return true if exists, false otherwise
 */
bool rs_file_exists(const char *path);

/**
 * Delete file.
 *
 * @param path File path
 * @return 0 on success, -1 on error
 */
int rs_unlink(const char *path);

/**
 * Sync file to disk.
 *
 * @param fp FILE pointer
 * @return 0 on success, -1 on error
 */
int rs_fsync(void *fp);

/**
 * Get hostname of this machine.
 *
 * @param buf Output buffer
 * @param len Buffer length
 * @return 0 on success, -1 on error
 */
int rs_gethostname(char *buf, size_t len);

/* ============================================================================
 * Path Utilities
 * ============================================================================ */

/**
 * Get path separator for current platform.
 *
 * @return '/' on Unix, '\\' on Windows
 */
char rs_path_separator(void);

/**
 * Join path components.
 *
 * @param buf Output buffer
 * @param buflen Buffer length
 * @param base Base path
 * @param name File/directory name
 * @return Pointer to buf, or NULL on error
 */
char* rs_path_join(char *buf, size_t buflen, const char *base, const char *name);

#endif /* ROOTSTREAM_PLATFORM_H */
