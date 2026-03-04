/*
 * frc_clock.c — Monotonic clock implementation
 */

#include "frc_clock.h"

#include <time.h>

static int      g_stub_active = 0;
static uint64_t g_stub_ns     = 0;

uint64_t frc_clock_now_ns(void) {
    if (g_stub_active) return g_stub_ns;
#ifdef _POSIX_MONOTONIC_CLOCK
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#else
    /* Fallback: wall clock via time() — lower resolution */
    return (uint64_t)time(NULL) * 1000000000ULL;
#endif
}

void frc_clock_set_stub_ns(uint64_t ns) {
    g_stub_active = 1;
    g_stub_ns     = ns;
}

void frc_clock_clear_stub(void) {
    g_stub_active = 0;
    g_stub_ns     = 0;
}

bool frc_clock_is_stub(void) {
    return g_stub_active != 0;
}
