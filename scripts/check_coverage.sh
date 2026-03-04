#!/usr/bin/env bash
# check_coverage.sh — Generate and check unit test coverage
# Usage: ./scripts/check_coverage.sh [min_coverage_pct]
#
# Options:
#   --html    Open the HTML report in the default browser after generation
#   [pct]     Minimum line coverage percentage required (default: 80)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-coverage"
REPORT_DIR="${BUILD_DIR}/coverage"
MIN_COVERAGE=80
OPEN_HTML=false

for arg in "$@"; do
    case "${arg}" in
        --html) OPEN_HTML=true ;;
        [0-9]*) MIN_COVERAGE="${arg}" ;;
        *) echo "Unknown argument: ${arg}"; exit 1 ;;
    esac
done

# Colors
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${YELLOW}==> Building with coverage instrumentation...${NC}"
cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Coverage \
    -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0" \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage -O0" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"
cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

echo -e "${YELLOW}==> Running tests...${NC}"
(cd "${BUILD_DIR}" && ctest --output-on-failure)

echo -e "${YELLOW}==> Collecting coverage data with lcov...${NC}"
mkdir -p "${REPORT_DIR}"
lcov --capture \
     --directory "${BUILD_DIR}" \
     --base-directory "${REPO_ROOT}" \
     --output-file "${REPORT_DIR}/coverage.info" \
     --no-external \
     --quiet

# Remove test files from report
lcov --remove "${REPORT_DIR}/coverage.info" \
     '*/tests/*' '*/benchmarks/*' \
     --output-file "${REPORT_DIR}/coverage.info" \
     --quiet

echo -e "${YELLOW}==> Generating HTML report to ${REPORT_DIR}/html/...${NC}"
genhtml "${REPORT_DIR}/coverage.info" \
        --output-directory "${REPORT_DIR}/html" \
        --title "RootStream Coverage" \
        --quiet

# Extract line coverage percentage
COVERAGE=$(lcov --summary "${REPORT_DIR}/coverage.info" 2>&1 \
    | grep -oP 'lines\.*: \K[0-9.]+')

echo ""
echo -e "Line coverage: ${YELLOW}${COVERAGE}%${NC}  (minimum: ${MIN_COVERAGE}%)"

if (( $(echo "${COVERAGE} < ${MIN_COVERAGE}" | bc -l) )); then
    echo -e "${RED}FAIL: Coverage ${COVERAGE}% is below minimum ${MIN_COVERAGE}%${NC}"
    exit 1
else
    echo -e "${GREEN}PASS: Coverage ${COVERAGE}% meets minimum ${MIN_COVERAGE}%${NC}"
fi

if ${OPEN_HTML}; then
    xdg-open "${REPORT_DIR}/html/index.html" 2>/dev/null || \
        echo "Report: ${REPORT_DIR}/html/index.html"
else
    echo "Report: ${REPORT_DIR}/html/index.html"
fi
