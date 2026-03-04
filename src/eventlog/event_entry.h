/*
 * event_entry.h — Timestamped event log entry
 *
 * An event entry records a single diagnostic event with:
 *   - Monotonic timestamp (µs)
 *   - Log level (DEBUG / INFO / WARN / ERROR)
 *   - Event type code (application-defined uint16)
 *   - Message string (up to EVENT_MSG_MAX - 1 chars, NUL-terminated)
 *
 * Wire layout (little-endian) for serialisation
 * ──────────────────────────────────────────────
 *  Offset  Size  Field
 *   0      8     timestamp_us
 *   8      1     level
 *   9      1     reserved (0)
 *  10      2     event_type
 *  12      2     msg_len (including NUL)
 *  14      2     reserved (0)
 *  16      N     msg (msg_len bytes, NUL-terminated)
 *
 * Thread-safety: stateless encode/decode — thread-safe.
 */

#ifndef ROOTSTREAM_EVENT_ENTRY_H
#define ROOTSTREAM_EVENT_ENTRY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_ENTRY_HDR_SIZE  16
#define EVENT_MSG_MAX         128   /**< Max message length including NUL */

/** Log level */
typedef enum {
    EVENT_LEVEL_DEBUG = 0,
    EVENT_LEVEL_INFO  = 1,
    EVENT_LEVEL_WARN  = 2,
    EVENT_LEVEL_ERROR = 3,
} event_level_t;

/** Event log entry */
typedef struct {
    uint64_t      timestamp_us;
    event_level_t level;
    uint16_t      event_type;
    char          msg[EVENT_MSG_MAX];  /**< NUL-terminated message */
} event_entry_t;

/**
 * event_entry_encode — serialise @e into @buf
 *
 * @param e       Entry to encode
 * @param buf     Output buffer (>= EVENT_ENTRY_HDR_SIZE + strlen(msg) + 1)
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 on error
 */
int event_entry_encode(const event_entry_t *e,
                         uint8_t             *buf,
                         size_t               buf_sz);

/**
 * event_entry_decode — parse @e from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes
 * @param e       Output entry
 * @return        0 on success, -1 on error
 */
int event_entry_decode(const uint8_t *buf, size_t buf_sz, event_entry_t *e);

/**
 * event_level_name — human-readable level string
 *
 * @param l  Level
 * @return   Static string
 */
const char *event_level_name(event_level_t l);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EVENT_ENTRY_H */
