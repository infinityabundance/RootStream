#!/usr/bin/env bash
# validate_traceability.sh — Gate script for docs/microtasks.md
# Exits 0 on full success, 1 on any failure.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MICROTASKS="$REPO_ROOT/docs/microtasks.md"
PASS=0
FAIL=0

pass() { echo "  ✅  $*"; PASS=$((PASS + 1)); }
fail() { echo "  ❌  $*"; FAIL=$((FAIL + 1)); }

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  RootStream Traceability Validator"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# ── 1. microtasks.md exists ───────────────────────────
echo "[ 1 ] Checking docs/microtasks.md exists..."
if [[ -f "$MICROTASKS" ]]; then
    pass "docs/microtasks.md found ($(wc -l < "$MICROTASKS") lines)"
else
    fail "docs/microtasks.md NOT found at $MICROTASKS"
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  RESULT: FAILED — microtasks.md missing"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    exit 1
fi
echo ""

# ── 2. All required PHASE-NN headers present ─────────
echo "[ 2 ] Checking phase IDs PHASE-00 through PHASE-38..."
ALL_PHASES_OK=true
for i in $(seq -w 0 38); do
    PHASE_ID="PHASE-${i}"
    if grep -q "$PHASE_ID" "$MICROTASKS"; then
        pass "$PHASE_ID present"
    else
        fail "$PHASE_ID NOT found in microtasks.md"
        ALL_PHASES_OK=false
    fi
done
echo ""

# ── 3. Gate scripts that are referenced actually exist ─
echo "[ 3 ] Checking gate scripts referenced in microtasks.md..."

# Collect unique gate script paths from the Gate column
mapfile -t GATES < <(grep -oP '`[^`]+\.sh`' "$MICROTASKS" | tr -d '`' | sort -u)

if [[ ${#GATES[@]} -eq 0 ]]; then
    fail "No gate scripts found in microtasks.md"
else
    for GATE in "${GATES[@]}"; do
        # Gate paths are relative to repo root
        GATE_PATH="$REPO_ROOT/$GATE"
        if [[ -f "$GATE_PATH" ]]; then
            pass "Gate script exists: $GATE"
        else
            fail "Gate script MISSING: $GATE (expected at $GATE_PATH)"
        fi
    done
fi
echo ""

# ── 4. docs/archive directory exists ─────────────────
echo "[ 4 ] Checking docs/archive/ directory..."
if [[ -d "$REPO_ROOT/docs/archive" ]]; then
    ARCHIVE_COUNT=$(find "$REPO_ROOT/docs/archive" -type f | wc -l)
    pass "docs/archive/ exists with $ARCHIVE_COUNT file(s)"
else
    fail "docs/archive/ directory NOT found"
fi
echo ""

# ── 5. Summary ────────────────────────────────────────
TOTAL=$((PASS + FAIL))
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Validation Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Checks passed : $PASS / $TOTAL"
echo "  Checks failed : $FAIL / $TOTAL"

if [[ $FAIL -eq 0 ]]; then
    echo ""
    echo "  ✅  All checks passed — traceability is intact."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    exit 0
else
    echo ""
    echo "  ❌  $FAIL check(s) failed — see details above."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    exit 1
fi
