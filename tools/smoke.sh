#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/build"
cmake --build "${ROOT_DIR}/build"
ctest --test-dir "${ROOT_DIR}/build"

"${ROOT_DIR}/build/rootstream" --version
"${ROOT_DIR}/build/rootstream" --help >/dev/null
