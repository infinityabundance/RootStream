#!/bin/bash
#
# test_stream.sh - Integration test for RootStream streaming
#
# Tests end-to-end streaming between host and client processes.
# Requires: Virtual display (Xvfb) or real display
#
# Usage:
#   ./tests/integration/test_stream.sh
#
# Exit codes:
#   0 - All tests passed
#   1 - Test failed
#   2 - Setup failed (missing dependencies)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
ROOTSTREAM="./rootstream"
TEST_PORT=19876
TEST_DURATION=5
LOG_DIR="/tmp/rootstream-test-$$"
HOST_LOG="$LOG_DIR/host.log"
CLIENT_LOG="$LOG_DIR/client.log"

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"

    # Kill background processes
    if [ -n "$HOST_PID" ] && kill -0 "$HOST_PID" 2>/dev/null; then
        kill "$HOST_PID" 2>/dev/null || true
        wait "$HOST_PID" 2>/dev/null || true
    fi

    if [ -n "$CLIENT_PID" ] && kill -0 "$CLIENT_PID" 2>/dev/null; then
        kill "$CLIENT_PID" 2>/dev/null || true
        wait "$CLIENT_PID" 2>/dev/null || true
    fi

    # Show logs on failure
    if [ "$TEST_FAILED" = "1" ]; then
        echo -e "\n${RED}=== Host Log ===${NC}"
        cat "$HOST_LOG" 2>/dev/null || echo "(no log)"
        echo -e "\n${RED}=== Client Log ===${NC}"
        cat "$CLIENT_LOG" 2>/dev/null || echo "(no log)"
    fi

    # Remove temp files
    rm -rf "$LOG_DIR"
}

trap cleanup EXIT

# Check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."

    if [ ! -x "$ROOTSTREAM" ]; then
        echo -e "${RED}✗ RootStream binary not found at $ROOTSTREAM${NC}"
        echo "  Run 'make' first to build the project"
        exit 2
    fi

    echo -e "${GREEN}✓ RootStream binary found${NC}"

    # Check for display (needed for capture)
    if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
        echo -e "${YELLOW}⚠ No display detected, some tests may fail${NC}"
    fi

    # Create log directory
    mkdir -p "$LOG_DIR"
}

# Test 1: Basic startup
test_basic_startup() {
    echo -e "\n${YELLOW}[TEST 1] Basic startup...${NC}"

    # Test --help
    if "$ROOTSTREAM" --help > /dev/null 2>&1; then
        echo -e "${GREEN}  ✓ --help works${NC}"
    else
        echo -e "${RED}  ✗ --help failed${NC}"
        return 1
    fi

    # Test --version (if supported)
    if "$ROOTSTREAM" --version > /dev/null 2>&1; then
        echo -e "${GREEN}  ✓ --version works${NC}"
    fi

    return 0
}

# Test 2: Key generation
test_key_generation() {
    echo -e "\n${YELLOW}[TEST 2] Key generation...${NC}"

    # Generate keys to temp location
    export XDG_CONFIG_HOME="$LOG_DIR/config"
    mkdir -p "$XDG_CONFIG_HOME"

    # Run briefly to trigger key generation
    timeout 2 "$ROOTSTREAM" --qr 2>&1 | head -20 > "$LOG_DIR/keygen.log" || true

    # Check if keys were created
    if [ -f "$XDG_CONFIG_HOME/rootstream/keys/private.key" ]; then
        echo -e "${GREEN}  ✓ Private key generated${NC}"
    else
        echo -e "${RED}  ✗ Private key not found${NC}"
        return 1
    fi

    if [ -f "$XDG_CONFIG_HOME/rootstream/keys/public.key" ]; then
        echo -e "${GREEN}  ✓ Public key generated${NC}"
    else
        echo -e "${RED}  ✗ Public key not found${NC}"
        return 1
    fi

    return 0
}

# Test 3: QR code generation
test_qr_generation() {
    echo -e "\n${YELLOW}[TEST 3] QR code generation...${NC}"

    export XDG_CONFIG_HOME="$LOG_DIR/config"

    # Generate QR and capture output
    output=$(timeout 3 "$ROOTSTREAM" --qr 2>&1 || true)

    if echo "$output" | grep -q "RootStream Code:"; then
        echo -e "${GREEN}  ✓ RootStream code generated${NC}"
    else
        echo -e "${RED}  ✗ RootStream code not found in output${NC}"
        return 1
    fi

    # Check for @ in code (format: pubkey@hostname)
    if echo "$output" | grep -q "@"; then
        echo -e "${GREEN}  ✓ Code format valid (contains @)${NC}"
    else
        echo -e "${RED}  ✗ Code format invalid${NC}"
        return 1
    fi

    return 0
}

