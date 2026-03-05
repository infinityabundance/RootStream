/*
 * schedule_clock.c — Clock helpers implementation
 */

#include "schedule_clock.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

uint64_t schedule_clock_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

uint64_t schedule_clock_mono_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

void schedule_clock_sleep_us(uint64_t us) {
    struct timespec req;
    req.tv_sec  = (time_t)(us / 1000000ULL);
    req.tv_nsec = (long)((us % 1000000ULL) * 1000ULL);
    nanosleep(&req, NULL);
}

char *schedule_clock_format(uint64_t us, char *buf, size_t buf_sz) {
    if (!buf || buf_sz < 20) return NULL;
    time_t sec = (time_t)(us / 1000000ULL);
    struct tm tm_info;
    gmtime_r(&sec, &tm_info);
    strftime(buf, buf_sz, "%Y-%m-%d %H:%M:%S", &tm_info);
    return buf;
}
