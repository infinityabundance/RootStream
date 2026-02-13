#!/bin/bash
# Minimal test to verify recording system compiles independently

echo "Testing recording types header..."
gcc -c -o /tmp/test_types.o tests/unit/test_recording_types.c \
    -I./include -I./src/recording -std=c11 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Recording types test compiles successfully"
    rm -f /tmp/test_types.o
else
    echo "✗ Recording types test failed to compile"
    exit 1
fi

echo ""
echo "Testing disk manager (requires C++17)..."
g++ -c -o /tmp/test_disk.o src/recording/disk_manager.cpp \
    -I./include -I./src/recording -std=c++17 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Disk manager compiles successfully"
    rm -f /tmp/test_disk.o
else
    echo "✗ Disk manager failed to compile"
    exit 1
fi

echo ""
echo "Testing recording manager header..."
g++ -c -o /tmp/test_mgr_hdr.o -x c++ - <<EOF 2>&1
#include "src/recording/recording_manager.h"
int main() { return 0; }
EOF

if [ $? -eq 0 ]; then
    echo "✓ Recording manager header is valid"
    rm -f /tmp/test_mgr_hdr.o
else
    echo "✗ Recording manager header has errors"
    exit 1
fi

echo ""
echo "✓ All basic compilation tests passed!"
echo ""
echo "Note: Full build requires:"
echo "  - FFmpeg libraries (libavformat, libavcodec, libavutil)"
echo "  - SDL2"
echo "  - Other RootStream dependencies"
