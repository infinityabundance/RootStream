/*
 * health_monitor.c — Health monitor implementation
 */

#include "health_monitor.h"

#include <stdlib.h>
#include <string.h>

struct health_monitor_s {
    health_metric_t metrics[HEALTH_MAX_METRICS];
    bool            used[HEALTH_MAX_METRICS];
    int             count;
};

health_monitor_t *health_monitor_create(void) {
    return calloc(1, sizeof(health_monitor_t));
}

void health_monitor_destroy(health_monitor_t *hm) { free(hm); }

int health_monitor_metric_count(const health_monitor_t *hm) {
    return hm ? hm->count : 0;
}

health_metric_t *health_monitor_register(health_monitor_t *hm,
                                           const char       *name,
                                           hm_kind_t         kind) {
    if (!hm || !name || hm->count >= HEALTH_MAX_METRICS) return NULL;
    /* Reject duplicates */
    for (int i = 0; i < HEALTH_MAX_METRICS; i++) {
        if (hm->used[i] &&
            strncmp(hm->metrics[i].name, name, HEALTH_METRIC_NAME_MAX) == 0)
            return NULL;
    }
    for (int i = 0; i < HEALTH_MAX_METRICS; i++) {
        if (!hm->used[i]) {
            hm_init(&hm->metrics[i], name, kind);
            hm->used[i] = true;
            hm->count++;
            return &hm->metrics[i];
        }
    }
    return NULL;
}

health_metric_t *health_monitor_get(health_monitor_t *hm, const char *name) {
    if (!hm || !name) return NULL;
    for (int i = 0; i < HEALTH_MAX_METRICS; i++) {
        if (hm->used[i] &&
            strncmp(hm->metrics[i].name, name, HEALTH_METRIC_NAME_MAX) == 0)
            return &hm->metrics[i];
    }
    return NULL;
}

int health_monitor_evaluate(health_monitor_t *hm, health_summary_t *out) {
    if (!hm || !out) return -1;
    out->overall   = HM_OK;
    out->n_ok      = 0;
    out->n_warn    = 0;
    out->n_crit    = 0;
    out->n_metrics = hm->count;

    for (int i = 0; i < HEALTH_MAX_METRICS; i++) {
        if (!hm->used[i]) continue;
        hm_level_t lv = hm_evaluate(&hm->metrics[i]);
        if      (lv == HM_CRIT) { out->n_crit++; if (out->overall < HM_CRIT) out->overall = HM_CRIT; }
        else if (lv == HM_WARN) { out->n_warn++; if (out->overall < HM_WARN) out->overall = HM_WARN; }
        else                     { out->n_ok++; }
    }
    return 0;
}

void health_monitor_foreach(health_monitor_t *hm,
                               void (*cb)(const health_metric_t *m, void *user),
                               void *user) {
    if (!hm || !cb) return;
    for (int i = 0; i < HEALTH_MAX_METRICS; i++) {
        if (hm->used[i]) cb(&hm->metrics[i], user);
    }
}
