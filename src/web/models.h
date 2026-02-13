/**
 * PHASE 19: Web Dashboard - Data Models
 * 
 * Defines data structures for REST API and WebSocket communication
 */

#ifndef WEB_MODELS_H
#define WEB_MODELS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Host information
 */
typedef struct {
    char hostname[256];
    char platform[64];              // Linux, Windows, macOS
    char rootstream_version[32];
    uint32_t uptime_seconds;
    bool is_streaming;
} host_info_t;

/**
 * Peer information
 */
typedef struct {
    char peer_id[64];
    char name[256];
    char capability[16];            // "host" or "client"
    char ip_address[64];
    uint16_t port;
    char version[32];
    uint32_t max_peers;
    char bandwidth[32];
    bool is_online;
    uint64_t last_seen_time_us;
} peer_info_t;

/**
 * Stream information
 */
typedef struct {
    char stream_id[64];
    char peer_name[256];
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint64_t start_time_us;
    bool is_recording;
    char recording_file[512];
    uint64_t recording_size_bytes;
} stream_info_t;

/**
 * Real-time metrics snapshot
 */
typedef struct {
    uint32_t fps;
    uint32_t rtt_ms;
    uint32_t jitter_ms;
    uint32_t gpu_util;
    uint32_t gpu_temp;
    uint32_t cpu_util;
    float bandwidth_mbps;
    uint64_t packets_sent;
    uint64_t packets_lost;
    uint64_t bytes_sent;
    uint64_t timestamp_us;
} metrics_snapshot_t;

/**
 * Video settings
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrate_kbps;
    char encoder[32];               // vaapi, nvenc, ffmpeg, raw
    char codec[32];                 // h264, h265, vp9
} video_settings_t;

/**
 * Audio settings
 */
typedef struct {
    char output_device[256];
    char input_device[256];
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate_kbps;
} audio_settings_t;

/**
 * Network settings
 */
typedef struct {
    uint16_t port;
    uint32_t target_bitrate_mbps;
    uint32_t buffer_size_ms;
    bool enable_tcp_fallback;
    bool enable_encryption;
} network_settings_t;

/**
 * WebSocket message types
 */
typedef enum {
    WS_MSG_METRICS = 1,          // Metrics update
    WS_MSG_EVENT = 2,            // Event notification
    WS_MSG_COMMAND = 3,          // Remote command
    WS_MSG_ACK = 4,              // Acknowledgment
} websocket_message_type_t;

/**
 * WebSocket message
 */
typedef struct {
    uint32_t message_id;
    websocket_message_type_t type;
    char payload[4096];
} websocket_message_t;

/**
 * User roles for RBAC
 */
typedef enum {
    ROLE_ADMIN = 1,              // Full access
    ROLE_OPERATOR = 2,           // Control streaming
    ROLE_VIEWER = 3,             // View-only
} user_role_t;

/**
 * Authentication token
 */
typedef struct {
    char username[256];
    user_role_t role;
    uint64_t expires_at;
    uint64_t issued_at;
    char token[512];
} auth_token_t;

#ifdef __cplusplus
}
#endif

#endif // WEB_MODELS_H
