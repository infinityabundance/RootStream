/*
 * metadata_store.h — In-memory key-value metadata store
 *
 * Provides a simple string→string key-value store for dynamic stream
 * metadata (e.g. current song, viewer count, custom labels).
 *
 * Keys and values are NUL-terminated strings up to METADATA_KV_MAX_KEY
 * and METADATA_KV_MAX_VAL bytes respectively.
 *
 * Capacity: METADATA_STORE_CAPACITY entries; inserting beyond capacity
 * returns an error.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_METADATA_STORE_H
#define ROOTSTREAM_METADATA_STORE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METADATA_KV_MAX_KEY        64
#define METADATA_KV_MAX_VAL        256
#define METADATA_STORE_CAPACITY    128

/** Opaque metadata store */
typedef struct metadata_store_s metadata_store_t;

/**
 * metadata_store_create — allocate empty store
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
metadata_store_t *metadata_store_create(void);

/**
 * metadata_store_destroy — free store
 *
 * @param store  Store to destroy
 */
void metadata_store_destroy(metadata_store_t *store);

/**
 * metadata_store_set — insert or update a key-value pair
 *
 * @param store  Store
 * @param key    Key string
 * @param value  Value string
 * @return       0 on success, -1 on full / NULL args / key too long
 */
int metadata_store_set(metadata_store_t *store,
                        const char        *key,
                        const char        *value);

/**
 * metadata_store_get — look up a value by key
 *
 * @param store  Store
 * @param key    Key to look up
 * @return       Pointer to stored value string, or NULL if not found
 */
const char *metadata_store_get(const metadata_store_t *store,
                                 const char              *key);

/**
 * metadata_store_delete — remove a key
 *
 * @param store  Store
 * @param key    Key to remove
 * @return       0 on success, -1 if not found
 */
int metadata_store_delete(metadata_store_t *store, const char *key);

/**
 * metadata_store_count — number of entries
 *
 * @param store  Store
 * @return       Entry count
 */
size_t metadata_store_count(const metadata_store_t *store);

/**
 * metadata_store_clear — remove all entries
 *
 * @param store  Store
 */
void metadata_store_clear(metadata_store_t *store);

/**
 * metadata_store_has — return true if @key exists
 *
 * @param store  Store
 * @param key    Key to check
 * @return       true if key exists
 */
bool metadata_store_has(const metadata_store_t *store, const char *key);

/**
 * metadata_store_foreach — iterate all key-value pairs
 *
 * Calls @cb for each entry.  Iteration stops early if @cb returns non-zero.
 *
 * @param store  Store to iterate
 * @param cb     Callback: fn(key, value, userdata)
 * @param ud     User data passed to @cb
 */
void metadata_store_foreach(const metadata_store_t *store,
                              int (*cb)(const char *key,
                                        const char *value,
                                        void       *ud),
                              void *ud);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_METADATA_STORE_H */
