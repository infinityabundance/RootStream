/*
 * schedule_clock.h — Monotonic and wall-clock helpers for the scheduler
 *
 * Provides thin wrappers around POSIX clock_gettime() so the scheduler
 * and its tests can inject a fake clock for deterministic unit testing.
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_SCHEDULE_CLOCK_H
#define ROOTSTREAM_SCHEDULE_CLOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * schedule_clock_now_us — return current wall-clock time in µs since epoch
 *
 * Uses CLOCK_REALTIME.
 *
 * @return  Microseconds since Unix epoch
 */
uint64_t schedule_clock_now_us(void);

/**
 * schedule_clock_mono_us — return monotonic timestamp in µs
 *
 * Uses CLOCK_MONOTONIC.  Useful for measuring intervals without
 * clock-adjustment hazards.
 *
 * @return  Microseconds since unspecified monotonic epoch
 */
uint64_t schedule_clock_mono_us(void);

/**
 * schedule_clock_sleep_us — sleep for @us microseconds
 *
 * Implemented via nanosleep(); may sleep longer if interrupted.
 *
 * @param us  Microseconds to sleep
 */
void schedule_clock_sleep_us(uint64_t us);

/**
 * schedule_clock_format — format @us (epoch µs) as "YYYY-MM-DD HH:MM:SS"
 *
 * @param us      Microseconds since Unix epoch
 * @param buf     Output buffer
 * @param buf_sz  Must be >= 20 bytes
 * @return        @buf on success, NULL on error
 */
char *schedule_clock_format(uint64_t us, char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SCHEDULE_CLOCK_H */
