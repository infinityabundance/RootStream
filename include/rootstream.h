#ifndef ROOTSTREAM_H
#define ROOTSTREAM_H

#include <stdint.h>
#include <stdbool.h>

/* Platform abstraction for cross-platform socket types */
#include "../src/platform/platform.h"

/* Cross-platform packed struct support */
#ifdef _MSC_VER
  #define PACKED_STRUCT __pragma(pack(push, 1)) struct
  #define PACKED_STRUCT_END __pragma(pack(pop))
#else
  #define PACKED_STRUCT struct __attribute__((packed))
  #define PACKED_STRUCT_END
#endif

/*
 * ============================================================================
 * RootStream - Secure Peer-to-Peer Game Streaming
 * ============================================================================
 * 
 * A lightweight, encrypted streaming solution with:
 * - Ed25519 public/private key authentication
 * - No accounts, no servers, just peer-to-peer
 * - QR code sharing for instant connection
 * - Auto-discovery on local network (mDNS)
 * - Direct kernel DRM capture (no compositor)
 * - VA-API hardware encoding
 * 
 * Architecture:
 *   [DRM Capture] → [VA-API Encode] → [Encrypt] → [UDP] → [Network]
 *                                                              ↓
 *   [Display] ← [VA-API Decode] ← [Decrypt] ← [UDP] ← [Receive]
 * 
 * Security:
 *   - Each device has Ed25519 keypair (32-byte public, 32-byte private)
 *   - All packets encrypted with ChaCha20-Poly1305
 *   - Perfect forward secrecy with ephemeral keys
 *   - No central authority, no account database
 * 
 * RootStream Code Format:
 *   <base64_public_key>@<hostname>
 *   Example: kXx7Y...Qp9w@gaming-pc
 * ============================================================================
 */

#define ROOTSTREAM_VERSION "1.0.0"
#define PROTOCOL_VERSION 1
#define PROTOCOL_MIN_VERSION 1
#define PROTOCOL_FLAGS 0
#define MAX_DISPLAYS 4
#define MAX_PACKET_SIZE 1400
#define MAX_PEERS 16

/* Cryptographic constants (libsodium) */
#define CRYPTO_PUBLIC_KEY_BYTES 32
#define CRYPTO_SECRET_KEY_BYTES 32
#define CRYPTO_NONCE_BYTES 24
#define CRYPTO_MAC_BYTES 16
#define CRYPTO_SHARED_KEY_BYTES 32

/* RootStream code format: base64(pubkey) + "@" + hostname */
#define ROOTSTREAM_CODE_MAX_LEN 128

/* ============================================================================
 * CAPTURE - DRM/KMS framebuffer capture
 * ============================================================================ */

typedef enum {
    CAPTURE_DRM_KMS,      /* Direct kernel DRM/KMS (default) */
    CAPTURE_MMAP,         /* Memory mapped framebuffer fallback */
} capture_mode_t;

typedef struct {
    int fd;                    /* DRM device file descriptor */
    uint32_t connector_id;     /* DRM connector ID */
    uint32_t crtc_id;          /* CRTC ID */
    uint32_t fb_id;            /* Framebuffer ID */
    uint32_t width;            /* Display width in pixels */
    uint32_t height;           /* Display height in pixels */
    uint32_t refresh_rate;     /* Display refresh rate (Hz) */
    char name[64];             /* Display name (e.g., "HDMI-A-1") */
} display_info_t;

typedef struct {
    uint8_t *data;             /* Frame pixel data (RGBA) */
    uint32_t size;             /* Total size in bytes */
    uint32_t capacity;         /* Allocated buffer size in bytes */
    uint32_t width;            /* Frame width */
    uint32_t height;           /* Frame height */
    uint32_t pitch;            /* Bytes per row (stride) */
    uint32_t format;           /* Pixel format (DRM fourcc) */
    uint64_t timestamp;        /* Capture timestamp (microseconds) */
    bool is_keyframe;          /* True if this is an I-frame/IDR */
} frame_buffer_t;

