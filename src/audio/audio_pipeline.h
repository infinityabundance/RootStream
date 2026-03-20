/*
 * audio_pipeline.h — Audio DSP chain framework
 *
 * Provides a composable linear DSP pipeline where each stage is a
 * stateless or stateful audio_filter_node_t.  Nodes are chained by the
 * pipeline and called in insertion order on each PCM buffer.
 *
 * All processing is interleaved 32-bit float PCM at the sample rate and
 * channel count negotiated at pipeline creation time.
 *
 * Thread-safety: audio_pipeline_process() is not thread-safe; call it
 * from a single audio thread.  audio_pipeline_add_node() / _remove_node()
 * must not be called concurrently with _process().
 */

#ifndef ROOTSTREAM_AUDIO_PIPELINE_H
#define ROOTSTREAM_AUDIO_PIPELINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of filter nodes in a single pipeline */
#define AUDIO_PIPELINE_MAX_NODES 16

/** DSP node function type — modifies @samples in-place */
typedef void (*audio_filter_fn_t)(float *samples, size_t frame_count, int channels,
                                  void *user_data);

/**
 * audio_filter_node_t — one DSP stage in the chain
 *
 * All fields are set at node-creation time and must not change while the
 * node is in a pipeline.
 */
typedef struct {
    const char *name;          /**< Human-readable name for diagnostics */
    audio_filter_fn_t process; /**< Processing callback (non-NULL) */
    void *user_data;           /**< Opaque state passed to @process */
    bool enabled;              /**< When false the node is bypassed */
} audio_filter_node_t;

/** Opaque pipeline handle */
typedef struct audio_pipeline_s audio_pipeline_t;

/**
 * audio_pipeline_create — allocate an empty pipeline
 *
 * @param sample_rate  PCM sample rate in Hz (e.g. 48000)
 * @param channels     Number of interleaved channels (1 or 2)
 * @return             Non-NULL pipeline, or NULL on OOM
 */
audio_pipeline_t *audio_pipeline_create(int sample_rate, int channels);

/**
 * audio_pipeline_destroy — free all resources
 *
 * Does not call any cleanup on the nodes' user_data; callers own that.
 *
 * @param pipeline  Pipeline to destroy
 */
void audio_pipeline_destroy(audio_pipeline_t *pipeline);

/**
 * audio_pipeline_add_node — append a filter node at the tail
 *
 * @param pipeline  Target pipeline
 * @param node      Fully initialised node (shallow copy stored)
 * @return          0 on success, -1 if pipeline is full
 */
int audio_pipeline_add_node(audio_pipeline_t *pipeline, const audio_filter_node_t *node);

/**
 * audio_pipeline_remove_node — remove a node by name
 *
 * @param pipeline  Target pipeline
 * @param name      Exact match of audio_filter_node_t::name
 * @return          0 on success, -1 if not found
 */
int audio_pipeline_remove_node(audio_pipeline_t *pipeline, const char *name);

/**
 * audio_pipeline_process — run @samples through all enabled nodes
 *
 * @param pipeline     Pipeline to run
 * @param samples      Interleaved float PCM buffer (modified in-place)
 * @param frame_count  Number of audio frames (samples / channels)
 */
void audio_pipeline_process(audio_pipeline_t *pipeline, float *samples, size_t frame_count);

/**
 * audio_pipeline_node_count — return number of nodes in the pipeline
 *
 * @param pipeline  Pipeline
 * @return          Node count
 */
size_t audio_pipeline_node_count(const audio_pipeline_t *pipeline);

/**
 * audio_pipeline_get_sample_rate — return the configured sample rate
 *
 * @param pipeline  Pipeline
 * @return          Sample rate in Hz
 */
int audio_pipeline_get_sample_rate(const audio_pipeline_t *pipeline);

/**
 * audio_pipeline_get_channels — return the configured channel count
 *
 * @param pipeline  Pipeline
 * @return          Channel count
 */
int audio_pipeline_get_channels(const audio_pipeline_t *pipeline);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_AUDIO_PIPELINE_H */
