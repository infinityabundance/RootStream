/*
 * plc_history.h — Ring buffer of recent good audio frames for PLC
 *
 * Stores the last PLC_HISTORY_DEPTH received (non-lost) frames so that
 * concealment algorithms can use them to synthesise substitute frames.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PLC_HISTORY_H
#define ROOTSTREAM_PLC_HISTORY_H

#include "plc_frame.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_HISTORY_DEPTH  8   /**< Number of past frames retained */

/** Opaque PLC history context */
typedef struct plc_history_s plc_history_t;

/**
 * plc_history_create — allocate history context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
plc_history_t *plc_history_create(void);

/**
 * plc_history_destroy — free context
 *
 * @param h  Context to destroy
 */
void plc_history_destroy(plc_history_t *h);

/**
 * plc_history_push — record a successfully received frame
 *
 * Older frames beyond PLC_HISTORY_DEPTH are silently dropped.
 *
 * @param h      History
 * @param frame  Frame to store
 * @return       0 on success, -1 on NULL args
 */
int plc_history_push(plc_history_t     *h,
                      const plc_frame_t *frame);

/**
 * plc_history_get_last — retrieve the most recently pushed frame
 *
 * @param h    History
 * @param out  Output frame
 * @return     0 on success, -1 if empty / NULL args
 */
int plc_history_get_last(const plc_history_t *h, plc_frame_t *out);

/**
 * plc_history_get — retrieve a frame by age (0 = newest, 1 = one before, …)
 *
 * @param h    History
 * @param age  Age index (0 to count-1)
 * @param out  Output frame
 * @return     0 on success, -1 if age out of range
 */
int plc_history_get(const plc_history_t *h, int age, plc_frame_t *out);

/**
 * plc_history_count — number of frames stored
 *
 * @param h  History
 * @return   Count (0 to PLC_HISTORY_DEPTH)
 */
int plc_history_count(const plc_history_t *h);

/**
 * plc_history_is_empty — return true if no frames have been pushed
 *
 * @param h  History
 * @return   true if empty
 */
bool plc_history_is_empty(const plc_history_t *h);

/**
 * plc_history_clear — remove all stored frames
 *
 * @param h  History
 */
void plc_history_clear(plc_history_t *h);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLC_HISTORY_H */
