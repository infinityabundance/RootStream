/*
 * tag_serial.h — Tag store serialisation / deserialisation
 *
 * Serialises a tag_store_t to a NUL-terminated text buffer in the
 * format:
 *
 *   key1=value1\n
 *   key2=value2\n
 *   …
 *
 * and parses such a buffer back into a tag_store_t.  Lines that
 * contain no '=' character are silently skipped.  Lines with an empty
 * key are skipped.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TAG_SERIAL_H
#define ROOTSTREAM_TAG_SERIAL_H

#include "tag_store.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * tag_serial_write — serialise store to text buffer
 *
 * @param s    Source store
 * @param buf  Output buffer
 * @param len  Buffer size in bytes
 * @return     Number of bytes written (excl. NUL), or -1 on error
 */
int tag_serial_write(const tag_store_t *s, char *buf, size_t len);

/**
 * tag_serial_read — parse text buffer into store
 *
 * Existing tags in the store are NOT cleared first; use
 * tag_store_clear() before calling if you want a fresh load.
 *
 * @param s    Destination store
 * @param buf  NUL-terminated input text (modified internally by strtok)
 * @return     Number of tags successfully parsed, or -1 on error
 */
int tag_serial_read(tag_store_t *s, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TAG_SERIAL_H */
