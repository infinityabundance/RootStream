/*
 * health_monitor.h — Stream health monitor
 *
 * Maintains a registry of up to HEALTH_MAX_METRICS health_metric_t
 * instances.  On each evaluation pass the monitor:
 *   1. Evaluates every registered metric against its threshold.
 *   2. Sets the overall health level to the worst metric level.
 *   3. Counts metrics per level.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HEALTH_MONITOR_H
#define ROOTSTREAM_HEALTH_MONITOR_H

#include "health_metric.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEALTH_MAX_METRICS  32   /**< Maximum registered metrics */

/** Health evaluation summary */
typedef struct {
    hm_level_t overall;   /**< Worst level across all metrics */
    int        n_ok;
    int        n_warn;
    int        n_crit;
    int        n_metrics; /**< Total registered */
} health_summary_t;

/** Opaque health monitor */
typedef struct health_monitor_s health_monitor_t;

/**
 * health_monitor_create — allocate monitor
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
health_monitor_t *health_monitor_create(void);

/**
 * health_monitor_destroy — free monitor
 */
void health_monitor_destroy(health_monitor_t *hm);

/**
 * health_monitor_register — register a metric (copied by name)
 *
 * @param hm    Monitor
 * @param name  Unique metric name
 * @param kind  Metric kind
 * @return      Pointer to the registered metric (owned by monitor),
 *              or NULL if full or duplicate name
 */
health_metric_t *health_monitor_register(health_monitor_t *hm,
                                           const char       *name,
                                           hm_kind_t         kind);

/**
 * health_monitor_get — look up metric by name
 *
 * @param hm    Monitor
 * @param name  Metric name
 * @return      Pointer to metric, or NULL if not found
 */
health_metric_t *health_monitor_get(health_monitor_t *hm, const char *name);

/**
 * health_monitor_evaluate — evaluate all metrics and return summary
 *
 * @param hm   Monitor
 * @param out  Output summary
 * @return     0 on success, -1 on NULL
 */
int health_monitor_evaluate(health_monitor_t *hm, health_summary_t *out);

/**
 * health_monitor_metric_count — number of registered metrics
 */
int health_monitor_metric_count(const health_monitor_t *hm);

/**
 * health_monitor_foreach — iterate all registered metrics
 *
 * @param hm    Monitor
 * @param cb    Callback called once per registered metric
 * @param user  User pointer forwarded to callback
 */
void health_monitor_foreach(health_monitor_t *hm,
                               void (*cb)(const health_metric_t *m, void *user),
                               void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HEALTH_MONITOR_H */
