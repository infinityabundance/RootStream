/*
 * fc_engine.c — Token-bucket flow control engine
 */

#include "fc_engine.h"
#include <stdlib.h>
#include <string.h>

struct fc_engine_s {
    fc_params_t  params;
    uint32_t     credit;   /**< Available send credit (bytes) */
};

fc_engine_t *fc_engine_create(const fc_params_t *p) {
    if (!p || p->window_bytes == 0 || p->send_budget == 0) return NULL;
    fc_engine_t *e = malloc(sizeof(*e));
    if (!e) return NULL;
    e->params = *p;
    e->credit = p->send_budget;
    return e;
}

void fc_engine_destroy(fc_engine_t *e) { free(e); }

bool fc_engine_can_send(const fc_engine_t *e, uint32_t bytes) {
    if (!e) return false;
    return e->credit >= bytes;
}

int fc_engine_consume(fc_engine_t *e, uint32_t bytes) {
    if (!e || e->credit < bytes) return -1;
    e->credit -= bytes;
    return 0;
}

uint32_t fc_engine_replenish(fc_engine_t *e, uint32_t bytes) {
    if (!e) return 0;
    uint32_t cap   = e->params.window_bytes;
    uint32_t added = (bytes < e->params.credit_step)
                     ? e->params.credit_step : bytes;
    e->credit = (e->credit + added > cap) ? cap : e->credit + added;
    return e->credit;
}

uint32_t fc_engine_credit(const fc_engine_t *e) { return e ? e->credit : 0; }

void fc_engine_reset(fc_engine_t *e) {
    if (e) e->credit = e->params.send_budget;
}
