/*
 * tag_store.h — 32-slot stream tag store
 *
 * Stores up to TAG_STORE_MAX key=value tags.  Provides set (insert or
 * update), get, remove, clear, and foreach iteration.
 *
 * Keys are case-sensitive.  Setting a key that already exists
 * overwrites its value in-place.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TAG_STORE_H
#define ROOTSTREAM_TAG_STORE_H

#include <stddef.h>

#include "tag_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_STORE_MAX 32 /**< Maximum number of tags */

/** Opaque tag store */
typedef struct tag_store_s tag_store_t;

/**
 * tag_store_create — allocate store
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
tag_store_t *tag_store_create(void);

/**
 * tag_store_destroy — free store
 */
void tag_store_destroy(tag_store_t *s);

/**
 * tag_store_set — set or update a tag
 *
 * @param s    Store
 * @param key  Tag key (non-empty)
 * @param val  Tag value
 * @return     0 on success, -1 on full/invalid
 */
int tag_store_set(tag_store_t *s, const char *key, const char *val);

/**
 * tag_store_get — look up a tag value
 *
 * @param s    Store
 * @param key  Tag key
 * @return     Value string (owned by store), or NULL if not found
 */
const char *tag_store_get(const tag_store_t *s, const char *key);

/**
 * tag_store_remove — remove a tag by key
 *
 * @param s    Store
 * @param key  Tag key
 * @return     0 on success, -1 if not found
 */
int tag_store_remove(tag_store_t *s, const char *key);

/**
 * tag_store_clear — remove all tags
 *
 * @param s  Store
 */
void tag_store_clear(tag_store_t *s);

/**
 * tag_store_count — number of active tags
 */
int tag_store_count(const tag_store_t *s);

/**
 * tag_store_foreach — iterate active tags
 */
void tag_store_foreach(const tag_store_t *s, void (*cb)(const tag_entry_t *e, void *user),
                       void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TAG_STORE_H */
