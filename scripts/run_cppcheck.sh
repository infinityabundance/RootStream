#!/usr/bin/env bash
# run_cppcheck.sh — Run cppcheck static analysis on src/ and clients/
# Usage: ./scripts/run_cppcheck.sh [--xml]

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
XML_MODE=false
XML_OUTPUT="${REPO_ROOT}/build/cppcheck-report.xml"

for arg in "$@"; do
    case "${arg}" in
        --xml) XML_MODE=true ;;
        *) echo "Unknown argument: ${arg}"; exit 1 ;;
    esac
done

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

if ! command -v cppcheck &>/dev/null; then
    echo -e "${RED}ERROR: cppcheck not found. Install with: sudo pacman -S cppcheck${NC}"
    exit 1
fi

CPPCHECK_ARGS=(
    --error-exitcode=1
    --enable=warning,style,performance,portability
    --suppress=missingIncludeSystem
    --std=c17
    --platform=unix64
    --quiet
)

if ${XML_MODE}; then
    mkdir -p "$(dirname "${XML_OUTPUT}")"
    CPPCHECK_ARGS+=(--xml --xml-version=2)
    echo -e "${YELLOW}==> Running cppcheck (XML output → ${XML_OUTPUT})...${NC}"
    cppcheck "${CPPCHECK_ARGS[@]}" \
        "${REPO_ROOT}/src/" "${REPO_ROOT}/clients/" \
        2>"${XML_OUTPUT}" && RC=0 || RC=$?
    echo "Report written to: ${XML_OUTPUT}"
else
    echo -e "${YELLOW}==> Running cppcheck on src/ and clients/...${NC}"
    cppcheck "${CPPCHECK_ARGS[@]}" \
        "${REPO_ROOT}/src/" "${REPO_ROOT}/clients/" && RC=0 || RC=$?
fi

if (( RC != 0 )); then
    echo -e "${RED}FAIL: cppcheck found errors (exit code ${RC})${NC}"
    exit "${RC}"
else
    echo -e "${GREEN}PASS: cppcheck found no errors${NC}"
fi
