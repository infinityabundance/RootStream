/*
 * cs_sample.c — NTP-style clock sync sample implementation
 */

#include "cs_sample.h"

int cs_sample_init(cs_sample_t *s, uint64_t t0, uint64_t t1, uint64_t t2, uint64_t t3) {
    if (!s)
        return -1;
    s->t0 = t0;
    s->t1 = t1;
    s->t2 = t2;
    s->t3 = t3;
    return 0;
}

int64_t cs_sample_rtt_us(const cs_sample_t *s) {
    if (!s)
        return 0;
    /* d = (t3 - t0) - (t2 - t1) */
    return (int64_t)(s->t3 - s->t0) - (int64_t)(s->t2 - s->t1);
}

int64_t cs_sample_offset_us(const cs_sample_t *s) {
    if (!s)
        return 0;
    /* θ = ((t1 - t0) + (t2 - t3)) / 2 */
    int64_t a = (int64_t)s->t1 - (int64_t)s->t0;
    int64_t b = (int64_t)s->t2 - (int64_t)s->t3;
    return (a + b) / 2;
}
