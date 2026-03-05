/*
 * sr_signal.c — Signal descriptor implementation
 */

#include "sr_signal.h"
#include <string.h>

int sr_signal_init(sr_signal_t    *s,
                   sr_signal_id_t  signal_id,
                   uint8_t         level,
                   sr_source_id_t  source_id,
                   uint64_t        timestamp_us) {
    if (!s) return -1;
    s->signal_id    = signal_id;
    s->level        = level;
    s->source_id    = source_id;
    s->timestamp_us = timestamp_us;
    return 0;
}