/* ============================================================================
 * LATENCY - Stage timing and reporting
 * ============================================================================ */

typedef struct {
    uint64_t capture_us;        /* Capture duration */
    uint64_t encode_us;         /* Encode duration */
    uint64_t send_us;           /* Send duration (all peers) */
    uint64_t total_us;          /* Capture → send duration */
} latency_sample_t;

typedef struct {
    bool enabled;               /* Enable latency logging */
    size_t capacity;            /* Ring buffer capacity */
    size_t count;               /* Samples stored */
    size_t cursor;              /* Next insert position */
    uint64_t report_interval_ms;/* How often to print stats */
    uint64_t last_report_ms;    /* Last report timestamp */
    latency_sample_t *samples;  /* Sample ring buffer */
} latency_stats_t;

/* ============================================================================
 * ENCODING - VA-API hardware video encoding
 * ============================================================================ */

typedef enum {
    ENCODER_VAAPI,        /* VA-API (Intel/AMD) */
    ENCODER_NVENC,        /* NVENC (NVIDIA) */
    ENCODER_SOFTWARE      /* CPU encoding fallback - TODO */
} encoder_type_t;

typedef enum {
    CODEC_H264,           /* H.264/AVC */
    CODEC_H265            /* H.265/HEVC */
} codec_type_t;

typedef struct {
    encoder_type_t type;       /* Encoder type */
    codec_type_t codec;        /* Video codec */
    int device_fd;             /* Encoder device file descriptor */
    void *hw_ctx;              /* Hardware context (opaque) */

    /* Encoding parameters */
    uint32_t bitrate;          /* Target bitrate (bits/sec) */
    uint32_t framerate;        /* Target framerate (fps) */
    uint8_t quality;           /* Quality level 0-100 */
    bool low_latency;          /* Enable low-latency mode */
    bool force_keyframe;       /* Force next frame as keyframe */
    size_t max_output_size;    /* Max encoded output size (bytes) */
} encoder_ctx_t;

typedef struct {
    codec_type_t codec;        /* Video codec */
    void *backend_ctx;         /* Backend-specific context (opaque) */
    int width;                 /* Frame width */
    int height;                /* Frame height */
    bool initialized;          /* Decoder initialized? */
} decoder_ctx_t;

typedef struct {
    void *backend_ctx;         /* Backend-specific context (opaque) */
    int sample_rate;           /* Audio sample rate */
    int channels;              /* Number of channels */
    bool initialized;          /* Audio initialized? */
} audio_playback_ctx_t;

/* ============================================================================
 * CRYPTOGRAPHY - Ed25519 keypairs and encryption
 * ============================================================================ */

typedef struct {
    uint8_t public_key[CRYPTO_PUBLIC_KEY_BYTES];   /* Ed25519 public key */
    uint8_t secret_key[CRYPTO_SECRET_KEY_BYTES];   /* Ed25519 private key */
    char identity[128];                             /* Hostname/device name */
    char rootstream_code[ROOTSTREAM_CODE_MAX_LEN]; /* Public shareable code */
} keypair_t;

typedef struct {
    uint8_t shared_key[CRYPTO_SHARED_KEY_BYTES];   /* Shared encryption key */
    uint64_t nonce_counter;                         /* Nonce counter for packets */
    bool authenticated;                             /* Peer authenticated? */
} crypto_session_t;

/* ============================================================================
 * NETWORK - Encrypted UDP protocol
 * ============================================================================ */

/* Packet header (always plaintext for routing) */
typedef PACKED_STRUCT {
    uint32_t magic;            /* 0x524F4F54 "ROOT" */
    uint8_t version;           /* Protocol version (1) */
    uint8_t type;              /* Packet type (see below) */
    uint16_t flags;            /* Packet flags */
    uint64_t nonce;            /* Encryption nonce */
    uint16_t payload_size;     /* Encrypted payload size */
    uint8_t mac[CRYPTO_MAC_BYTES]; /* Authentication tag */
} packet_header_t;
PACKED_STRUCT_END

