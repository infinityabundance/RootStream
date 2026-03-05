/*
 * metadata_store.c — In-memory KV metadata store implementation
 *
 * Linear-scan array.  Suitable for up to METADATA_STORE_CAPACITY
 * entries; for larger stores a hash table would be preferable.
 */

#include "metadata_store.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char  key[METADATA_KV_MAX_KEY + 1];
    char  val[METADATA_KV_MAX_VAL + 1];
    bool  valid;
} kv_entry_t;

struct metadata_store_s {
    kv_entry_t entries[METADATA_STORE_CAPACITY];
    size_t     count;
};

metadata_store_t *metadata_store_create(void) {
    return calloc(1, sizeof(metadata_store_t));
}

void metadata_store_destroy(metadata_store_t *store) {
    free(store);
}

size_t metadata_store_count(const metadata_store_t *store) {
    return store ? store->count : 0;
}

void metadata_store_clear(metadata_store_t *store) {
    if (!store) return;
    memset(store->entries, 0, sizeof(store->entries));
    store->count = 0;
}

bool metadata_store_has(const metadata_store_t *store, const char *key) {
    return metadata_store_get(store, key) != NULL;
}

int metadata_store_set(metadata_store_t *store,
                        const char        *key,
                        const char        *value) {
    if (!store || !key || !value) return -1;
    if (strlen(key) > METADATA_KV_MAX_KEY) return -1;

    /* Update existing entry */
    for (size_t i = 0; i < METADATA_STORE_CAPACITY; i++) {
        if (store->entries[i].valid &&
            strcmp(store->entries[i].key, key) == 0) {
            snprintf(store->entries[i].val, sizeof(store->entries[i].val),
                     "%s", value);
            return 0;
        }
    }

    /* Insert new entry */
    if (store->count >= METADATA_STORE_CAPACITY) return -1;
    for (size_t i = 0; i < METADATA_STORE_CAPACITY; i++) {
        if (!store->entries[i].valid) {
            snprintf(store->entries[i].key, sizeof(store->entries[i].key), "%s", key);
            snprintf(store->entries[i].val, sizeof(store->entries[i].val), "%s", value);
            store->entries[i].valid = true;
            store->count++;
            return 0;
        }
    }
    return -1;
}

const char *metadata_store_get(const metadata_store_t *store, const char *key) {
    if (!store || !key) return NULL;
    for (size_t i = 0; i < METADATA_STORE_CAPACITY; i++) {
        if (store->entries[i].valid &&
            strcmp(store->entries[i].key, key) == 0)
            return store->entries[i].val;
    }
    return NULL;
}

int metadata_store_delete(metadata_store_t *store, const char *key) {
    if (!store || !key) return -1;
    for (size_t i = 0; i < METADATA_STORE_CAPACITY; i++) {
        if (store->entries[i].valid &&
            strcmp(store->entries[i].key, key) == 0) {
            store->entries[i].valid = false;
            store->count--;
            return 0;
        }
    }
    return -1;
}

void metadata_store_foreach(const metadata_store_t *store,
                              int (*cb)(const char *key,
                                        const char *value,
                                        void       *ud),
                              void *ud) {
    if (!store || !cb) return;
    for (size_t i = 0; i < METADATA_STORE_CAPACITY; i++) {
        if (store->entries[i].valid) {
            if (cb(store->entries[i].key, store->entries[i].val, ud) != 0)
                break;
        }
    }
}
