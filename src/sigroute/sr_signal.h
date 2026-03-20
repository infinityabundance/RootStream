/*
 * sr_signal.h — Signal Router: signal descriptor
 *
 * A signal carries a numeric identifier, a level (0–255), the
 * wall-clock timestamp (µs) at which it was created, and a source
 * identifier (which module or component emitted the signal).
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_SR_SIGNAL_H
#define ROOTSTREAM_SR_SIGNAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t sr_signal_id_t;
typedef uint32_t sr_source_id_t;

/** Signal descriptor */
typedef struct {
    sr_signal_id_t signal_id; /**< Numeric signal type */
    uint8_t level;            /**< Signal level / severity (0–255) */
    sr_source_id_t source_id; /**< Originating component ID */
    uint64_t timestamp_us;    /**< Wall-clock creation time (µs) */
} sr_signal_t;

/**
 * sr_signal_init — initialise a signal descriptor
 *
 * @return 0 on success, -1 on NULL
 */
int sr_signal_init(sr_signal_t *s, sr_signal_id_t signal_id, uint8_t level,
                   sr_source_id_t source_id, uint64_t timestamp_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SR_SIGNAL_H */