/* Packet types */
#define PKT_HANDSHAKE     0x01  /* Initial key exchange */
#define PKT_VIDEO         0x02  /* Encrypted video frame */
#define PKT_AUDIO         0x03  /* Encrypted audio frame */
#define PKT_INPUT         0x04  /* Encrypted input events */
#define PKT_CONTROL       0x05  /* Control messages */
#define PKT_PING          0x06  /* Keepalive ping */
#define PKT_PONG          0x07  /* Keepalive pong */

/* Control command types for PKT_CONTROL */
typedef enum {
    CTRL_PAUSE           = 0x01,  /* Pause streaming */
    CTRL_RESUME          = 0x02,  /* Resume streaming */
    CTRL_SET_BITRATE     = 0x03,  /* Change target bitrate */
    CTRL_SET_FPS         = 0x04,  /* Change target framerate */
    CTRL_REQUEST_KEYFRAME = 0x05, /* Request immediate keyframe */
    CTRL_SET_QUALITY     = 0x06,  /* Change quality level */
    CTRL_DISCONNECT      = 0x07,  /* Graceful disconnect */
} control_cmd_t;

/* Control packet payload (encrypted) */
typedef PACKED_STRUCT {
    uint8_t cmd;       /* control_cmd_t command */
    uint32_t value;    /* Command-specific value */
} control_packet_t;
PACKED_STRUCT_END

/* Fragmented video payload header (inside encrypted payload) */
typedef PACKED_STRUCT {
    uint32_t frame_id;     /* Frame sequence number */
    uint32_t total_size;   /* Total encoded frame size */
    uint32_t offset;       /* Offset of this chunk */
    uint16_t chunk_size;   /* Size of this chunk */
    uint16_t flags;        /* Reserved for future use */
    uint64_t timestamp_us; /* Capture timestamp */
} video_chunk_header_t;
PACKED_STRUCT_END

/* Audio payload header (inside encrypted payload) */
typedef PACKED_STRUCT {
    uint64_t timestamp_us; /* Capture timestamp */
    uint32_t sample_rate;  /* Samples per second */
    uint16_t channels;     /* Channel count */
    uint16_t samples;      /* Samples per channel */
} audio_packet_header_t;
PACKED_STRUCT_END

/* Encrypted input event payload */
typedef PACKED_STRUCT {
    uint8_t type;              /* EV_KEY, EV_REL, etc */
    uint16_t code;             /* Key/button code */
    int32_t value;             /* Value/delta */
} input_event_pkt_t;
PACKED_STRUCT_END

/* ============================================================================
 * PEER MANAGEMENT - Connected peer tracking
 * ============================================================================ */

typedef enum {
    PEER_DISCOVERED,           /* Found via mDNS */
    PEER_CONNECTING,           /* Handshake in progress */
    PEER_HANDSHAKE_SENT,       /* Sent handshake, awaiting response */
    PEER_HANDSHAKE_RECEIVED,   /* Received handshake, session established */
    PEER_CONNECTED,            /* Fully authenticated */
    PEER_DISCONNECTED,         /* Lost connection */
} peer_state_t;

typedef struct {
    char rootstream_code[ROOTSTREAM_CODE_MAX_LEN];  /* Peer's code */
    uint8_t public_key[CRYPTO_PUBLIC_KEY_BYTES];    /* Peer's public key */
    struct sockaddr_storage addr;                    /* Network address */
    socklen_t addr_len;                              /* Address length */
    crypto_session_t session;                        /* Encryption session */
    peer_state_t state;                              /* Connection state */
    uint64_t last_seen;                              /* Last packet time (ms) */
    uint64_t handshake_sent_time;                    /* Handshake timestamp for timeout */
    char hostname[64];                               /* Peer hostname */
    bool is_streaming;                               /* Currently streaming? */
    uint32_t video_tx_frame_id;                      /* Outgoing video frame counter */
    uint32_t video_rx_frame_id;                      /* Current incoming frame id */
    uint8_t *video_rx_buffer;                        /* Reassembly buffer */
    size_t video_rx_capacity;                        /* Reassembly buffer size */
    size_t video_rx_expected;                        /* Expected frame size */
    size_t video_rx_received;                        /* Bytes received so far */
    uint64_t last_sent;                              /* Last outbound packet time (ms) */
    uint64_t last_ping;                              /* Last keepalive ping time (ms) */
    uint8_t protocol_version;                       /* Peer protocol version */
    uint8_t protocol_flags;                         /* Peer protocol flags */
} peer_t;

