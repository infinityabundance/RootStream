/*
 * hr_entry.c — Plugin hot-reload entry implementation
 */

#include "hr_entry.h"

#include <string.h>

int hr_entry_init(hr_entry_t *e, const char *path) {
    if (!e)
        return -1;
    memset(e, 0, sizeof(*e));
    e->state = HR_STATE_UNLOADED;
    if (path)
        strncpy(e->path, path, HR_PATH_MAX - 1);
    return 0;
}

void hr_entry_clear(hr_entry_t *e) {
    if (!e)
        return;
    char path[HR_PATH_MAX];
    strncpy(path, e->path, HR_PATH_MAX);
    memset(e, 0, sizeof(*e));
    strncpy(e->path, path, HR_PATH_MAX);
    e->state = HR_STATE_UNLOADED;
}

const char *hr_state_name(hr_state_t s) {
    switch (s) {
        case HR_STATE_UNLOADED:
            return "UNLOADED";
        case HR_STATE_LOADED:
            return "LOADED";
        case HR_STATE_FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
    }
}
