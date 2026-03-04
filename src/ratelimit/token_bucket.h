/*
 * token_bucket.h — Token bucket rate limiter
 *
 * A token bucket allows bursting up to @burst_tokens at zero rate cost,
 * then refills at @rate_tokens_per_sec.  Each `consume()` call removes
 * @n tokens; if insufficient tokens are available, the call fails.
 *
 * Time is supplied by the caller (µs epoch) so the bucket is testable
 * without sleeping.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TOKEN_BUCKET_H
#define ROOTSTREAM_TOKEN_BUCKET_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque token bucket */
typedef struct token_bucket_s token_bucket_t;

/**
 * token_bucket_create — allocate a new token bucket
 *
 * @param rate_bps  Refill rate in tokens (bytes) per second
 * @param burst     Maximum bucket capacity (burst allowance in tokens)
 * @param now_us    Initial wall-clock time in µs
 * @return          Non-NULL handle, or NULL on OOM / bad parameters
 */
token_bucket_t *token_bucket_create(double   rate_bps,
                                      double   burst,
                                      uint64_t now_us);

/**
 * token_bucket_destroy — free bucket
 *
 * @param tb  Bucket to destroy
 */
void token_bucket_destroy(token_bucket_t *tb);

/**
 * token_bucket_consume — attempt to consume @n tokens
 *
 * Refills the bucket based on elapsed time since last call, then
 * removes @n tokens if available.
 *
 * @param tb      Bucket
 * @param n       Tokens requested (e.g. packet bytes)
 * @param now_us  Current wall-clock time in µs
 * @return        true if tokens were consumed, false if bucket empty
 */
bool token_bucket_consume(token_bucket_t *tb, double n, uint64_t now_us);

/**
 * token_bucket_available — return current token level
 *
 * Refills based on elapsed time but does NOT remove tokens.
 *
 * @param tb      Bucket
 * @param now_us  Current wall-clock time in µs
 * @return        Available tokens
 */
double token_bucket_available(token_bucket_t *tb, uint64_t now_us);

/**
 * token_bucket_reset — reset bucket to full capacity
 *
 * @param tb      Bucket
 * @param now_us  Current wall-clock time in µs
 */
void token_bucket_reset(token_bucket_t *tb, uint64_t now_us);

/**
 * token_bucket_set_rate — update refill rate (keeps current level)
 *
 * @param tb        Bucket
 * @param rate_bps  New refill rate (tokens/sec)
 * @param now_us    Current time in µs (triggers refill first)
 * @return          0 on success, -1 on bad rate / NULL
 */
int token_bucket_set_rate(token_bucket_t *tb, double rate_bps, uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TOKEN_BUCKET_H */
