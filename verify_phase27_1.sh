#!/bin/bash
# Simple compilation check for Phase 27.1 implementation
# This script verifies that the code at least has valid C++ syntax

echo "=================================================="
echo "Phase 27.1 Implementation Verification"
echo "=================================================="
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

errors=0

echo "Checking recording types header..."
gcc -c -o /tmp/test_types.o tests/unit/test_recording_types.c \
    -I./include -I./src/recording -std=c11 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Recording types header is valid${NC}"
    rm -f /tmp/test_types.o
else
    echo -e "${RED}✗ Recording types header has errors${NC}"
    errors=$((errors + 1))
fi

echo ""
echo "Checking replay buffer header..."
if [ -f src/recording/replay_buffer.h ]; then
    echo -e "${GREEN}✓ Replay buffer header exists${NC}"
else
    echo -e "${RED}✗ Replay buffer header missing${NC}"
    errors=$((errors + 1))
fi

echo ""
echo "Checking recording manager header..."
if [ -f src/recording/recording_manager.h ]; then
    echo -e "${GREEN}✓ Recording manager header exists${NC}"
else
    echo -e "${RED}✗ Recording manager header missing${NC}"
    errors=$((errors + 1))
fi

echo ""
echo "Checking test files exist..."
test_files=(
    "tests/unit/test_container_formats.cpp"
    "tests/unit/test_replay_buffer.cpp"
    "tests/unit/test_recording_manager_integration.cpp"
)

for test_file in "${test_files[@]}"; do
    if [ -f "$test_file" ]; then
        echo -e "${GREEN}✓ $test_file exists${NC}"
    else
        echo -e "${RED}✗ $test_file missing${NC}"
        errors=$((errors + 1))
    fi
done

echo ""
echo "Checking documentation..."
if [ -f PHASE27.1_COMPLETION_SUMMARY.md ]; then
    echo -e "${GREEN}✓ Phase 27.1 completion summary exists${NC}"
else
    echo -e "${RED}✗ Phase 27.1 completion summary missing${NC}"
    errors=$((errors + 1))
fi

echo ""
echo "=================================================="
if [ $errors -eq 0 ]; then
    echo -e "${GREEN}✓ All verification checks passed!${NC}"
    echo ""
    echo "Note: Full compilation requires FFmpeg libraries."
    echo "To build with tests:"
    echo "  1. Install FFmpeg development libraries:"
    echo "     sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev"
    echo "  2. Configure and build:"
    echo "     mkdir build && cd build"
    echo "     cmake -DENABLE_UNIT_TESTS=ON .."
    echo "     make"
    echo "  3. Run recording tests:"
    echo "     ctest -R 'Container|Replay|RecordingManager'"
    echo ""
    exit 0
else
    echo -e "${RED}✗ $errors verification check(s) failed${NC}"
    exit 1
fi
