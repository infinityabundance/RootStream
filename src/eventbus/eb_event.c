/*
 * eb_event.c — Event descriptor implementation
 */

#include "eb_event.h"
#include <string.h>

int eb_event_init(eb_event_t *e,
                  eb_type_t   type_id,
                  void       *payload,
                  size_t      payload_len,
                  uint64_t    timestamp_us) {
    if (!e) return -1;
    e->type_id      = type_id;
    e->payload      = payload;
    e->payload_len  = payload_len;
    e->timestamp_us = timestamp_us;
    return 0;
}
