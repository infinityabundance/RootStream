#ifndef ROOTSTREAM_H
#define ROOTSTREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>

/* ============================================================================
 * RootStream - Native Linux Game Streaming
 * 
 * A lightweight streaming solution that works directly with kernel APIs
 * to bypass the broken PipeWire/compositor ecosystem.
 * 
 * Architecture:
 *   1. DRM/KMS Direct Capture - Read framebuffers straight from GPU
 *   2. Hardware Encoding - VA-API (Intel/AMD) or NVENC (NVIDIA)  
 *   3. UDP with FEC - Custom low-latency protocol
 *   4. uinput Injection - Virtual input devices for remote control
 * ============================================================================ */

#define ROOTSTREAM_VERSION "0.1.0"
#define MAX_DISPLAYS 4
#define MAX_PACKET_SIZE 1400  // MTU-safe

/* Frame capture modes */
typedef enum {
    CAPTURE_DRM_KMS,      /* Direct kernel DRM/KMS (best) */
    CAPTURE_MMAP,         /* Memory mapped framebuffer */
    CAPTURE_FALLBACK      /* Software fallback */
} capture_mode_t;

/* Hardware encoder types */
typedef enum {
    ENCODER_VAAPI,        /* VA-API (Intel/AMD) */
    ENCODER_NVENC,        /* NVENC (NVIDIA) */
    ENCODER_SOFTWARE      /* CPU encoding (fallback) */
} encoder_type_t;

/* Display information */
typedef struct {
    int fd;                    /* DRM device file descriptor */
    uint32_t connector_id;     /* DRM connector ID */
    uint32_t crtc_id;          /* CRTC ID */
    uint32_t fb_id;            /* Framebuffer ID */
    uint32_t width;
    uint32_t height;
    uint32_t refresh_rate;
    char name[64];             /* e.g., "HDMI-A-1" */
} display_info_t;

/* Frame buffer structure */
typedef struct {
    uint8_t *data;            /* Frame data */
    uint32_t size;            /* Size in bytes */
    uint32_t width;
    uint32_t height;
    uint32_t pitch;           /* Bytes per row */
    uint32_t format;          /* Pixel format (fourcc) */
    uint64_t timestamp;       /* Capture timestamp (usec) */
} frame_buffer_t;

/* Encoder context */
typedef struct {
    encoder_type_t type;
    int device_fd;            /* Encoder device fd */
    void *hw_ctx;             /* Hardware context */
    
    /* Encoding parameters */
    uint32_t bitrate;
    uint32_t framerate;
    uint8_t quality;          /* 0-100 */
    bool low_latency;
} encoder_ctx_t;

/* Network packet header */
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* 0x524F4F54 "ROOT" */
    uint8_t version;
    uint8_t type;             /* PACKET_VIDEO, PACKET_AUDIO, etc */
    uint16_t sequence;
    uint32_t timestamp;
    uint32_t payload_size;
    uint16_t checksum;
} packet_header_t;

/* Packet types */
#define PACKET_VIDEO      0x01
#define PACKET_AUDIO      0x02
#define PACKET_INPUT      0x03
#define PACKET_CONTROL    0x04

/* Input event structure */
typedef struct __attribute__((packed)) {
    uint8_t type;             /* KEY, MOUSE_MOVE, MOUSE_BUTTON, etc */
    uint16_t code;            /* Key/button code */
    int32_t value;            /* Value/delta */
} input_event_pkt_t;

/* Main streaming context */
typedef struct {
    /* Capture */
    capture_mode_t capture_mode;
    display_info_t display;
    frame_buffer_t current_frame;
    
    /* Encoding */
    encoder_ctx_t encoder;
    
    /* Network */
    int sock_fd;
    struct sockaddr_storage peer_addr;
    uint16_t sequence;
    
    /* Input */
    int uinput_kbd_fd;
    int uinput_mouse_fd;
    
    /* State */
    bool running;
    uint64_t frames_captured;
    uint64_t frames_encoded;
    uint64_t bytes_sent;
} rootstream_ctx_t;

/* API Functions */

/* Initialization */
int rootstream_init(rootstream_ctx_t *ctx);
int rootstream_detect_displays(display_info_t *displays, int max_displays);
int rootstream_select_display(rootstream_ctx_t *ctx, int display_index);

/* Capture */
int rootstream_capture_init(rootstream_ctx_t *ctx);
int rootstream_capture_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame);
void rootstream_capture_cleanup(rootstream_ctx_t *ctx);

/* Encoding */
int rootstream_encoder_init(rootstream_ctx_t *ctx, encoder_type_t type);
int rootstream_encode_frame(rootstream_ctx_t *ctx, frame_buffer_t *in, uint8_t *out, size_t *out_size);
void rootstream_encoder_cleanup(rootstream_ctx_t *ctx);

/* Network */
int rootstream_net_init(rootstream_ctx_t *ctx, const char *bind_addr, uint16_t port);
int rootstream_net_send(rootstream_ctx_t *ctx, uint8_t type, const void *data, size_t size);
int rootstream_net_recv(rootstream_ctx_t *ctx, void *buf, size_t size, int timeout_ms);

/* Input */
int rootstream_input_init(rootstream_ctx_t *ctx);
int rootstream_input_process(rootstream_ctx_t *ctx, input_event_pkt_t *event);
void rootstream_input_cleanup(rootstream_ctx_t *ctx);

/* Main loops */
int rootstream_run_host(rootstream_ctx_t *ctx);
int rootstream_run_client(rootstream_ctx_t *ctx, const char *host_addr);

/* Cleanup */
void rootstream_cleanup(rootstream_ctx_t *ctx);

/* Utilities */
const char* rootstream_get_error(void);
void rootstream_print_stats(rootstream_ctx_t *ctx);

#endif /* ROOTSTREAM_H */
