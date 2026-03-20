/*
 * mix_engine.c — Weighted PCM blending engine
 */

#include "mix_engine.h"

#include <stdlib.h>
#include <string.h>

struct mix_engine_s {
    mix_source_t sources[MIX_MAX_SOURCES];
    bool used[MIX_MAX_SOURCES];
    int count;
};

mix_engine_t *mix_engine_create(void) {
    return calloc(1, sizeof(mix_engine_t));
}

void mix_engine_destroy(mix_engine_t *e) {
    free(e);
}

int mix_engine_source_count(const mix_engine_t *e) {
    return e ? e->count : 0;
}

static int find_slot(const mix_engine_t *e, uint32_t id) {
    for (int i = 0; i < MIX_MAX_SOURCES; i++)
        if (e->used[i] && e->sources[i].id == id)
            return i;
    return -1;
}

int mix_engine_add_source(mix_engine_t *e, const mix_source_t *src) {
    if (!e || !src)
        return -1;
    if (e->count >= MIX_MAX_SOURCES)
        return -1;
    if (find_slot(e, src->id) >= 0)
        return -1; /* duplicate */

    for (int i = 0; i < MIX_MAX_SOURCES; i++) {
        if (!e->used[i]) {
            e->sources[i] = *src;
            e->used[i] = true;
            e->count++;
            return 0;
        }
    }
    return -1;
}

int mix_engine_remove_source(mix_engine_t *e, uint32_t id) {
    if (!e)
        return -1;
    int slot = find_slot(e, id);
    if (slot < 0)
        return -1;
    e->used[slot] = false;
    e->count--;
    return 0;
}

int mix_engine_update_source(mix_engine_t *e, const mix_source_t *src) {
    if (!e || !src)
        return -1;
    int slot = find_slot(e, src->id);
    if (slot < 0)
        return -1;
    e->sources[slot] = *src;
    return 0;
}

void mix_engine_silence(int16_t *out, int frames) {
    if (out && frames > 0)
        memset(out, 0, (size_t)frames * sizeof(int16_t));
}

int mix_engine_mix(mix_engine_t *e, const int16_t *const *inputs, const uint32_t *src_ids,
                   int src_count, int16_t *out, int frames) {
    if (!e || !inputs || !src_ids || !out || frames <= 0)
        return -1;
    if (frames > MIX_MAX_FRAMES)
        return -1;

    mix_engine_silence(out, frames);

    for (int s = 0; s < src_count; s++) {
        if (!inputs[s])
            continue;
        int slot = find_slot(e, src_ids[s]);
        if (slot < 0)
            continue;
        const mix_source_t *src = &e->sources[slot];
        if (src->muted || src->weight == 0.0f)
            continue;

        for (int f = 0; f < frames; f++) {
            float sample = (float)inputs[s][f] * src->weight;
            float mixed = (float)out[f] + sample;
            /* Hard-clip */
            if (mixed > 32767.0f)
                mixed = 32767.0f;
            else if (mixed < -32768.0f)
                mixed = -32768.0f;
            out[f] = (int16_t)mixed;
        }
    }
    return 0;
}