/* ============================================================================
 * DISCOVERY - mDNS/Avahi service discovery
 * ============================================================================ */

typedef struct {
    void *avahi_client;        /* Avahi client (opaque) */
    void *avahi_group;         /* Avahi entry group (opaque) */
    void *avahi_browser;       /* Avahi service browser (opaque) */
    bool running;              /* Discovery active? */
} discovery_ctx_t;

/* ============================================================================
 * TRAY UI - GTK3 system tray application
 * ============================================================================ */

typedef enum {
    STATUS_IDLE,          /* Not streaming */
    STATUS_HOSTING,       /* Hosting stream */
    STATUS_CONNECTED,     /* Connected to peer */
    STATUS_ERROR,         /* Error state */
} tray_status_t;

typedef struct {
    void *gtk_app;            /* GtkApplication (opaque) */
    void *tray_icon;          /* GtkStatusIcon (opaque) */
    void *menu;               /* GtkMenu (opaque) */
    void *qr_window;          /* QR code display window (opaque) */
    tray_status_t status;     /* Current status */
} tray_ctx_t;

/* ============================================================================
 * CONFIGURATION - User settings from config.ini
 * ============================================================================ */

#define MAX_PEER_HISTORY 32

typedef struct {
    /* Video settings */
    uint32_t video_bitrate;    /* Target bitrate (bits/sec) */
    uint32_t video_framerate;  /* Target framerate (fps) */
    char video_codec[16];      /* Codec: "h264", "h265" */
    int display_index;         /* Preferred display index */

    /* Audio settings */
    bool audio_enabled;        /* Enable audio streaming */
    uint32_t audio_bitrate;    /* Audio bitrate (bits/sec) */

    /* Network settings */
    uint16_t network_port;     /* UDP port */
    bool discovery_enabled;    /* Enable mDNS discovery */

    /* Connection history */
    char peer_history[MAX_PEER_HISTORY][ROOTSTREAM_CODE_MAX_LEN];
    int peer_history_count;
    char last_connected[ROOTSTREAM_CODE_MAX_LEN];
} settings_t;

/* ============================================================================
 * RECORDING - Stream recording to file
 * ============================================================================ */

typedef struct {
    int fd;                    /* Recording file descriptor */
    bool active;               /* Recording in progress */
    uint64_t start_time_us;    /* Recording start timestamp */
    uint64_t frame_count;      /* Frames written */
    uint64_t bytes_written;    /* Total bytes written */
    char filename[256];        /* Output filename */
} recording_ctx_t;

/* ============================================================================
 * AUDIO BACKEND ABSTRACTION - Multi-fallback support
 * ============================================================================ */

/* Forward declaration */
typedef struct rootstream_ctx rootstream_ctx_t;

typedef struct {
    const char *name;
    int (*init_fn)(rootstream_ctx_t *ctx);
    int (*capture_fn)(rootstream_ctx_t *ctx, int16_t *samples, size_t *num_samples);
    void (*cleanup_fn)(rootstream_ctx_t *ctx);
    bool (*is_available_fn)(void);
} audio_capture_backend_t;

typedef struct {
    const char *name;
    int (*init_fn)(rootstream_ctx_t *ctx);
    int (*playback_fn)(rootstream_ctx_t *ctx, int16_t *samples, size_t num_samples);
    void (*cleanup_fn)(rootstream_ctx_t *ctx);
    bool (*is_available_fn)(void);
} audio_playback_backend_t;

/* ============================================================================
 * MAIN CONTEXT - Application state
 * ============================================================================ */

