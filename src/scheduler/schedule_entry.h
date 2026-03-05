/*
 * schedule_entry.h — Stream schedule entry format
 *
 * A schedule entry describes one planned streaming event: when it
 * starts, how long it runs, what source to activate, and an optional
 * human-readable title.
 *
 * Wire encoding (little-endian, used for persistence)
 * ────────────────────────────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic   0x5343454E ('SCEN')
 *   4      8     start_us  — wall-clock start (µs since Unix epoch)
 *  12      4     duration_us — planned duration (µs); 0 = until stopped
 *  16      1     source_type (schedule_source_t)
 *  17      1     flags
 *  18      2     title_len (bytes)
 *  20      N     title (UTF-8, <= SCHEDULE_MAX_TITLE bytes)
 */

#ifndef ROOTSTREAM_SCHEDULE_ENTRY_H
#define ROOTSTREAM_SCHEDULE_ENTRY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULE_MAGIC         0x5343454EUL  /* 'SCEN' */
#define SCHEDULE_MAX_TITLE     128
#define SCHEDULE_HDR_SIZE      20
#define SCHEDULE_ENTRY_MAX_SZ  (SCHEDULE_HDR_SIZE + SCHEDULE_MAX_TITLE)

/** Stream source type for a scheduled entry */
typedef enum {
    SCHED_SOURCE_CAPTURE  = 0,  /**< Live display/camera capture */
    SCHED_SOURCE_FILE     = 1,  /**< Pre-recorded file playback */
    SCHED_SOURCE_PLAYLIST = 2,  /**< Playlist item */
    SCHED_SOURCE_TEST     = 3,  /**< Test pattern / loopback */
} schedule_source_t;

/** Schedule entry flags */
#define SCHED_FLAG_REPEAT   0x01  /**< Repeat entry daily */
#define SCHED_FLAG_ENABLED  0x02  /**< Entry is active (not disabled) */

/** A single schedule entry */
typedef struct {
    uint64_t          start_us;     /**< Wall-clock start time (µs epoch) */
    uint32_t          duration_us;  /**< Duration in µs; 0 = until stopped */
    schedule_source_t source_type;
    uint8_t           flags;
    uint16_t          title_len;
    char              title[SCHEDULE_MAX_TITLE + 1];  /**< NUL-terminated */
    uint64_t          id;           /**< Assigned by scheduler on add */
} schedule_entry_t;

/**
 * schedule_entry_encode — serialise @entry into @buf
 *
 * @param entry   Entry to encode
 * @param buf     Output buffer (>= SCHEDULE_ENTRY_MAX_SZ bytes)
 * @param buf_sz  Size of @buf
 * @return        Bytes written, or -1 on error
 */
int schedule_entry_encode(const schedule_entry_t *entry,
                           uint8_t                *buf,
                           size_t                  buf_sz);

/**
 * schedule_entry_decode — parse @entry from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param entry   Output entry
 * @return        0 on success, -1 on parse error
 */
int schedule_entry_decode(const uint8_t    *buf,
                           size_t            buf_sz,
                           schedule_entry_t *entry);

/**
 * schedule_entry_encoded_size — return serialised byte count for @entry
 */
size_t schedule_entry_encoded_size(const schedule_entry_t *entry);

/**
 * schedule_entry_is_enabled — return true if SCHED_FLAG_ENABLED is set
 */
bool schedule_entry_is_enabled(const schedule_entry_t *entry);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SCHEDULE_ENTRY_H */
