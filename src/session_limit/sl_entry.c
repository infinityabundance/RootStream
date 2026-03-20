/*
 * sl_entry.c — Session entry implementation
 */

#include "sl_entry.h"

#include <string.h>

int sl_entry_init(sl_entry_t *e, uint64_t session_id, const char *remote_ip, uint64_t start_us) {
    if (!e)
        return -1;
    memset(e, 0, sizeof(*e));
    e->session_id = session_id;
    e->start_us = start_us;
    e->state = SL_CONNECTING;
    e->in_use = true;
    if (remote_ip)
        strncpy(e->remote_ip, remote_ip, SL_IP_MAX - 1);
    return 0;
}

const char *sl_state_name(sl_state_t s) {
    switch (s) {
        case SL_CONNECTING:
            return "CONNECTING";
        case SL_ACTIVE:
            return "ACTIVE";
        case SL_CLOSING:
            return "CLOSING";
        default:
            return "UNKNOWN";
    }
}
