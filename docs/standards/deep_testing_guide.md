# Deep Integration Testing Guide — RootStream

> **Phase:** PHASE-86.4
> **Applies to:** All modules in `src/`, `tests/unit/`, `tests/integration/`,
>   and `clients/kde-plasma-client/tests/`

---

## 1. What "Non-Ceremonial" Testing Means

A test is *ceremonial* when it:
- Calls a function and checks that it does not crash, but never verifies the
  output reaches its intended destination.
- Tests the module in isolation without confirming that the wiring between
  modules actually exists.
- Verifies return values (0 = success) but not observable side-effects.

A test is *substantive* when it:
- Drives two or more modules together and verifies that a value produced by
  module A is observable in module B's output.
- Exercises the path from input to output through every wired component.
- Fails if any component in the chain is removed or disconnected.

**Example (ceremonial):**
```c
fc_engine_t *e = fc_engine_create(&p);
assert(e != NULL);  // proves allocation works, nothing more
```

**Example (substantive):**
```c
fc_engine_consume(e, 200);
fc_stats_record_send(stats, 200);
mx_gauge_add(g_bytes_sent, 200);
// ← Now verify the gauge reflects the consumption:
assert(mx_gauge_get(g_bytes_sent) == 200);
// ← AND verify fc_stats matches the gauge:
fc_stats_snapshot(stats, &snap);
assert(snap.bytes_sent == mx_gauge_get(g_bytes_sent));
```
The second example fails if any one of consume/stats/gauge is disconnected.

---

## 2. Integration Test Structure

Integration tests live in `tests/integration/` and use the shared harness:

```c
#include "integration_harness.h"

static int test_my_pipeline(void)
{
    INTEG_SUITE("module_a↔module_b: description");

    /* 1. Setup: create all participating modules */
    module_a_t *a = module_a_create(...);
    module_b_t *b = module_b_create(...);
    INTEG_ASSERT(a != NULL, "module_a created");
    INTEG_ASSERT(b != NULL, "module_b created");

    /* 2. Exercise: drive the pipeline */
    module_a_do_thing(a, input);
    bridge_a_to_b(a, b);         /* ← this is what we're testing */

    /* 3. Verify: check observable output on module_b */
    INTEG_ASSERT(module_b_output(b) == expected_value,
                 "module_b received expected value from module_a");

    /* 4. Teardown */
    module_b_destroy(b);
    module_a_destroy(a);

    INTEG_PASS("module_a↔module_b", "description");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_my_pipeline();
    return failures ? 1 : 0;
}
```

---

## 3. Five-Pass Test Review Protocol

For each feature, perform five passes before considering it tested:

| Pass | Question |
|------|----------|
| **1. Existence** | Does the feature compile and link without errors? |
| **2. Unit** | Does the feature's unit test pass in isolation? |
| **3. Wiring** | Does an integration test confirm the output of this module reaches its consumer? |
| **4. Boundary** | Are NULL, empty, full, zero, and max-value cases handled and tested? |
| **5. Observability** | Is every recordable metric actually recorded and reachable from a snapshot or log? |

A feature fails the review if **any** of the five passes is missing.

---

## 4. What to Test in Each Layer

### C Subsystem (`src/`)

- ✅ `*_create()` returns non-NULL with valid params
- ✅ `*_create()` returns NULL on invalid/zero params
- ✅ All public functions return -1/false/NULL on NULL input
- ✅ State changes after operations are reflected in getter output
- ✅ Capacity limits enforced (e.g., DQ_MAX_ENTRIES)
- ✅ `*_snapshot()` values match accumulated state

### KDE Plasma Client (`clients/kde-plasma-client/`)

- ✅ Every Q_PROPERTY has a NOTIFY signal (checked via QMetaObject)
- ✅ Every Q_INVOKABLE is reachable via `indexOfMethod()`
- ✅ `setXxx()` emits the corresponding NOTIFY signal (QSignalSpy)
- ✅ Values set via `setXxx()` are returned unchanged by `getXxx()`
- ✅ save()/load() cycle preserves all settings fields
- ✅ Signals fire the expected number of times (no duplicate notifications)

### Android Client (`android/`)

- ✅ ViewModel state transitions are tested with `TestCoroutineDispatcher`
- ✅ `StreamingClient.connect()` tested with `FakeStreamingClient`
- ✅ `VideoDecoder.decode()` tested with a synthetic H.264 NAL unit
- ✅ `AudioEngine.feed()` tested with silence PCM data

### iOS Client (`ios/`)

- ✅ `VideoDecoder` output callback is invoked with non-nil CVPixelBuffer
- ✅ `StreamingClient` packet dispatch routes video to `VideoDecoder`
- ✅ `UserDefaultsManager` round-trips all settings fields

### React Web Dashboard (`frontend/`)

- ✅ Dashboard renders without error when WebSocket is disconnected
- ✅ Settings save calls `APIClient.updateVideoSettings()` with correct args
- ✅ WebSocket reconnection is attempted after disconnect

---

## 5. Testing Anti-Patterns to Avoid

| Anti-Pattern | Why It's Wrong | Fix |
|--------------|----------------|-----|
| Checking `rc == 0` only | Proves no crash, not correct output | Check the resulting state/output value |
| `QTest::qWait(500)` for every assertion | Flaky on slow CI, hides missing signals | Use QSignalSpy with 0 wait |
| Mocking everything | Tests the mock, not the real code | Use at most one mock per integration test |
| Testing through the UI layer | Too brittle, too slow | Test signal/slot layer directly |
| One test file per 20 functions | Coarse — failures are unlocatable | One test function per logical behaviour |

---

## 6. Running Integration Tests

```bash
# Build and run all integration tests (no external deps required)
gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I. \
    tests/integration/test_flowctl_metrics.c \
    src/flowctl/fc_params.c src/flowctl/fc_engine.c src/flowctl/fc_stats.c \
    src/metrics/mx_gauge.c src/metrics/mx_registry.c src/metrics/mx_snapshot.c \
    -o /tmp/test_flowctl_metrics && /tmp/test_flowctl_metrics

gcc -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -I. \
    tests/integration/test_sigroute_eventbus.c \
    src/sigroute/sr_signal.c src/sigroute/sr_route.c src/sigroute/sr_stats.c \
    src/eventbus/eb_bus.c src/eventbus/eb_event.c src/eventbus/eb_stats.c \
    -o /tmp/test_sigroute_eventbus && /tmp/test_sigroute_eventbus
```

---

## 7. What to Prompt for Next Testing Improvements

To continue improving test quality, use prompts such as:

```
"For the <module> subsystem, identify all observable side-effects of
<function()> and write an integration test that verifies each one reaches
its downstream consumer."

"Audit <file.cpp> for Q_PROPERTY declarations whose NOTIFY signal is never
actually emitted, and write a QSignalSpy test for each."

"Run the five-pass test review protocol on <feature> and list which passes
are currently missing."

"Write a Kotlin unit test for <ViewModel> using FakeStreamingClient that
verifies state transitions are not purely mock-driven."
```

---

## 8. Test Coverage Goals

| Layer | Current Coverage | Target |
|-------|-----------------|--------|
| C subsystem unit tests | ~95% modules have unit tests | 100% |
| C integration tests | 2 integration pairs | ≥1 per cross-module dependency |
| KDE client C++ | 3 test files | 1 per major QObject subclass |
| Android ViewModel | 0 | 1 per ViewModel |
| iOS | 1 test file (4 test cases) | +6 categories |
| Web dashboard | 0 test files | 4 test files |
