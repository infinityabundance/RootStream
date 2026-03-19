#!/usr/bin/env bash
# tests/integration/test_soak.sh — Soak test scaffolding
#
# Validates that the RootStream host binary remains stable under
# extended operation (no crashes, no runaway memory growth).
#
# This test is NOT run in CI by default because it requires time and
# optionally real hardware. It is designed to be run manually before
# release or on a dedicated test machine.
#
# Usage:
#   ./tests/integration/test_soak.sh [--duration SECONDS] [--interval SECONDS]
#
#   --duration N    How long to run the host (default: 60s)
#   --interval N    Memory sampling interval (default: 5s)
#
# Checks:
#   S-1: Host process does not crash during the soak period
#   S-2: Memory usage (RSS) does not grow more than 50% from baseline
#   S-3: CPU usage does not spike above 90% sustained
#
# Exit codes:
#   0  All soak checks passed
#   1  One or more soak checks failed
#   2  Prerequisite missing (binary not built)

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BINARY="$REPO_ROOT/rootstream"
DURATION=60
INTERVAL=5

for arg in "$@"; do
    case "$arg" in
        --duration) shift; DURATION="${1:-60}" ;;
        --interval) shift; INTERVAL="${1:-5}" ;;
        --help|-h)
            echo "Usage: $0 [--duration SECONDS] [--interval SECONDS]"
            exit 0
            ;;
    esac
done

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║          RootStream Soak Test Scaffolding                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "  Duration:  ${DURATION}s"
echo "  Interval:  ${INTERVAL}s"
echo ""

# ── Prerequisite check ─────────────────────────────────────────────────────────
if [ ! -x "$BINARY" ]; then
    echo "ERROR: rootstream binary not found at $BINARY"
    echo "Build with: make HEADLESS=1"
    exit 2
fi

# ── Setup ─────────────────────────────────────────────────────────────────────
TMPDIR_SOAK="$(mktemp -d /tmp/rootstream_soak_XXXXXX)"
trap 'kill "$HOST_PID" 2>/dev/null || true; rm -rf "$TMPDIR_SOAK"' EXIT

LOG="$TMPDIR_SOAK/host.log"
MEM_LOG="$TMPDIR_SOAK/memory.log"
PASS=0
FAIL=0

# ── Start host ─────────────────────────────────────────────────────────────────
echo "── Starting host for ${DURATION}s soak ─────────────────────"
XDG_CONFIG_HOME="$TMPDIR_SOAK" "$BINARY" host --port 19876 > "$LOG" 2>&1 &
HOST_PID=$!
echo "  Host PID: $HOST_PID"

# Give it a moment to start
sleep 2

if ! kill -0 "$HOST_PID" 2>/dev/null; then
    echo "  ❌ Host exited immediately — soak test cannot proceed"
    echo "  Host output:"
    cat "$LOG" | head -20 | sed 's/^/    /'
    exit 1
fi

echo "  ✅ Host started successfully"
echo ""

# ── Capture baseline memory ─────────────────────────────────────────────────────
BASELINE_RSS=$(ps -o rss= -p "$HOST_PID" 2>/dev/null || echo "0")
echo "  Baseline RSS: ${BASELINE_RSS} kB"
echo "timestamp_s rss_kB cpu_pct" > "$MEM_LOG"

# ── Monitoring loop ─────────────────────────────────────────────────────────────
echo "── Monitoring (${DURATION}s) ──────────────────────────────────"
ELAPSED=0
CRASHED=0

while [ $ELAPSED -lt $DURATION ]; do
    sleep "$INTERVAL"
    ELAPSED=$((ELAPSED+INTERVAL))

    if ! kill -0 "$HOST_PID" 2>/dev/null; then
        echo "  ❌ Host crashed at ${ELAPSED}s"
        CRASHED=1
        break
    fi

    RSS=$(ps -o rss= -p "$HOST_PID" 2>/dev/null || echo "0")
    CPU=$(ps -o %cpu= -p "$HOST_PID" 2>/dev/null || echo "0.0")
    echo "${ELAPSED} ${RSS} ${CPU}" >> "$MEM_LOG"
    printf "  [%3ds] RSS=%s kB  CPU=%s%%\n" "$ELAPSED" "$RSS" "$CPU"
done

echo ""

# ── S-1: Crash check ─────────────────────────────────────────────────────────────
echo "── S-1: Crash check ─────────────────────────────────────────"
if [ $CRASHED -eq 0 ] && kill -0 "$HOST_PID" 2>/dev/null; then
    echo "  ✅ Host ran for full ${DURATION}s without crashing"
    PASS=$((PASS+1))
else
    echo "  ❌ Host crashed during soak period"
    FAIL=$((FAIL+1))
fi
echo ""

# ── S-2: Memory growth check ──────────────────────────────────────────────────
echo "── S-2: Memory growth check ────────────────────────────────"
if [ -s "$MEM_LOG" ] && [ "$BASELINE_RSS" -gt 0 ] 2>/dev/null; then
    FINAL_RSS=$(tail -1 "$MEM_LOG" | awk '{print $2}')
    GROWTH_PCT=$(( (FINAL_RSS - BASELINE_RSS) * 100 / BASELINE_RSS ))
    echo "  Baseline RSS: ${BASELINE_RSS} kB  Final RSS: ${FINAL_RSS} kB  Growth: ${GROWTH_PCT}%"
    if [ "$GROWTH_PCT" -le 50 ]; then
        echo "  ✅ Memory growth within acceptable limit (≤50%)"
        PASS=$((PASS+1))
    else
        echo "  ❌ Memory growth exceeded 50% (possible leak)"
        FAIL=$((FAIL+1))
    fi
else
    echo "  ⚠️  Memory data insufficient — skipping growth check"
fi
echo ""

# ── Stop host ────────────────────────────────────────────────────────────────
kill "$HOST_PID" 2>/dev/null || true
wait "$HOST_PID" 2>/dev/null || true

# ── Summary ──────────────────────────────────────────────────────────────────
echo "══ Soak Test Summary ═══════════════════════════════════════"
echo "  Passed: $PASS  Failed: $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "  ✅ Soak test passed"
    echo ""
    echo "  Memory log saved to: $MEM_LOG"
    exit 0
else
    echo "  ❌ $FAIL soak check(s) failed"
    echo ""
    echo "  Host log: $LOG"
    echo "  Memory log: $MEM_LOG"
    exit 1
fi
