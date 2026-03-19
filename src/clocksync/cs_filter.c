/*
 * cs_filter.c — 8-sample median filter for clock sync
 */

#include "cs_filter.h"

#include <stdlib.h>
#include <string.h>

struct cs_filter_s {
    int64_t offsets[CS_FILTER_SIZE]; /* offset samples (sliding ring) */
    int64_t rtts[CS_FILTER_SIZE];    /* RTT samples (sliding ring) */
    int head;                        /* next write index */
    int count;                       /* samples held (≤ CS_FILTER_SIZE) */
};

cs_filter_t *cs_filter_create(void) {
    return calloc(1, sizeof(cs_filter_t));
}

void cs_filter_destroy(cs_filter_t *f) {
    free(f);
}

void cs_filter_reset(cs_filter_t *f) {
    if (f)
        memset(f, 0, sizeof(*f));
}

/* Simple insertion sort of n int64_t values into tmp */
static void sort64(const int64_t *src, int64_t *tmp, int n) {
    for (int i = 0; i < n; i++) tmp[i] = src[i];
    for (int i = 1; i < n; i++) {
        int64_t key = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > key) {
            tmp[j + 1] = tmp[j];
            j--;
        }
        tmp[j + 1] = key;
    }
}

static int64_t median64(const int64_t *arr, int n) {
    int64_t tmp[CS_FILTER_SIZE];
    sort64(arr, tmp, n);
    if (n % 2 == 0)
        return (tmp[n / 2 - 1] + tmp[n / 2]) / 2;
    return tmp[n / 2];
}

int cs_filter_push(cs_filter_t *f, const cs_sample_t *s, cs_filter_out_t *out) {
    if (!f || !s || !out)
        return -1;

    f->offsets[f->head] = cs_sample_offset_us(s);
    f->rtts[f->head] = cs_sample_rtt_us(s);
    f->head = (f->head + 1) % CS_FILTER_SIZE;
    if (f->count < CS_FILTER_SIZE)
        f->count++;

    out->count = f->count;
    out->converged = (f->count >= CS_FILTER_SIZE);
    out->offset_us = median64(f->offsets, f->count);
    out->rtt_us = median64(f->rtts, f->count);
    return 0;
}
