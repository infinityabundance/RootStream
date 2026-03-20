/*
 * health_report.c — JSON health serialiser
 */

#include "health_report.h"

#include <stdio.h>
#include <string.h>

#include "health_metric.h"

typedef struct {
    char *buf;
    size_t sz;
    int off;
    int first;
} report_ctx_t;

static void emit_metric_cb(const health_metric_t *m, void *ud) {
    report_ctx_t *c = (report_ctx_t *)ud;
    if (!c->first) {
        int n = snprintf(c->buf + c->off, c->sz > (size_t)c->off ? c->sz - (size_t)c->off : 0, ",");
        if (n > 0)
            c->off += n;
    }
    c->first = 0;

    hm_level_t lv = hm_evaluate(m);
    char val_str[32];
    if (m->kind == HM_BOOLEAN)
        snprintf(val_str, sizeof(val_str), "%d", (int)m->value.bval);
    else if (m->kind == HM_COUNTER)
        snprintf(val_str, sizeof(val_str), "%llu", (unsigned long long)m->value.uval);
    else
        snprintf(val_str, sizeof(val_str), "%.4g", m->value.fval);

    int n = snprintf(c->buf + c->off, c->sz > (size_t)c->off ? c->sz - (size_t)c->off : 0,
                     "\n    { \"name\": \"%s\", \"kind\": \"%s\","
                     " \"level\": \"%s\", \"value\": %s }",
                     m->name, hm_kind_name(m->kind), hm_level_name(lv), val_str);
    if (n > 0)
        c->off += n;
}

int health_report_json(health_monitor_t *hm, char *buf, size_t buf_sz) {
    if (!hm || !buf || buf_sz == 0)
        return -1;

    health_summary_t sum;
    if (health_monitor_evaluate(hm, &sum) < 0)
        return -1;

    int off = 0;

#define APPEND(fmt, ...)                                                                   \
    do {                                                                                   \
        int _n = snprintf(buf + off, buf_sz > (size_t)off ? buf_sz - (size_t)off : 0, fmt, \
                          ##__VA_ARGS__);                                                  \
        if (_n > 0)                                                                        \
            off += _n;                                                                     \
    } while (0)

    APPEND("{\n  \"overall\": \"%s\",\n", hm_level_name(sum.overall));
    APPEND("  \"n_ok\": %d, \"n_warn\": %d, \"n_crit\": %d,\n", sum.n_ok, sum.n_warn, sum.n_crit);
    APPEND("  \"metrics\": [");

    report_ctx_t ctx = {buf, buf_sz, off, 1};
    health_monitor_foreach(hm, emit_metric_cb, &ctx);
    off = ctx.off;

    APPEND("\n  ]\n}\n");

#undef APPEND

    if ((size_t)off >= buf_sz)
        buf[buf_sz - 1] = '\0';
    return off;
}
