#!/usr/bin/env bash
# run_sanitizers.sh — Build and test with ASan/UBSan/TSan
# Usage: ./scripts/run_sanitizers.sh [asan|ubsan|tsan|all]
#
# Modes:
#   asan   AddressSanitizer + UndefinedBehaviorSanitizer
#   ubsan  UndefinedBehaviorSanitizer only
#   tsan   ThreadSanitizer
#   all    Run all three modes sequentially (default)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_BASE="${REPO_ROOT}/build-sanitizers"
MODE="${1:-all}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

pass_count=0
fail_count=0

run_sanitizer() {
    local name="$1"
    local flags="$2"
    local build_dir="${BUILD_BASE}/${name}"

    echo ""
    echo -e "${CYAN}==> [${name}] Building with ${flags}...${NC}"

    cmake -S "${REPO_ROOT}" -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS="-fsanitize=${flags} -g -O1 -fno-omit-frame-pointer" \
        -DCMAKE_CXX_FLAGS="-fsanitize=${flags} -g -O1 -fno-omit-frame-pointer" \
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=${flags}" \
        -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=${flags}" \
        -Wno-dev -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF 2>&1 | tail -5

    cmake --build "${build_dir}" -- -j"$(nproc)" 2>&1 | tail -5

    echo -e "${CYAN}==> [${name}] Running ctest...${NC}"
    if (cd "${build_dir}" && ctest --output-on-failure 2>&1); then
        echo -e "${GREEN}PASS [${name}]: No sanitizer violations detected${NC}"
        (( pass_count++ )) || true
    else
        echo -e "${RED}FAIL [${name}]: Sanitizer violations detected!${NC}"
        (( fail_count++ )) || true
    fi
}

mkdir -p "${BUILD_BASE}"

case "${MODE}" in
    asan)  run_sanitizer "asan"  "address,undefined" ;;
    ubsan) run_sanitizer "ubsan" "undefined" ;;
    tsan)  run_sanitizer "tsan"  "thread" ;;
    all)
        run_sanitizer "asan"  "address,undefined"
        run_sanitizer "ubsan" "undefined"
        run_sanitizer "tsan"  "thread"
        ;;
    *)
        echo "Unknown mode: ${MODE}. Use asan, ubsan, tsan, or all."
        exit 1
        ;;
esac

echo ""
echo -e "Results: ${GREEN}${pass_count} passed${NC}, ${RED}${fail_count} failed${NC}"

if (( fail_count > 0 )); then
    exit 1
fi
