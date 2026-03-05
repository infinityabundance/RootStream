/*
 * output_target.h — Single output target descriptor
 *
 * Represents one streaming output endpoint: a URL (e.g. rtmp://…),
 * protocol tag, and current connection state.  The target is a plain
 * value type; the output_registry owns instances.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_OUTPUT_TARGET_H
#define ROOTSTREAM_OUTPUT_TARGET_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OUTPUT_URL_MAX      256   /**< Maximum URL length (incl. NUL) */
#define OUTPUT_PROTO_MAX    16    /**< Maximum protocol tag length */
#define OUTPUT_NAME_MAX     64    /**< Maximum friendly-name length */

/** Output target state */
typedef enum {
    OT_IDLE    = 0,   /**< Registered but not yet connected */
    OT_ACTIVE  = 1,   /**< Connected and streaming */
    OT_ERROR   = 2,   /**< Last connection attempt failed */
    OT_DISABLED = 3,  /**< Explicitly disabled by caller */
} ot_state_t;

/** Single output target */
typedef struct {
    char        name[OUTPUT_NAME_MAX];
    char        url[OUTPUT_URL_MAX];
    char        protocol[OUTPUT_PROTO_MAX];  /**< e.g. "rtmp", "srt", "hls" */
    ot_state_t  state;
    uint64_t    connect_time_us;  /**< Timestamp of last successful connect */
    bool        enabled;
} output_target_t;

/**
 * ot_init — initialise target
 *
 * @param t        Target to initialise
 * @param name     Friendly name (truncated to OUTPUT_NAME_MAX-1)
 * @param url      Endpoint URL (truncated to OUTPUT_URL_MAX-1)
 * @param protocol Protocol tag (truncated to OUTPUT_PROTO_MAX-1)
 * @return         0 on success, -1 on NULL
 */
int ot_init(output_target_t *t,
              const char *name,
              const char *url,
              const char *protocol);

/**
 * ot_state_name — human-readable state string
 */
const char *ot_state_name(ot_state_t s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_OUTPUT_TARGET_H */
