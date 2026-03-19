/*
 * output_target.c — Output target implementation
 */

#include "output_target.h"

#include <string.h>

int ot_init(output_target_t *t, const char *name, const char *url, const char *protocol) {
    if (!t)
        return -1;
    memset(t, 0, sizeof(*t));
    t->state = OT_IDLE;
    t->enabled = true;
    if (name)
        strncpy(t->name, name, OUTPUT_NAME_MAX - 1);
    if (url)
        strncpy(t->url, url, OUTPUT_URL_MAX - 1);
    if (protocol)
        strncpy(t->protocol, protocol, OUTPUT_PROTO_MAX - 1);
    return 0;
}

const char *ot_state_name(ot_state_t s) {
    switch (s) {
        case OT_IDLE:
            return "IDLE";
        case OT_ACTIVE:
            return "ACTIVE";
        case OT_ERROR:
            return "ERROR";
        case OT_DISABLED:
            return "DISABLED";
        default:
            return "UNKNOWN";
    }
}
