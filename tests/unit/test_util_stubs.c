/*
 * test_util_stubs.c - Minimal stubs for unit tests
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

/*
 * Get current time in milliseconds
 */
uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*
 * Get current time in microseconds
 */
uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
