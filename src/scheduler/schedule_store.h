/*
 * schedule_store.h — JSON-like persistence for schedule entries
 *
 * Saves and loads a set of schedule_entry_t items to/from a file using
 * the binary format defined in schedule_entry.h.  The file is a simple
 * concatenation of encoded entries prefixed by a 12-byte file header:
 *
 *   Offset  Size  Field
 *    0      4     File magic  0x52535348 ('RSSH')
 *    4      2     Version (1)
 *    6      2     Entry count
 *    8      4     Reserved
 *   12      ...   Entries (each length-prefixed: 2-byte le entry_size + entry)
 */

#ifndef ROOTSTREAM_SCHEDULE_STORE_H
#define ROOTSTREAM_SCHEDULE_STORE_H

#include <stddef.h>

#include "schedule_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULE_STORE_MAGIC 0x52535348UL /* 'RSSH' */
#define SCHEDULE_STORE_VERSION 1
#define SCHEDULE_STORE_HDR_SZ 12
#define SCHEDULE_STORE_MAX_ENTRIES SCHEDULER_MAX_ENTRIES

/**
 * schedule_store_save — write @n entries to @path
 *
 * Writes atomically via a temp file + rename.
 *
 * @param path     Destination file path
 * @param entries  Array of entries to save
 * @param n        Number of entries
 * @return         0 on success, -1 on I/O error
 */
int schedule_store_save(const char *path, const schedule_entry_t *entries, size_t n);

/**
 * schedule_store_load — read entries from @path into @entries
 *
 * @param path       Source file path
 * @param entries    Output array (must have capacity >= @max)
 * @param max        Maximum entries to read
 * @param out_count  Receives actual count loaded
 * @return           0 on success, -1 on I/O or parse error
 */
int schedule_store_load(const char *path, schedule_entry_t *entries, size_t max, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SCHEDULE_STORE_H */
