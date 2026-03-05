/*
 * integration_harness.h — Shared test harness for RootStream integration tests
 *
 * PURPOSE
 * -------
 * Integration tests verify that two or more subsystems work correctly
 * together in a real call sequence — not merely that each subsystem
 * compiles or that its unit tests pass in isolation.  This header
 * provides lightweight macros so every integration test reads the
 * same way, without pulling in an external test framework.
 *
 * USAGE
 * -----
 * #include "integration_harness.h"
 * INTEG_ASSERT(expr, "human message");   // abort test on failure
 * INTEG_PASS(suite, test_name);          // print green line
 * INTEG_FAIL(suite, test_name, msg);     // print red line + return 1
 *
 * Each test function returns int (0 = pass, 1 = fail).
 * main() sums return values; exits 0 only if sum == 0.
 *
 * THREAD-SAFETY
 * -------------
 * Macros are not thread-safe.  Run integration tests single-threaded.
 */

#ifndef ROOTSTREAM_INTEGRATION_HARNESS_H
#define ROOTSTREAM_INTEGRATION_HARNESS_H

#include <stdio.h>

/* ── assertion ─────────────────────────────────────────────────────── */

/**
 * INTEG_ASSERT — abort current test function if condition is false.
 *
 * On failure prints file:line and the human-readable @msg, then
 * returns 1 from the enclosing function (which must return int).
 */
#define INTEG_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  INTEG FAIL  [%s:%d]  %s\n", \
                    __FILE__, __LINE__, (msg)); \
            return 1; \
        } \
    } while (0)

/* ── pass / fail reporters ─────────────────────────────────────────── */

/**
 * INTEG_PASS — print a passing test line and continue.
 * Use at the end of each test function before `return 0;`.
 */
#define INTEG_PASS(suite, name) \
    printf("  ✅  [%s] %s\n", (suite), (name))

/**
 * INTEG_FAIL — print a failing test line and return 1.
 * Rarely needed directly — INTEG_ASSERT is preferred.
 */
#define INTEG_FAIL(suite, name, msg) \
    do { \
        fprintf(stderr, "  ❌  [%s] %s — %s\n", (suite), (name), (msg)); \
        return 1; \
    } while (0)

/* ── suite banner ──────────────────────────────────────────────────── */

/**
 * INTEG_SUITE — print a section header for a group of related tests.
 */
#define INTEG_SUITE(name) \
    printf("\n── Integration Suite: %s ──\n", (name))

#endif /* ROOTSTREAM_INTEGRATION_HARNESS_H */
