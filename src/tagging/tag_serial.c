/*
 * tag_serial.c — Tag store serialisation / deserialisation
 */

#include "tag_serial.h"
#include "tag_entry.h"
#include <string.h>
#include <stdio.h>

/* Write context passed through foreach callback */
typedef struct {
    char  *buf;
    size_t rem;
    int    total;
} write_ctx_t;

static void write_one(const tag_entry_t *e, void *user) {
    write_ctx_t *c = (write_ctx_t *)user;
    int n = snprintf(c->buf + c->total, c->rem,
                     "%s=%s\n", e->key, e->value);
    if (n > 0 && (size_t)n < c->rem) {
        c->total += n;
        c->rem   -= (size_t)n;
    }
}

int tag_serial_write(const tag_store_t *s, char *buf, size_t len) {
    if (!s || !buf || len == 0) return -1;
    write_ctx_t ctx = { buf, len, 0 };
    tag_store_foreach(s, write_one, &ctx);
    if ((size_t)ctx.total < len) buf[ctx.total] = '\0';
    return ctx.total;
}

int tag_serial_read(tag_store_t *s, char *buf) {
    if (!s || !buf) return -1;
    int count = 0;
    char *line = buf;
    char *end;

    while (line && *line) {
        /* Find end of line */
        end = strchr(line, '\n');
        if (end) *end = '\0';

        char *eq = strchr(line, '=');
        if (eq && eq != line) {   /* has '=' and key is non-empty */
            *eq = '\0';
            const char *key = line;
            const char *val = eq + 1;
            if (tag_store_set(s, key, val) == 0) count++;
            *eq = '=';  /* restore for caller */
        }
        line = end ? end + 1 : NULL;
    }
    return count;
}
