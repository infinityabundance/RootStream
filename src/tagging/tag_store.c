/*
 * tag_store.c — Tag store implementation
 */

#include "tag_store.h"

#include <stdlib.h>
#include <string.h>

struct tag_store_s {
    tag_entry_t entries[TAG_STORE_MAX];
    int count;
};

tag_store_t *tag_store_create(void) {
    return calloc(1, sizeof(tag_store_t));
}

void tag_store_destroy(tag_store_t *s) {
    free(s);
}

int tag_store_count(const tag_store_t *s) {
    return s ? s->count : 0;
}

static int find_key(const tag_store_t *s, const char *key) {
    for (int i = 0; i < TAG_STORE_MAX; i++)
        if (s->entries[i].in_use && strncmp(s->entries[i].key, key, TAG_KEY_MAX) == 0)
            return i;
    return -1;
}

int tag_store_set(tag_store_t *s, const char *key, const char *val) {
    if (!s || !key || key[0] == '\0')
        return -1;

    /* Update existing */
    int slot = find_key(s, key);
    if (slot >= 0) {
        memset(s->entries[slot].value, 0, TAG_VAL_MAX);
        if (val)
            strncpy(s->entries[slot].value, val, TAG_VAL_MAX - 1);
        return 0;
    }

    if (s->count >= TAG_STORE_MAX)
        return -1; /* full */
    for (int i = 0; i < TAG_STORE_MAX; i++) {
        if (!s->entries[i].in_use) {
            tag_entry_init(&s->entries[i], key, val);
            s->count++;
            return 0;
        }
    }
    return -1;
}

const char *tag_store_get(const tag_store_t *s, const char *key) {
    if (!s || !key)
        return NULL;
    int slot = find_key(s, key);
    return (slot >= 0) ? s->entries[slot].value : NULL;
}

int tag_store_remove(tag_store_t *s, const char *key) {
    if (!s || !key)
        return -1;
    int slot = find_key(s, key);
    if (slot < 0)
        return -1;
    memset(&s->entries[slot], 0, sizeof(s->entries[slot]));
    s->count--;
    return 0;
}

void tag_store_clear(tag_store_t *s) {
    if (!s)
        return;
    memset(s->entries, 0, sizeof(s->entries));
    s->count = 0;
}

void tag_store_foreach(const tag_store_t *s, void (*cb)(const tag_entry_t *e, void *user),
                       void *user) {
    if (!s || !cb)
        return;
    for (int i = 0; i < TAG_STORE_MAX; i++)
        if (s->entries[i].in_use)
            cb(&s->entries[i], user);
}
