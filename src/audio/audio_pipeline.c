/*
 * audio_pipeline.c — Audio DSP chain framework implementation
 */

#include "audio_pipeline.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct audio_pipeline_s {
    audio_filter_node_t nodes[AUDIO_PIPELINE_MAX_NODES];
    size_t              node_count;
    int                 sample_rate;
    int                 channels;
};

audio_pipeline_t *audio_pipeline_create(int sample_rate, int channels) {
    if (sample_rate <= 0 || channels <= 0 || channels > 8) {
        return NULL;
    }

    audio_pipeline_t *p = calloc(1, sizeof(*p));
    if (!p) return NULL;

    p->sample_rate = sample_rate;
    p->channels    = channels;
    return p;
}

void audio_pipeline_destroy(audio_pipeline_t *pipeline) {
    free(pipeline);
}

int audio_pipeline_add_node(audio_pipeline_t *pipeline,
                             const audio_filter_node_t *node) {
    if (!pipeline || !node || !node->process) return -1;

    if (pipeline->node_count >= AUDIO_PIPELINE_MAX_NODES) {
        fprintf(stderr, "[audio_pipeline] pipeline full (max %d nodes)\n",
                AUDIO_PIPELINE_MAX_NODES);
        return -1;
    }

    pipeline->nodes[pipeline->node_count++] = *node;
    return 0;
}

int audio_pipeline_remove_node(audio_pipeline_t *pipeline,
                                const char *name) {
    if (!pipeline || !name) return -1;

    for (size_t i = 0; i < pipeline->node_count; i++) {
        if (pipeline->nodes[i].name &&
            strcmp(pipeline->nodes[i].name, name) == 0) {
            memmove(&pipeline->nodes[i],
                    &pipeline->nodes[i + 1],
                    (pipeline->node_count - i - 1) *
                    sizeof(audio_filter_node_t));
            pipeline->node_count--;
            return 0;
        }
    }
    return -1;
}

void audio_pipeline_process(audio_pipeline_t *pipeline,
                             float            *samples,
                             size_t            frame_count) {
    if (!pipeline || !samples || frame_count == 0) return;

    for (size_t i = 0; i < pipeline->node_count; i++) {
        audio_filter_node_t *n = &pipeline->nodes[i];
        if (n->enabled && n->process) {
            n->process(samples, frame_count, pipeline->channels,
                       n->user_data);
        }
    }
}

size_t audio_pipeline_node_count(const audio_pipeline_t *pipeline) {
    return pipeline ? pipeline->node_count : 0;
}

int audio_pipeline_get_sample_rate(const audio_pipeline_t *pipeline) {
    return pipeline ? pipeline->sample_rate : 0;
}

int audio_pipeline_get_channels(const audio_pipeline_t *pipeline) {
    return pipeline ? pipeline->channels : 0;
}
