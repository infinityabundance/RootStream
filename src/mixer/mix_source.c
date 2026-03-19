/*
 * mix_source.c — Mixer source implementation
 */

#include "mix_source.h"

#include <string.h>

int mix_source_init(mix_source_t *src, uint32_t id, mix_src_type_t type, float weight,
                    const char *name) {
    if (!src)
        return -1;
    memset(src, 0, sizeof(*src));
    src->id = id;
    src->type = type;
    if (weight < 0.0f)
        weight = 0.0f;
    if (weight > MIX_WEIGHT_MAX)
        weight = MIX_WEIGHT_MAX;
    src->weight = weight;
    if (name)
        strncpy(src->name, name, MIX_SOURCE_NAME_MAX - 1);
    return 0;
}

int mix_source_set_weight(mix_source_t *src, float weight) {
    if (!src)
        return -1;
    if (weight < 0.0f)
        weight = 0.0f;
    if (weight > MIX_WEIGHT_MAX)
        weight = MIX_WEIGHT_MAX;
    src->weight = weight;
    return 0;
}

int mix_source_set_muted(mix_source_t *src, bool muted) {
    if (!src)
        return -1;
    src->muted = muted;
    return 0;
}

const char *mix_src_type_name(mix_src_type_t t) {
    switch (t) {
        case MIX_SRC_CAPTURE:
            return "CAPTURE";
        case MIX_SRC_MICROPHONE:
            return "MICROPHONE";
        case MIX_SRC_LOOPBACK:
            return "LOOPBACK";
        case MIX_SRC_SYNTH:
            return "SYNTH";
        default:
            return "UNKNOWN";
    }
}
