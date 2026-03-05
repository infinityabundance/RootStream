#!/usr/bin/env bash
# test_full_stream.sh — End-to-end streaming test
# Starts server + KDE client in Docker; streams 60 seconds; validates output
#
# Usage: ./tests/e2e/test_full_stream.sh [--dry-run]
#
# Exit codes:
#   0  PASS
#   1  FAIL

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="${REPO_ROOT}/infrastructure/docker/docker-compose.yml"
DRY_RUN=false
STREAM_DURATION=60

for arg in "$@"; do
    case "${arg}" in
        --dry-run) DRY_RUN=true ;;
        *) echo "Unknown argument: ${arg}"; exit 1 ;;
    esac
done

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# ---------------------------------------------------------------------------
# Dry-run: validate config only
# ---------------------------------------------------------------------------
if ${DRY_RUN}; then
    echo -e "${YELLOW}==> [dry-run] Validating test configuration...${NC}"
    if [[ ! -f "${COMPOSE_FILE}" ]]; then
        echo -e "${RED}FAIL: docker-compose.yml not found at ${COMPOSE_FILE}${NC}"
        exit 1
    fi
    if ! command -v docker &>/dev/null; then
        echo -e "${RED}FAIL: docker not found${NC}"; exit 1
    fi
    if ! command -v docker-compose &>/dev/null && ! docker compose version &>/dev/null 2>&1; then
        echo -e "${RED}FAIL: docker-compose not found${NC}"; exit 1
    fi
    echo -e "${GREEN}PASS: Configuration is valid (dry-run)${NC}"
    exit 0
fi

# ---------------------------------------------------------------------------
# Compose helper (supports both 'docker compose' and 'docker-compose')
# ---------------------------------------------------------------------------
compose() {
    if docker compose version &>/dev/null 2>&1; then
        docker compose -f "${COMPOSE_FILE}" "$@"
    else
        docker-compose -f "${COMPOSE_FILE}" "$@"
    fi
}

# ---------------------------------------------------------------------------
# Cleanup on exit
# ---------------------------------------------------------------------------
cleanup() {
    echo -e "${YELLOW}==> Cleaning up containers...${NC}"
    compose down --remove-orphans --timeout 10 2>/dev/null || true
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Start services
# ---------------------------------------------------------------------------
echo -e "${YELLOW}==> Starting server container...${NC}"
compose up -d rootstream-server 2>&1

echo -e "${YELLOW}==> Waiting for server to be ready (up to 30s)...${NC}"
for i in $(seq 1 30); do
    if compose logs rootstream-server 2>&1 | grep -q "waiting for client"; then
        echo "  Server ready after ${i}s"
        break
    fi
    sleep 1
done

# ---------------------------------------------------------------------------
# Start test client and stream for STREAM_DURATION seconds
# ---------------------------------------------------------------------------
echo -e "${YELLOW}==> Starting test client (streaming ${STREAM_DURATION}s)...${NC}"
CLIENT_LOG=$(mktemp /tmp/rootstream-e2e-XXXXXX.log)
compose run --rm rootstream-client \
    --connect server --duration "${STREAM_DURATION}" \
    --stats-output /dev/stdout 2>&1 | tee "${CLIENT_LOG}" &
CLIENT_PID=$!

# Wait for client to finish
if ! wait "${CLIENT_PID}"; then
    echo -e "${RED}FAIL: Client process exited with error${NC}"
    exit 1
fi

# ---------------------------------------------------------------------------
# Parse and validate results
# ---------------------------------------------------------------------------
echo -e "${YELLOW}==> Validating results...${NC}"

FRAMES_RECEIVED=$(grep -oP 'frames_received=\K[0-9]+' "${CLIENT_LOG}" | tail -1 || echo "0")
FRAMES_DROPPED=$(grep -oP 'frames_dropped=\K[0-9]+' "${CLIENT_LOG}" | tail -1 || echo "0")
LATENCY_AVG=$(grep -oP 'latency_avg_ms=\K[0-9.]+' "${CLIENT_LOG}" | tail -1 || echo "N/A")
CONNECTED=$(grep -c "connection established" "${CLIENT_LOG}" || echo "0")

rm -f "${CLIENT_LOG}"

FAIL=false

if (( CONNECTED == 0 )); then
    echo -e "${RED}  ✗ Connection was never established${NC}"
    FAIL=true
else
    echo -e "${GREEN}  ✓ Connection established${NC}"
fi

if (( FRAMES_RECEIVED == 0 )); then
    echo -e "${RED}  ✗ No frames received${NC}"
    FAIL=true
else
    echo -e "${GREEN}  ✓ Frames received: ${FRAMES_RECEIVED}${NC}"
fi

if (( FRAMES_RECEIVED > 0 )); then
    DROP_PCT=$(echo "scale=2; ${FRAMES_DROPPED} * 100 / ${FRAMES_RECEIVED}" | bc)
    if (( $(echo "${DROP_PCT} > 1.0" | bc -l) )); then
        echo -e "${RED}  ✗ Dropped frame rate ${DROP_PCT}% exceeds 1% threshold${NC}"
        FAIL=true
    else
        echo -e "${GREEN}  ✓ Dropped frame rate: ${DROP_PCT}% (≤1%)${NC}"
    fi
fi

echo ""
echo "  Frames received : ${FRAMES_RECEIVED}"
echo "  Frames dropped  : ${FRAMES_DROPPED}"
echo "  Avg latency     : ${LATENCY_AVG} ms"
echo ""

if ${FAIL}; then
    echo -e "${RED}FAIL: E2E test did not meet acceptance criteria${NC}"
    exit 1
else
    echo -e "${GREEN}PASS: E2E streaming test passed${NC}"
fi
