/*
 * mix_engine.h — Weighted PCM audio blending engine
 *
 * Accepts signed 16-bit mono PCM samples from up to MIX_MAX_SOURCES
 * sources, blends them with per-source linear weights, and writes the
 * result into a caller-supplied output buffer.
 *
 * Overflow is handled by symmetric hard-clipping to [-32768, 32767].
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_MIX_ENGINE_H
#define ROOTSTREAM_MIX_ENGINE_H

#include "mix_source.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIX_MAX_SOURCES   16   /**< Maximum simultaneously registered sources */
#define MIX_MAX_FRAMES    4096 /**< Maximum frame count per mix call */

/** Opaque mixer engine */
typedef struct mix_engine_s mix_engine_t;

/**
 * mix_engine_create — allocate engine
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
mix_engine_t *mix_engine_create(void);

/**
 * mix_engine_destroy — free engine
 *
 * @param e  Engine to destroy
 */
void mix_engine_destroy(mix_engine_t *e);

/**
 * mix_engine_add_source — register a source
 *
 * @param e    Engine
 * @param src  Source descriptor (copied)
 * @return     0 on success, -1 if engine full or duplicate ID
 */
int mix_engine_add_source(mix_engine_t *e, const mix_source_t *src);

/**
 * mix_engine_remove_source — unregister source by ID
 *
 * @param e   Engine
 * @param id  Source ID
 * @return    0 on success, -1 if not found
 */
int mix_engine_remove_source(mix_engine_t *e, uint32_t id);

/**
 * mix_engine_update_source — replace source descriptor (must match ID)
 *
 * @param e    Engine
 * @param src  New descriptor
 * @return     0 on success, -1 if not found
 */
int mix_engine_update_source(mix_engine_t *e, const mix_source_t *src);

/**
 * mix_engine_source_count — number of registered sources
 *
 * @param e  Engine
 * @return   Count
 */
int mix_engine_source_count(const mix_engine_t *e);

/**
 * mix_engine_mix — blend PCM data from all non-muted sources
 *
 * Each source contributes its @frames signed-16 samples (one channel),
 * scaled by its weight.  Mixed samples are hard-clipped to [-32768, 32767].
 *
 * @param e          Engine
 * @param inputs     Array of @source_count input buffers (each @frames samples)
 * @param src_ids    Source IDs corresponding to each input buffer
 * @param src_count  Number of input buffers
 * @param out        Output buffer (@frames samples)
 * @param frames     Number of samples per buffer
 * @return           0 on success, -1 on error
 */
int mix_engine_mix(mix_engine_t        *e,
                   const int16_t *const *inputs,
                   const uint32_t       *src_ids,
                   int                   src_count,
                   int16_t              *out,
                   int                   frames);

/**
 * mix_engine_silence — fill output buffer with zeros
 *
 * @param out     Buffer to zero
 * @param frames  Sample count
 */
void mix_engine_silence(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MIX_ENGINE_H */
