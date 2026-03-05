# Code Hygiene Standards — RootStream C/C++ Codebase

> **Phase:** PHASE-86.1
> **Applies to:** All C11 files in `src/`, all C++17 files in
> `clients/kde-plasma-client/src/`, and integration/unit tests.

---

## 1. C11 Module Structure

Every C module consists of exactly four files:

```
src/<module>/
    <module>.h     — public API (included by callers)
    <module>.c     — implementation (includes only its own .h + stdlib)
    <module>_test.c  (optional — unit tests live in tests/unit/)
```

**Rules:**
- Header files must be wrapped in `#ifndef ROOTSTREAM_<MODULE>_H` include
  guards — never use `#pragma once` (not guaranteed by C11).
- All public types end in `_t`.  Example: `fc_engine_t`, not `FCEngine`.
- All public functions are prefixed with the module abbreviation.
  Example: `fc_engine_create()`, `eb_bus_subscribe()`.
- Internal (static) helpers carry no prefix and are not declared in the header.
- All public pointer parameters are checked for NULL at the top of the
  function body.  On NULL: return -1 (for int-returning functions), return
  NULL (for pointer-returning functions), or return early (for void functions).

---

## 2. Naming Conventions

| Construct | Convention | Example |
|-----------|------------|---------|
| Public type | `snake_case_t` | `fc_params_t` |
| Public function | `module_noun_verb()` | `sr_router_add_route()` |
| Public constant / macro | `MODULE_UPPER_CASE` | `EB_MAX_SUBSCRIBERS` |
| Private (static) function | `snake_case()` | `find_free_slot()` |
| Struct field | `snake_case` | `window_bytes` |
| Boolean field | `in_use`, `is_active` | `g->in_use` |
| Opaque struct | `struct <module>_s` → `typedef … <module>_t` | `struct fc_engine_s` |

---

## 3. Memory Management

- Every `*_create()` function must be paired with a `*_destroy()`.
- `*_destroy()` accepts NULL (no-op) — never crashes on double-free path.
- `*_destroy()` does NOT free caller-owned payload pointers (e.g., `data`
  fields in queue entries).  Ownership is documented in the header.
- Use `calloc(1, sizeof(*obj))` for allocation — zero-initialises, avoiding
  "forgot to memset" bugs.
- Prefer `free()` over `memset + free` — the memory will be reclaimed by the
  OS; scrubbing is only required for security-sensitive buffers.

---

## 4. Error Handling

- Functions that can fail return `int`: `0` = success, `-1` = failure.
- Never return negative values other than `-1` (use `errno` or an out
  parameter for error codes when finer granularity is needed).
- Allocation failures (`malloc` returns NULL) must be checked and handled.
  Unchecked allocations are rejected in code review.
- Functions that produce a value and can fail use output parameters:
  ```c
  int fc_stats_snapshot(const fc_stats_t *st, fc_stats_snapshot_t *out);
  ```
  Not: `fc_stats_snapshot_t fc_stats_snapshot(...)` (return by value forces
  the caller to distinguish "valid snapshot with zeros" from "error").

---

## 5. Thread Safety Documentation

Every public header must state its thread-safety contract at the top of
the file.  Use one of:
- `Thread-safety: NOT thread-safe.` — caller must serialise all calls.
- `Thread-safety: individual operations are atomic.` — each call is
  safe in isolation but compound operations are not.
- `Thread-safety: <function> is safe to call from any thread.` — specific
  call-out for functions that are internally locked.

---

## 6. Commenting Standards

Every public function must have a Doxygen-style block comment:
```c
/**
 * function_name — one-line summary
 *
 * Longer description if needed.
 *
 * @param p  Description of parameter
 * @return   0 on success, -1 on failure (specific condition)
 */
```

Implementation files (`.c`) must explain **why**, not just **what**:
```c
/* Use send_budget (not window_bytes) for initial credit.
 * window_bytes is the MAXIMUM in-flight limit; send_budget is the
 * per-epoch allocation.  Starting at window_bytes would allow a burst
 * on the first epoch that starves other flows. */
e->credit = p->send_budget;
```

Avoid:
```c
/* Set credit */          // ← useless — the code already says this
e->credit = p->send_budget;
```

---

## 7. Test Requirements

Every new module must ship with:
1. A unit test in `tests/unit/test_<module>.c` covering:
   - Normal operation (happy path).
   - NULL pointer guards (all public functions called with NULL).
   - Boundary conditions (capacity limits, zero-length, max values).
2. An integration test in `tests/integration/` if the module is designed
   to work with another module (see `tests/integration/integration_harness.h`).

Test functions return `int` (0 = pass, 1 = fail).  `main()` sums results.
No external test framework dependencies.

---

## 8. C++ (Qt6 Client) Hygiene

- Use `nullptr` not `NULL` in C++ code.
- Q_PROPERTY declarations must always have a NOTIFY signal.
- Use new-style `connect()` syntax (type-safe):
  ```cpp
  connect(&mgr, &SettingsManager::codecChanged, this, &MyClass::onCodecChanged);
  ```
  Not: `connect(&mgr, SIGNAL(codecChanged()), this, SLOT(onCodecChanged()));`
- Use `QSignalSpy` in tests to verify signal emission count and arguments.
- Every `QObject` subclass must declare `Q_OBJECT` — omitting it breaks
  signals, slots, and `qobject_cast`.
- Ownership: parent/child QObject trees are preferred over manual `delete`.
  Use `new SomeWidget(this)` rather than `new SomeWidget(nullptr)` + manual
  `delete`.

---

## 9. Build Cleanliness

All C modules must compile clean with:
```
-std=c11 -Wall -Wextra -Wno-unused-parameter -D_POSIX_C_SOURCE=200809L
```

All C++ modules must compile clean with:
```
-std=c++17 -Wall -Wextra -Wno-unused-parameter
```

Warnings are treated as errors in CI.  If a warning cannot be suppressed
cleanly, add a `// NOLINTNEXTLINE(...)` comment with a justification.

---

## 10. File Checklist (before merging a new module)

- [ ] Header has include guard `#ifndef ROOTSTREAM_<MODULE>_H`
- [ ] Header documents thread-safety contract
- [ ] All public functions have Doxygen `/**` comment blocks
- [ ] All public pointer params have NULL checks
- [ ] `*_create()` / `*_destroy()` pair exists
- [ ] Unit test in `tests/unit/test_<module>.c`
- [ ] Integration test in `tests/integration/` (if cross-module)
- [ ] `docs/microtasks.md` updated with new phase rows
- [ ] `scripts/validate_traceability.sh` range extended
- [ ] Compiles without warnings under the flags above
