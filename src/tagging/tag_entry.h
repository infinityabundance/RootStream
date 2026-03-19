/*
 * tag_entry.h — Single stream tag (key=value pair)
 *
 * A tag has a short key (up to TAG_KEY_MAX-1 bytes) and a value
 * (up to TAG_VAL_MAX-1 bytes).  Tags are used for runtime metadata
 * annotation of streams (e.g. "title=My Stream", "game=FPS").
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_TAG_ENTRY_H
#define ROOTSTREAM_TAG_ENTRY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_KEY_MAX 32  /**< Max key length (incl. NUL) */
#define TAG_VAL_MAX 128 /**< Max value length (incl. NUL) */

/** Single key=value tag */
typedef struct {
    char key[TAG_KEY_MAX];
    char value[TAG_VAL_MAX];
    bool in_use;
} tag_entry_t;

/**
 * tag_entry_init — initialise a tag entry
 *
 * @param t    Entry
 * @param key  Tag key (truncated to TAG_KEY_MAX-1; must not be empty)
 * @param val  Tag value (truncated to TAG_VAL_MAX-1)
 * @return     0 on success, -1 on NULL or empty key
 */
int tag_entry_init(tag_entry_t *t, const char *key, const char *val);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TAG_ENTRY_H */
