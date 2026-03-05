/*
 * mix_source.h — Audio mixer source registration
 *
 * Represents one contributor to the mix.  Each source has an integer
 * ID, a human-readable name, a linear gain weight in [0.0, 4.0] and a
 * mute flag.  Sources are value-typed; the engine holds an array.
 *
 * Thread-safety: no shared mutable state — thread-safe.
 */

#ifndef ROOTSTREAM_MIX_SOURCE_H
#define ROOTSTREAM_MIX_SOURCE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIX_SOURCE_NAME_MAX  32   /**< Max source name length (incl. NUL) */
#define MIX_WEIGHT_MAX       4.0f /**< Maximum per-source linear gain */

/** Source type tag */
typedef enum {
    MIX_SRC_CAPTURE  = 0,  /**< Desktop / DMA-BUF capture audio */
    MIX_SRC_MICROPHONE = 1,/**< Microphone input */
    MIX_SRC_LOOPBACK = 2,  /**< PulseAudio / PipeWire loopback */
    MIX_SRC_SYNTH    = 3,  /**< Synthetic / test tone */
} mix_src_type_t;

/** Mixer source descriptor */
typedef struct {
    uint32_t      id;                       /**< Unique source ID */
    mix_src_type_t type;
    float         weight;                   /**< Linear gain [0.0, MIX_WEIGHT_MAX] */
    bool          muted;
    char          name[MIX_SOURCE_NAME_MAX];
} mix_source_t;

/**
 * mix_source_init — initialise a source descriptor
 *
 * @param src     Source to initialise
 * @param id      Unique ID
 * @param type    Source type
 * @param weight  Linear gain (clamped to [0.0, MIX_WEIGHT_MAX])
 * @param name    Display name (truncated to MIX_SOURCE_NAME_MAX-1)
 * @return        0 on success, -1 on NULL
 */
int mix_source_init(mix_source_t  *src,
                    uint32_t       id,
                    mix_src_type_t type,
                    float          weight,
                    const char    *name);

/**
 * mix_source_set_weight — update gain, clamp to [0, MIX_WEIGHT_MAX]
 *
 * @param src     Source
 * @param weight  New linear gain
 * @return        0 on success, -1 on NULL
 */
int mix_source_set_weight(mix_source_t *src, float weight);

/**
 * mix_source_set_muted — update mute flag
 *
 * @param src   Source
 * @param muted New mute state
 * @return      0 on success, -1 on NULL
 */
int mix_source_set_muted(mix_source_t *src, bool muted);

/**
 * mix_src_type_name — human-readable type name
 *
 * @param t  Type
 * @return   Static string
 */
const char *mix_src_type_name(mix_src_type_t t);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MIX_SOURCE_H */
