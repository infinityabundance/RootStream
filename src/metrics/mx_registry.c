/*
 * mx_registry.c — Named gauge registry implementation
 */

#include "mx_registry.h"
#include <stdlib.h>
#include <string.h>

struct mx_registry_s {
    mx_gauge_t gauges[MX_MAX_GAUGES];
    int        count;
};

mx_registry_t *mx_registry_create(void)  { return calloc(1, sizeof(mx_registry_t)); }
void           mx_registry_destroy(mx_registry_t *r) { free(r); }
int            mx_registry_count(const mx_registry_t *r) { return r ? r->count : 0; }

mx_gauge_t *mx_registry_register(mx_registry_t *r, const char *name) {
    if (!r || !name || name[0] == '\0' || r->count >= MX_MAX_GAUGES) return NULL;
    /* Reject duplicates */
    for (int i = 0; i < MX_MAX_GAUGES; i++)
        if (r->gauges[i].in_use &&
            strncmp(r->gauges[i].name, name, MX_GAUGE_NAME_MAX) == 0)
            return NULL;
    for (int i = 0; i < MX_MAX_GAUGES; i++) {
        if (!r->gauges[i].in_use) {
            if (mx_gauge_init(&r->gauges[i], name) != 0) return NULL;
            r->count++;
            return &r->gauges[i];
        }
    }
    return NULL;
}

mx_gauge_t *mx_registry_lookup(mx_registry_t *r, const char *name) {
    if (!r || !name) return NULL;
    for (int i = 0; i < MX_MAX_GAUGES; i++)
        if (r->gauges[i].in_use &&
            strncmp(r->gauges[i].name, name, MX_GAUGE_NAME_MAX) == 0)
            return &r->gauges[i];
    return NULL;
}

int mx_registry_snapshot_all(const mx_registry_t *r,
                              mx_gauge_t          *out,
                              int                  max_out) {
    if (!r || !out || max_out <= 0) return 0;
    int n = 0;
    for (int i = 0; i < MX_MAX_GAUGES && n < max_out; i++)
        if (r->gauges[i].in_use) out[n++] = r->gauges[i];
    return n;
}
