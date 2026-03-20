/*
 * fc_engine.h — Flow Controller: token-bucket credit engine
 *
 * A single-channel token-bucket flow controller.  The caller creates
 * an engine from an fc_params_t, then calls:
 *   fc_engine_can_send(e, bytes)  — returns true if credit allows
 *   fc_engine_consume(e, bytes)   — deducts bytes from available credit
 *   fc_engine_replenish(e, bytes) — adds bytes up to window_bytes cap
 *
 * The engine does not own timers — the caller drives replenishment on
 * each scheduler tick or ACK receipt.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FC_ENGINE_H
#define ROOTSTREAM_FC_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "fc_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque flow control engine */
typedef struct fc_engine_s fc_engine_t;

/**
 * fc_engine_create — allocate engine with given parameters
 *
 * @return Non-NULL, or NULL on OOM / invalid params
 */
fc_engine_t *fc_engine_create(const fc_params_t *p);

/**
 * fc_engine_destroy — free engine
 */
void fc_engine_destroy(fc_engine_t *e);

/**
 * fc_engine_can_send — check if @bytes of credit is available
 *
 * Does NOT consume credit.
 *
 * @return true if credit_available >= bytes
 */
bool fc_engine_can_send(const fc_engine_t *e, uint32_t bytes);

/**
 * fc_engine_consume — deduct @bytes from available credit
 *
 * Should only be called after fc_engine_can_send() returns true.
 *
 * @return 0 on success, -1 if insufficient credit or NULL
 */
int fc_engine_consume(fc_engine_t *e, uint32_t bytes);

/**
 * fc_engine_replenish — add @bytes of credit (capped at window_bytes)
 *
 * @return new credit_available value
 */
uint32_t fc_engine_replenish(fc_engine_t *e, uint32_t bytes);

/**
 * fc_engine_credit — return current available credit
 */
uint32_t fc_engine_credit(const fc_engine_t *e);

/**
 * fc_engine_reset — restore credit to send_budget
 */
void fc_engine_reset(fc_engine_t *e);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FC_ENGINE_H */
