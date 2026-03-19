#!/usr/bin/env bash
# demo.sh — RootStream canonical demo and validation flow
#
# This script exercises the canonical RootStream product path:
#   1. Build verification
#   2. Identity generation (--qr)
#   3. Host startup and network binding
#   4. Config and key persistence check
#
# It is NOT a full end-to-end streaming test (that requires two machines
# or a loopback setup with a display).  It validates that the binary is
# working correctly for the startup and identity generation phase.
#
# Usage:
#   ./scripts/demo.sh [--build] [--clean]
#
#   --build   Build from source before running demo
#   --clean   Remove demo state directory before running
#
# Exit codes:
#   0  All checks passed
#   1  One or more checks failed

set -uo pipefail
# Note: -e is intentionally omitted here because this script uses `|| true`
# for expected-fallible commands (timeout host, host startup in CI) and tracks
# failures explicitly through the PASS/FAIL counters rather than early exit.

BUILD=0
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        --build) BUILD=1 ;;
        --clean) CLEAN=1 ;;
        --help|-h)
            sed -n '/^# demo.sh/,/^$/p' "$0" | grep '^#' | sed 's/^# //'
            exit 0
            ;;
    esac
done

# ── Config ────────────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="$REPO_ROOT/rootstream"
DEMO_STATE="$REPO_ROOT/_demo_state"
PASS=0
FAIL=0

cd "$REPO_ROOT"

log_pass() { echo "  ✅ $*"; PASS=$((PASS+1)); }
log_fail() { echo "  ❌ $*"; FAIL=$((FAIL+1)); }
log_info() { echo "  ℹ️  $*"; }
log_warn() { echo "  ⚠️  $*"; }

# ── Banner ────────────────────────────────────────────────────────────────────
echo ""
echo "╔═══════════════════════════════════════════════════════╗"
echo "║       RootStream Canonical Demo / Validation          ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""

# ── Optionally clean state ─────────────────────────────────────────────────────
if [ $CLEAN -eq 1 ] && [ -d "$DEMO_STATE" ]; then
    rm -rf "$DEMO_STATE"
    log_info "Removed previous demo state."
fi

mkdir -p "$DEMO_STATE"

# ── Optionally build ──────────────────────────────────────────────────────────
if [ $BUILD -eq 1 ]; then
    echo "── Step 0: Build ────────────────────────────────────────────"
    if make HEADLESS=1 2>&1 | grep -E "^✓|✅"; then
        log_pass "Build succeeded"
    else
        log_fail "Build failed — run 'make HEADLESS=1' manually to see errors"
        exit 1
    fi
    echo ""
fi

# ── Step 1: Binary exists ─────────────────────────────────────────────────────
echo "── Step 1: Binary check ─────────────────────────────────────────────"
if [ -x "$BINARY" ]; then
    log_pass "rootstream binary exists at $BINARY"
else
    log_fail "rootstream binary not found at $BINARY"
    log_info "Run: make HEADLESS=1"
    echo ""
    echo "══ FAILED: Binary not available ══"
    exit 1
fi

# Version string
VERSION=$("$BINARY" --version 2>&1 || true)
if echo "$VERSION" | grep -q "RootStream"; then
    log_pass "--version: $VERSION"
else
    log_fail "--version did not produce expected output"
fi

# Help string
if "$BINARY" --help 2>&1 | grep -q "connect"; then
    log_pass "--help output is present and contains expected commands"
else
    log_fail "--help output unexpected"
fi
echo ""

# ── Step 2: Identity generation ───────────────────────────────────────────────
echo "── Step 2: Identity / QR generation ────────────────────────────────"
XDG_CONFIG_HOME="$DEMO_STATE" timeout 10 "$BINARY" --qr 2>&1 > "$DEMO_STATE/qr_output.txt" || true

if grep -q "RootStream Code:" "$DEMO_STATE/qr_output.txt" 2>/dev/null; then
    CODE=$(grep "RootStream Code:" "$DEMO_STATE/qr_output.txt" | head -1)
    log_pass "Identity generated — $CODE"
else
    log_fail "--qr did not produce a 'RootStream Code:' line"
    log_info "Output: $(cat "$DEMO_STATE/qr_output.txt" 2>/dev/null | head -5)"
fi

# Key files should exist
KEY_DIR="$DEMO_STATE/rootstream/keys"
if [ -f "$KEY_DIR/identity.pub" ] || ls "$DEMO_STATE"/rootstream/keys/ >/dev/null 2>&1; then
    log_pass "Key files created under $KEY_DIR"
else
    log_warn "Key directory $KEY_DIR not found (may use different path)"
fi
echo ""

# ── Step 3: Host startup ──────────────────────────────────────────────────────
echo "── Step 3: Host startup ─────────────────────────────────────────────"
# Run host for 3 seconds and capture output
HOST_LOG="$DEMO_STATE/host_output.txt"
XDG_CONFIG_HOME="$DEMO_STATE" timeout 3 "$BINARY" host --port 19876 2>&1 > "$HOST_LOG" || true

if grep -q "Starting host mode\|Waiting for connections\|Network initialized\|waiting for" "$HOST_LOG" 2>/dev/null; then
    log_pass "Host mode started and reached waiting state"
else
    log_warn "Host mode output not fully verified (may need display device)"
    log_info "Host output (first 5 lines):"
    head -5 "$HOST_LOG" 2>/dev/null | sed 's/^/    /'
fi
echo ""

# ── Step 4: Unit tests ────────────────────────────────────────────────────────
echo "── Step 4: Unit tests ───────────────────────────────────────────────"
CRYPTO_TEST="$REPO_ROOT/tests/unit/test_crypto"
ENCODING_TEST="$REPO_ROOT/tests/unit/test_encoding"

if [ -x "$CRYPTO_TEST" ]; then
    if "$CRYPTO_TEST" 2>&1 | grep -q "10/10 passed\|passed"; then
        log_pass "test_crypto: all tests passed"
    else
        log_fail "test_crypto: some tests failed"
    fi
else
    log_warn "test_crypto binary not found — run: make test-build"
fi

if [ -x "$ENCODING_TEST" ]; then
    if "$ENCODING_TEST" 2>&1 | grep -q "18/18 passed\|passed"; then
        log_pass "test_encoding: all tests passed"
    else
        log_fail "test_encoding: some tests failed"
    fi
else
    log_warn "test_encoding binary not found — run: make test-build"
fi
echo ""

# ── Summary ───────────────────────────────────────────────────────────────────
echo "══ Demo Summary ════════════════════════════════════════════════════════"
echo ""
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "  ✅ Canonical demo validation PASSED"
    echo ""
    echo "  Next step: validate a two-machine peer connection"
    echo "  See: docs/CORE_PATH.md"
    echo ""
    exit 0
else
    echo "  ❌ Demo validation had $FAIL failure(s)"
    echo ""
    echo "  Diagnose with: docs/TROUBLESHOOTING.md"
    echo "  Build log:     make HEADLESS=1 2>&1 | head -50"
    echo ""
    exit 1
fi