typedef struct rootstream_ctx {
    /* Identity */
    keypair_t keypair;         /* This device's keys */

    /* Configuration */
    settings_t settings;       /* User settings from config.ini */

    /* Capture & Encoding */
    capture_mode_t capture_mode;
    display_info_t display;
    frame_buffer_t current_frame;
    encoder_ctx_t encoder;

    /* Decoding (client) */
    decoder_ctx_t decoder;

    /* Audio (client) */
    audio_playback_ctx_t audio_playback;

    /* Audio backends (Phase 3) */
    const audio_capture_backend_t *audio_capture_backend;
    const audio_playback_backend_t *audio_playback_backend;

    /* Network */
    rs_socket_t sock_fd;       /* UDP socket */
    uint16_t port;             /* Listening port */

    /* Peers */
    peer_t peers[MAX_PEERS];   /* Connected peers */
    int num_peers;             /* Number of active peers */

    /* Discovery */
    discovery_ctx_t discovery;

    /* Input */
    int uinput_kbd_fd;         /* Virtual keyboard */
    int uinput_mouse_fd;       /* Virtual mouse */

    /* UI */
    tray_ctx_t tray;

    /* Recording */
    recording_ctx_t recording;

    /* State */
    bool running;              /* Main loop running? */
    bool is_service;           /* Running as systemd service? */
    uint64_t frames_captured;  /* Statistics (host) */
    uint64_t frames_encoded;   /* Statistics (host) */
    uint64_t frames_received;  /* Statistics (client) */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    latency_stats_t latency;   /* Latency instrumentation */
    bool is_host;              /* Host mode (streamer) */
    uint64_t last_video_ts_us; /* Last received video timestamp */
    uint64_t last_audio_ts_us; /* Last received audio timestamp */
} rootstream_ctx_t;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* --- Initialization --- */
int rootstream_init(rootstream_ctx_t *ctx);
void rootstream_cleanup(rootstream_ctx_t *ctx);

/* --- Cryptography --- */
int crypto_init(void);
int crypto_generate_keypair(keypair_t *kp, const char *hostname);
int crypto_load_keypair(keypair_t *kp, const char *config_dir);
int crypto_save_keypair(const keypair_t *kp, const char *config_dir);
int crypto_verify_peer(const uint8_t *public_key, size_t key_len);
int crypto_format_fingerprint(const uint8_t *public_key, size_t key_len,
                              char *output, size_t output_len);
int crypto_create_session(crypto_session_t *session, 
                          const uint8_t *my_secret,
                          const uint8_t *peer_public);
int crypto_encrypt_packet(const crypto_session_t *session,
                         const void *plaintext, size_t plain_len,
                         void *ciphertext, size_t *cipher_len,
                         uint64_t nonce);
int crypto_decrypt_packet(const crypto_session_t *session,
                         const void *ciphertext, size_t cipher_len,
                         void *plaintext, size_t *plain_len,
                         uint64_t nonce);

/* --- Capture (existing, polished) --- */
int rootstream_detect_displays(display_info_t *displays, int max_displays);
int rootstream_select_display(rootstream_ctx_t *ctx, int display_index);
int rootstream_capture_init(rootstream_ctx_t *ctx);
int rootstream_capture_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame);
void rootstream_capture_cleanup(rootstream_ctx_t *ctx);

/* --- Encoding (existing, polished) --- */
int rootstream_encoder_init(rootstream_ctx_t *ctx, encoder_type_t type, codec_type_t codec);
int rootstream_encode_frame(rootstream_ctx_t *ctx, frame_buffer_t *in,
                           uint8_t *out, size_t *out_size);
int rootstream_encode_frame_ex(rootstream_ctx_t *ctx, frame_buffer_t *in,
                              uint8_t *out, size_t *out_size, bool *is_keyframe);
void rootstream_encoder_cleanup(rootstream_ctx_t *ctx);

