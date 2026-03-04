/*
 * frc_clock.h — Monotonic clock abstraction (nanosecond precision)
 *
 * Provides a thin wrapper around CLOCK_MONOTONIC so that upper-layer
 * code can be tested with an injected time source.
 *
 * Two modes:
 *  - Real mode  : `frc_clock_now_ns()` reads POSIX CLOCK_MONOTONIC
 *  - Stub mode  : `frc_clock_set_stub_ns()` injects a fixed value for tests
 *
 * Thread-safety: frc_clock_now_ns() is thread-safe (wraps a syscall).
 *                The stub is NOT thread-safe and is for testing only.
 */

#ifndef ROOTSTREAM_FRC_CLOCK_H
#define ROOTSTREAM_FRC_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * frc_clock_now_ns — return current monotonic time in nanoseconds
 *
 * Falls back to stub value when stub mode is active.
 *
 * @return  Monotonic nanoseconds
 */
uint64_t frc_clock_now_ns(void);

/**
 * frc_clock_set_stub_ns — enable stub mode with a fixed time value
 *
 * @param ns  Time value to return from frc_clock_now_ns()
 */
void frc_clock_set_stub_ns(uint64_t ns);

/**
 * frc_clock_clear_stub — disable stub mode (restore real clock)
 */
void frc_clock_clear_stub(void);

/**
 * frc_clock_is_stub — return true if stub mode is active
 *
 * @return  true if using injected time
 */
bool frc_clock_is_stub(void);

/**
 * frc_clock_ns_to_us — convert nanoseconds to microseconds
 *
 * @param ns  Nanoseconds
 * @return    Microseconds (truncated)
 */
static inline uint64_t frc_clock_ns_to_us(uint64_t ns) { return ns / 1000ULL; }

/**
 * frc_clock_ns_to_ms — convert nanoseconds to milliseconds
 *
 * @param ns  Nanoseconds
 * @return    Milliseconds (truncated)
 */
static inline uint64_t frc_clock_ns_to_ms(uint64_t ns) { return ns / 1000000ULL; }

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FRC_CLOCK_H */
