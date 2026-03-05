/*
 * output_registry.h — 16-slot output target registry
 *
 * Manages a flat array of output_target_t instances.  Provides
 * add/remove/enable/disable by name, a foreach iterator, and a bulk
 * count of active targets.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_OUTPUT_REGISTRY_H
#define ROOTSTREAM_OUTPUT_REGISTRY_H

#include "output_target.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OUTPUT_MAX_TARGETS  16   /**< Maximum registered targets */

/** Opaque output registry */
typedef struct output_registry_s output_registry_t;

/**
 * output_registry_create — allocate registry
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
output_registry_t *output_registry_create(void);

/**
 * output_registry_destroy — free registry
 */
void output_registry_destroy(output_registry_t *r);

/**
 * output_registry_add — register a target (copied by name+url+proto)
 *
 * @param r         Registry
 * @param name      Friendly name (must be unique)
 * @param url       Endpoint URL
 * @param protocol  Protocol tag
 * @return          Pointer to registered target, or NULL if full/dup
 */
output_target_t *output_registry_add(output_registry_t *r,
                                       const char *name,
                                       const char *url,
                                       const char *protocol);

/**
 * output_registry_remove — unregister target by name
 *
 * @param r     Registry
 * @param name  Target name
 * @return      0 on success, -1 if not found
 */
int output_registry_remove(output_registry_t *r, const char *name);

/**
 * output_registry_get — look up target by name
 *
 * @return  Pointer to target (owned by registry), or NULL
 */
output_target_t *output_registry_get(output_registry_t *r, const char *name);

/**
 * output_registry_set_state — update target state
 *
 * @param r      Registry
 * @param name   Target name
 * @param state  New state
 * @return       0 on success, -1 if not found
 */
int output_registry_set_state(output_registry_t *r,
                                const char *name,
                                ot_state_t state);

/**
 * output_registry_enable  — enable  target by name
 * output_registry_disable — disable target by name (sets OT_DISABLED)
 */
int output_registry_enable(output_registry_t *r, const char *name);
int output_registry_disable(output_registry_t *r, const char *name);

/**
 * output_registry_count — number of registered targets
 */
int output_registry_count(const output_registry_t *r);

/**
 * output_registry_active_count — number of targets in OT_ACTIVE state
 */
int output_registry_active_count(const output_registry_t *r);

/**
 * output_registry_foreach — iterate registered targets
 *
 * @param r     Registry
 * @param cb    Callback
 * @param user  User pointer forwarded to callback
 */
void output_registry_foreach(output_registry_t *r,
                               void (*cb)(output_target_t *t, void *user),
                               void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_OUTPUT_REGISTRY_H */
