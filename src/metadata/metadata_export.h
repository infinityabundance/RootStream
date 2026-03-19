/*
 * metadata_export.h — JSON serialisation of stream metadata and KV store
 *
 * Renders stream metadata and KV store contents into caller-supplied
 * buffers.  No heap allocations are performed.
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_METADATA_EXPORT_H
#define ROOTSTREAM_METADATA_EXPORT_H

#include <stddef.h>

#include "metadata_store.h"
#include "stream_metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * metadata_export_json — render @meta as JSON into @buf
 *
 * @param meta    Metadata to render
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written (excl. NUL), or -1 if buf too small
 */
int metadata_export_json(const stream_metadata_t *meta, char *buf, size_t buf_sz);

/**
 * metadata_store_export_json — render @store as a JSON object into @buf
 *
 * Only key-value pairs are emitted; e.g. {"song":"Title","viewers":"42"}
 *
 * @param store   Store to render
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 if buf too small
 */
int metadata_store_export_json(const metadata_store_t *store, char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_METADATA_EXPORT_H */
