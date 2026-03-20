/*
 * event_export.c — Event ring JSON / text export
 */

#include "event_export.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int event_export_json(const event_ring_t *r, char *buf, size_t buf_sz) {
    if (!r || !buf || buf_sz < 3)
        return -1;

    size_t pos = 0;
    int n;

#define APPEND(fmt, ...)                                           \
    do {                                                           \
        n = snprintf(buf + pos, buf_sz - pos, fmt, ##__VA_ARGS__); \
        if (n < 0 || (size_t)n >= buf_sz - pos)                    \
            return -1;                                             \
        pos += (size_t)n;                                          \
    } while (0)

    APPEND("[");

    int count = event_ring_count(r);
    for (int age = 0; age < count; age++) {
        event_entry_t e;
        event_ring_get(r, age, &e);

        if (age > 0)
            APPEND(",");
        APPEND("{\"ts_us\":%" PRIu64 ",\"level\":\"%s\",\"type\":%u,\"msg\":\"%s\"}",
               e.timestamp_us, event_level_name(e.level), (unsigned)e.event_type, e.msg);
    }

    APPEND("]");
#undef APPEND
    return (int)pos;
}

int event_export_text(const event_ring_t *r, char *buf, size_t buf_sz) {
    if (!r || !buf || buf_sz < 2)
        return -1;

    size_t pos = 0;
    int n;

#define APPEND(fmt, ...)                                           \
    do {                                                           \
        n = snprintf(buf + pos, buf_sz - pos, fmt, ##__VA_ARGS__); \
        if (n < 0 || (size_t)n >= buf_sz - pos)                    \
            return -1;                                             \
        pos += (size_t)n;                                          \
    } while (0)

    int count = event_ring_count(r);
    for (int age = 0; age < count; age++) {
        event_entry_t e;
        event_ring_get(r, age, &e);
        APPEND("[%s] %" PRIu64 " (type=%u) %s\n", event_level_name(e.level), e.timestamp_us,
               (unsigned)e.event_type, e.msg);
    }

#undef APPEND
    buf[pos] = '\0';
    return (int)pos;
}
