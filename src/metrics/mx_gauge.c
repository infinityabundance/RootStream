/*
 * mx_gauge.c — Named integer gauge implementation
 */

#include "mx_gauge.h"
#include <string.h>

int mx_gauge_init(mx_gauge_t *g, const char *name) {
    if (!g || !name || name[0] == '\0') return -1;
    memset(g, 0, sizeof(*g));
    strncpy(g->name, name, MX_GAUGE_NAME_MAX - 1);
    g->name[MX_GAUGE_NAME_MAX - 1] = '\0';
    g->in_use = 1;
    return 0;
}

void mx_gauge_set(mx_gauge_t *g, int64_t v)      { if (g) g->value  = v; }
void mx_gauge_add(mx_gauge_t *g, int64_t delta)  { if (g) g->value += delta; }
int64_t mx_gauge_get(const mx_gauge_t *g)        { return g ? g->value : 0; }
void mx_gauge_reset(mx_gauge_t *g)               { if (g) g->value  = 0; }
