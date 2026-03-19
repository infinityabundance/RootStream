/*
 * health_metric.h — Typed metric for the stream health monitor
 *
 * Supports four metric kinds:
 *   GAUGE    — instantaneous floating-point value (e.g. CPU %)
 *   COUNTER  — monotonically increasing uint64 (e.g. total packets sent)
 *   RATE     — floating-point per-second rate (e.g. bitrate bps)
 *   BOOLEAN  — 0/1 flag (e.g. "encoder running")
 *
 * Each metric carries optional threshold bounds that determine whether
 * its contribution to the overall health level is OK / WARN / CRIT.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_HEALTH_METRIC_H
#define ROOTSTREAM_HEALTH_METRIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEALTH_METRIC_NAME_MAX 48 /**< Max metric name (incl. NUL) */

/** Metric kind */
typedef enum {
    HM_GAUGE = 0,
    HM_COUNTER = 1,
    HM_RATE = 2,
    HM_BOOLEAN = 3,
} hm_kind_t;

/** Health level for a single metric */
typedef enum {
    HM_OK = 0,
    HM_WARN = 1,
    HM_CRIT = 2,
} hm_level_t;

/** Metric threshold: warn_lo ≤ ok ≤ warn_hi; outside → WARN/CRIT */
typedef struct {
    double warn_lo; /**< Value below this → WARN  (use -DBL_MAX to disable) */
    double warn_hi; /**< Value above this → WARN  (use +DBL_MAX to disable) */
    double crit_lo; /**< Value below this → CRIT  (use -DBL_MAX to disable) */
    double crit_hi; /**< Value above this → CRIT  (use +DBL_MAX to disable) */
} hm_threshold_t;

/** Metric value union */
typedef union {
    double fval;   /**< GAUGE / RATE */
    uint64_t uval; /**< COUNTER */
    bool bval;     /**< BOOLEAN */
} hm_value_t;

/** Single health metric */
typedef struct {
    char name[HEALTH_METRIC_NAME_MAX];
    hm_kind_t kind;
    hm_value_t value;
    hm_threshold_t thresh;
    bool has_threshold;
} health_metric_t;

/**
 * hm_init — initialise metric
 *
 * @param m     Metric to initialise
 * @param name  Name string (truncated to HEALTH_METRIC_NAME_MAX-1)
 * @param kind  Metric kind
 * @return      0 on success, -1 on NULL
 */
int hm_init(health_metric_t *m, const char *name, hm_kind_t kind);

/**
 * hm_set_threshold — attach threshold bounds
 *
 * @param m  Metric
 * @param t  Threshold
 * @return   0 on success, -1 on NULL
 */
int hm_set_threshold(health_metric_t *m, const hm_threshold_t *t);

/**
 * hm_set_fval / hm_set_uval / hm_set_bval — update metric value
 */
int hm_set_fval(health_metric_t *m, double v);
int hm_set_uval(health_metric_t *m, uint64_t v);
int hm_set_bval(health_metric_t *m, bool v);

/**
 * hm_evaluate — compute health level for the metric
 *
 * BOOLEAN: false → CRIT when has_threshold, else OK.
 * GAUGE/RATE/COUNTER: compared against crit/warn bounds.
 *
 * @param m  Metric
 * @return   HM_OK, HM_WARN, or HM_CRIT
 */
hm_level_t hm_evaluate(const health_metric_t *m);

/**
 * hm_level_name — human-readable level string
 */
const char *hm_level_name(hm_level_t l);

/**
 * hm_kind_name — human-readable kind string
 */
const char *hm_kind_name(hm_kind_t k);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HEALTH_METRIC_H */