/* NVENC encoder (Phase 5) */
int rootstream_encoder_init_nvenc(rootstream_ctx_t *ctx, codec_type_t codec);
int rootstream_encode_frame_nvenc(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                  uint8_t *out, size_t *out_size);
void rootstream_encoder_cleanup_nvenc(rootstream_ctx_t *ctx);
bool rootstream_encoder_nvenc_available(void);

/* --- Decoding (Phase 1) --- */
int rootstream_decoder_init(rootstream_ctx_t *ctx);
int rootstream_decode_frame(rootstream_ctx_t *ctx,
                           const uint8_t *in, size_t in_size,
                           frame_buffer_t *out);
void rootstream_decoder_cleanup(rootstream_ctx_t *ctx);

/* --- Display (Phase 1) --- */
int display_init(rootstream_ctx_t *ctx, const char *title,
                int width, int height);
int display_present_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame);
int display_poll_events(rootstream_ctx_t *ctx);
void display_cleanup(rootstream_ctx_t *ctx);

/* --- Audio (Phase 2) --- */
int rootstream_opus_encoder_init(rootstream_ctx_t *ctx);
int rootstream_opus_decoder_init(rootstream_ctx_t *ctx);
int rootstream_opus_encode(rootstream_ctx_t *ctx, const int16_t *pcm,
               uint8_t *out, size_t *out_len);
int rootstream_opus_decode(rootstream_ctx_t *ctx, const uint8_t *in, size_t in_len,
               int16_t *pcm, size_t *pcm_len);
void rootstream_opus_cleanup(rootstream_ctx_t *ctx);
int rootstream_opus_get_frame_size(void);
int rootstream_opus_get_sample_rate(void);
int rootstream_opus_get_channels(void);

/* Audio capture/playback - backward compatibility */
int audio_capture_init(rootstream_ctx_t *ctx);
int audio_capture_frame(rootstream_ctx_t *ctx, int16_t *samples,
                       size_t *num_samples);
void audio_capture_cleanup(rootstream_ctx_t *ctx);

int audio_playback_init(rootstream_ctx_t *ctx);
int audio_playback_write(rootstream_ctx_t *ctx, int16_t *samples,
                        size_t num_samples);
void audio_playback_cleanup(rootstream_ctx_t *ctx);

/* Audio backends - ALSA */
bool audio_capture_alsa_available(void);
int audio_capture_init_alsa(rootstream_ctx_t *ctx);
int audio_capture_frame_alsa(rootstream_ctx_t *ctx, int16_t *samples,
                             size_t *num_samples);
void audio_capture_cleanup_alsa(rootstream_ctx_t *ctx);

bool audio_playback_alsa_available(void);
int audio_playback_init_alsa(rootstream_ctx_t *ctx);
int audio_playback_write_alsa(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t num_samples);
void audio_playback_cleanup_alsa(rootstream_ctx_t *ctx);

/* Audio backends - PulseAudio */
bool audio_capture_pulse_available(void);
int audio_capture_init_pulse(rootstream_ctx_t *ctx);
int audio_capture_frame_pulse(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t *num_samples);
void audio_capture_cleanup_pulse(rootstream_ctx_t *ctx);

bool audio_playback_pulse_available(void);
int audio_playback_init_pulse(rootstream_ctx_t *ctx);
int audio_playback_write_pulse(rootstream_ctx_t *ctx, int16_t *samples,
                               size_t num_samples);
void audio_playback_cleanup_pulse(rootstream_ctx_t *ctx);

/* Audio backends - Dummy (silent/discard) */
int audio_capture_init_dummy(rootstream_ctx_t *ctx);
int audio_capture_frame_dummy(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t *num_samples);
void audio_capture_cleanup_dummy(rootstream_ctx_t *ctx);

int audio_playback_init_dummy(rootstream_ctx_t *ctx);
int audio_playback_write_dummy(rootstream_ctx_t *ctx, int16_t *samples,
                               size_t num_samples);
void audio_playback_cleanup_dummy(rootstream_ctx_t *ctx);

/* --- Recording (Phase 7) --- */
int recording_init(rootstream_ctx_t *ctx, const char *filename);
int recording_write_frame(rootstream_ctx_t *ctx, const uint8_t *data,
                          size_t size, bool is_keyframe);
