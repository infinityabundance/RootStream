/*
 * token_bucket.c — Token bucket implementation
 */

#include "token_bucket.h"

#include <stdlib.h>

struct token_bucket_s {
    double rate_per_us; /* tokens per µs (= rate_bps / 1e6) */
    double burst;       /* max tokens */
    double tokens;      /* current token count */
    uint64_t last_us;   /* last refill timestamp */
};

static void refill(token_bucket_t *tb, uint64_t now_us) {
    if (now_us > tb->last_us) {
        double elapsed = (double)(now_us - tb->last_us);
        tb->tokens += elapsed * tb->rate_per_us;
        if (tb->tokens > tb->burst)
            tb->tokens = tb->burst;
        tb->last_us = now_us;
    }
}

token_bucket_t *token_bucket_create(double rate_bps, double burst, uint64_t now_us) {
    if (rate_bps <= 0.0 || burst <= 0.0)
        return NULL;
    token_bucket_t *tb = malloc(sizeof(*tb));
    if (!tb)
        return NULL;
    tb->rate_per_us = rate_bps / 1e6;
    tb->burst = burst;
    tb->tokens = burst; /* start full */
    tb->last_us = now_us;
    return tb;
}

void token_bucket_destroy(token_bucket_t *tb) {
    free(tb);
}

bool token_bucket_consume(token_bucket_t *tb, double n, uint64_t now_us) {
    if (!tb || n <= 0.0)
        return false;
    refill(tb, now_us);
    if (tb->tokens < n)
        return false;
    tb->tokens -= n;
    return true;
}

double token_bucket_available(token_bucket_t *tb, uint64_t now_us) {
    if (!tb)
        return 0.0;
    refill(tb, now_us);
    return tb->tokens;
}

void token_bucket_reset(token_bucket_t *tb, uint64_t now_us) {
    if (!tb)
        return;
    tb->tokens = tb->burst;
    tb->last_us = now_us;
}

int token_bucket_set_rate(token_bucket_t *tb, double rate_bps, uint64_t now_us) {
    if (!tb || rate_bps <= 0.0)
        return -1;
    refill(tb, now_us);
    tb->rate_per_us = rate_bps / 1e6;
    return 0;
}
