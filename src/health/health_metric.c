/*
 * health_metric.c — Health metric implementation
 */

#include "health_metric.h"

#include <string.h>
#include <float.h>

int hm_init(health_metric_t *m, const char *name, hm_kind_t kind) {
    if (!m) return -1;
    memset(m, 0, sizeof(*m));
    m->kind = kind;
    if (name) strncpy(m->name, name, HEALTH_METRIC_NAME_MAX - 1);
    return 0;
}

int hm_set_threshold(health_metric_t *m, const hm_threshold_t *t) {
    if (!m || !t) return -1;
    m->thresh        = *t;
    m->has_threshold = true;
    return 0;
}

int hm_set_fval(health_metric_t *m, double v) {
    if (!m) return -1;
    m->value.fval = v;
    return 0;
}

int hm_set_uval(health_metric_t *m, uint64_t v) {
    if (!m) return -1;
    m->value.uval = v;
    return 0;
}

int hm_set_bval(health_metric_t *m, bool v) {
    if (!m) return -1;
    m->value.bval = v;
    return 0;
}

hm_level_t hm_evaluate(const health_metric_t *m) {
    if (!m || !m->has_threshold) return HM_OK;

    double val;
    if (m->kind == HM_BOOLEAN) {
        return m->value.bval ? HM_OK : HM_CRIT;
    } else if (m->kind == HM_COUNTER) {
        val = (double)m->value.uval;
    } else {
        val = m->value.fval;
    }

    /* CRIT bounds take priority */
    if (val <= m->thresh.crit_lo || val >= m->thresh.crit_hi) return HM_CRIT;
    if (val <= m->thresh.warn_lo || val >= m->thresh.warn_hi) return HM_WARN;
    return HM_OK;
}

const char *hm_level_name(hm_level_t l) {
    switch (l) {
    case HM_OK:   return "OK";
    case HM_WARN: return "WARN";
    case HM_CRIT: return "CRIT";
    default:      return "UNKNOWN";
    }
}

const char *hm_kind_name(hm_kind_t k) {
    switch (k) {
    case HM_GAUGE:   return "GAUGE";
    case HM_COUNTER: return "COUNTER";
    case HM_RATE:    return "RATE";
    case HM_BOOLEAN: return "BOOLEAN";
    default:         return "UNKNOWN";
    }
}
