/*
 * event_export.h — JSON and plain-text export of an event ring
 *
 * Renders the contents of an event_ring_t into a caller-supplied
 * buffer in JSON array format or plain-text (one line per entry).
 *
 * Thread-safety: stateless — thread-safe provided the ring is not
 *                mutated during export.
 */

#ifndef ROOTSTREAM_EVENT_EXPORT_H
#define ROOTSTREAM_EVENT_EXPORT_H

#include <stddef.h>

#include "event_ring.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * event_export_json — render ring as JSON array into @buf
 *
 * Each entry is rendered as:
 *   {"ts_us":NNN,"level":"INFO","type":0,"msg":"..."}
 *
 * @param r       Ring to export (newest first)
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written (excl. NUL), or -1 if buffer too small
 */
int event_export_json(const event_ring_t *r, char *buf, size_t buf_sz);

/**
 * event_export_text — render ring as plain text (one line per entry)
 *
 * Format: "[LEVEL] <ts_us> (type=NNN) msg\n"
 *
 * @param r       Ring to export (newest first)
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written (excl. NUL), or -1 if buffer too small
 */
int event_export_text(const event_ring_t *r, char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EVENT_EXPORT_H */