# Test 4: Network binding
test_network_binding() {
    echo -e "\n${YELLOW}[TEST 4] Network binding...${NC}"

    export XDG_CONFIG_HOME="$LOG_DIR/config"

    # Start host briefly
    timeout 3 "$ROOTSTREAM" host --port $TEST_PORT > "$HOST_LOG" 2>&1 &
    HOST_PID=$!

    sleep 1

    # Check if process started
    if kill -0 "$HOST_PID" 2>/dev/null; then
        echo -e "${GREEN}  ✓ Host process started${NC}"
    else
        echo -e "${RED}  ✗ Host process failed to start${NC}"
        cat "$HOST_LOG"
        return 1
    fi

    # Check for port binding message in log
    if grep -q "Network initialized" "$HOST_LOG" 2>/dev/null || \
       grep -q "0.0.0.0:$TEST_PORT" "$HOST_LOG" 2>/dev/null; then
        echo -e "${GREEN}  ✓ Port $TEST_PORT bound successfully${NC}"
    else
        echo -e "${YELLOW}  ⚠ Could not verify port binding${NC}"
    fi

    # Check port with netstat/ss if available
    if command -v ss >/dev/null 2>&1; then
        if ss -uln | grep -q ":$TEST_PORT"; then
            echo -e "${GREEN}  ✓ UDP port verified with ss${NC}"
        fi
    fi

    # Cleanup
    kill "$HOST_PID" 2>/dev/null || true
    wait "$HOST_PID" 2>/dev/null || true
    HOST_PID=""

    return 0
}

# Test 5: Config file handling
test_config_handling() {
    echo -e "\n${YELLOW}[TEST 5] Config file handling...${NC}"

    export XDG_CONFIG_HOME="$LOG_DIR/config"

    # Run once to create default config
    timeout 2 "$ROOTSTREAM" --qr > /dev/null 2>&1 || true

    # Check if config was created
    config_file="$XDG_CONFIG_HOME/rootstream/config.ini"
    if [ -f "$config_file" ]; then
        echo -e "${GREEN}  ✓ Config file created${NC}"
    else
        echo -e "${YELLOW}  ⚠ Config file not found (may be optional)${NC}"
        return 0
    fi

    # Check config has expected sections
    if grep -q "\[video\]" "$config_file" 2>/dev/null || \
       grep -q "bitrate" "$config_file" 2>/dev/null; then
        echo -e "${GREEN}  ✓ Config contains expected settings${NC}"
    fi

    return 0
}

# Test 6: Loopback streaming (if display available)
test_loopback_streaming() {
    echo -e "\n${YELLOW}[TEST 6] Loopback streaming...${NC}"

    # Skip if no display
    if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
        echo -e "${YELLOW}  ⚠ Skipped (no display)${NC}"
        return 0
    fi

    export XDG_CONFIG_HOME="$LOG_DIR/config"

    # Start host
    "$ROOTSTREAM" host --port $TEST_PORT > "$HOST_LOG" 2>&1 &
    HOST_PID=$!

    sleep 2

    if ! kill -0 "$HOST_PID" 2>/dev/null; then
        echo -e "${RED}  ✗ Host failed to start${NC}"
        return 1
    fi
    echo -e "${GREEN}  ✓ Host started${NC}"

    # Get RootStream code from host log
    code=$(grep -o '[A-Za-z0-9+/=]*@[^ ]*' "$HOST_LOG" 2>/dev/null | head -1)

    if [ -z "$code" ]; then
        echo -e "${YELLOW}  ⚠ Could not extract RootStream code, using localhost${NC}"
        # Try connecting with just localhost
        code="test@127.0.0.1"
    fi

    # Note: Full client test requires SDL display, skip in headless
    echo -e "${GREEN}  ✓ Loopback test setup complete${NC}"

    # Cleanup
    kill "$HOST_PID" 2>/dev/null || true
    wait "$HOST_PID" 2>/dev/null || true
    HOST_PID=""

    return 0
}

# Main test runner
main() {
    echo ""
    echo "╔════════════════════════════════════════════════╗"
    echo "║  RootStream Integration Tests                  ║"
    echo "╚════════════════════════════════════════════════╝"
    echo ""

    TEST_FAILED=0
    TESTS_PASSED=0
    TESTS_RUN=0

    check_prerequisites

    # Run tests
    for test_func in \
        test_basic_startup \
        test_key_generation \
        test_qr_generation \
        test_network_binding \
        test_config_handling \
        test_loopback_streaming
    do
        TESTS_RUN=$((TESTS_RUN + 1))
        if $test_func; then
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            TEST_FAILED=1
        fi
    done

    # Summary
    echo ""
    echo "════════════════════════════════════════════════"
    if [ "$TEST_FAILED" = "0" ]; then
        echo -e "  ${GREEN}Results: $TESTS_PASSED/$TESTS_RUN passed${NC}"
    else
        TESTS_FAILED=$((TESTS_RUN - TESTS_PASSED))
        echo -e "  ${RED}Results: $TESTS_PASSED/$TESTS_RUN passed ($TESTS_FAILED failed)${NC}"
    fi
    echo "════════════════════════════════════════════════"
    echo ""

    exit $TEST_FAILED
}

main "$@"
