/*
 * tag_entry.c — Tag entry implementation
 */

#include "tag_entry.h"

#include <string.h>

int tag_entry_init(tag_entry_t *t, const char *key, const char *val) {
    if (!t || !key || key[0] == '\0')
        return -1;
    memset(t, 0, sizeof(*t));
    strncpy(t->key, key, TAG_KEY_MAX - 1);
    if (val)
        strncpy(t->value, val, TAG_VAL_MAX - 1);
    t->in_use = true;
    return 0;
}