void recording_cleanup(rootstream_ctx_t *ctx);

/* --- Network --- */
int rootstream_net_init(rootstream_ctx_t *ctx, uint16_t port);
int rootstream_net_send_encrypted(rootstream_ctx_t *ctx, peer_t *peer,
                                  uint8_t type, const void *data, size_t size);
int rootstream_net_send_video(rootstream_ctx_t *ctx, peer_t *peer,
                              const uint8_t *data, size_t size,
                              uint64_t timestamp_us);
int rootstream_net_recv(rootstream_ctx_t *ctx, int timeout_ms);
int rootstream_net_handshake(rootstream_ctx_t *ctx, peer_t *peer);
void rootstream_net_tick(rootstream_ctx_t *ctx);
int rootstream_net_validate_packet(const uint8_t *buffer, size_t len);

/* --- Peer Management --- */
peer_t* rootstream_add_peer(rootstream_ctx_t *ctx, const char *rootstream_code);
peer_t* rootstream_find_peer(rootstream_ctx_t *ctx, const uint8_t *public_key);
void rootstream_remove_peer(rootstream_ctx_t *ctx, peer_t *peer);
int rootstream_connect_to_peer(rootstream_ctx_t *ctx, const char *rootstream_code);

/* --- Control Commands --- */
int rootstream_send_control(rootstream_ctx_t *ctx, peer_t *peer,
                           control_cmd_t cmd, uint32_t value);
int rootstream_pause_stream(rootstream_ctx_t *ctx, peer_t *peer);
int rootstream_resume_stream(rootstream_ctx_t *ctx, peer_t *peer);
int rootstream_request_keyframe(rootstream_ctx_t *ctx, peer_t *peer);

/* --- Discovery --- */
int discovery_init(rootstream_ctx_t *ctx);
int discovery_announce(rootstream_ctx_t *ctx);
int discovery_browse(rootstream_ctx_t *ctx);
void discovery_cleanup(rootstream_ctx_t *ctx);

/* --- Input (existing, polished) --- */
int rootstream_input_init(rootstream_ctx_t *ctx);
int rootstream_input_process(rootstream_ctx_t *ctx, input_event_pkt_t *event);
void rootstream_input_cleanup(rootstream_ctx_t *ctx);

/* --- Tray UI --- */
int tray_init(rootstream_ctx_t *ctx, int argc, char **argv);
void tray_update_status(rootstream_ctx_t *ctx, tray_status_t status);
void tray_show_qr_code(rootstream_ctx_t *ctx);
void tray_show_peers(rootstream_ctx_t *ctx);
void tray_run(rootstream_ctx_t *ctx);
void tray_cleanup(rootstream_ctx_t *ctx);

/* --- Service/Daemon --- */
int service_daemonize(void);
int service_run_host(rootstream_ctx_t *ctx);
int service_run_client(rootstream_ctx_t *ctx);

/* --- QR Code --- */
int qrcode_generate(const char *data, const char *output_file);
int qrcode_display(rootstream_ctx_t *ctx, const char *rootstream_code);
void qrcode_print_terminal(const char *data);

/* --- Configuration --- */
const char* config_get_dir(void);
int config_load(rootstream_ctx_t *ctx);
int config_save(rootstream_ctx_t *ctx);
void config_add_peer_to_history(rootstream_ctx_t *ctx, const char *rootstream_code);

/* --- Utilities --- */
const char* rootstream_get_error(void);
void rootstream_print_stats(rootstream_ctx_t *ctx);
uint64_t get_timestamp_ms(void);
uint64_t get_timestamp_us(void);

/* --- Latency instrumentation --- */
int latency_init(latency_stats_t *stats, size_t capacity, uint64_t report_interval_ms, bool enabled);
void latency_cleanup(latency_stats_t *stats);
void latency_record(latency_stats_t *stats, const latency_sample_t *sample);

#endif /* ROOTSTREAM_H */
