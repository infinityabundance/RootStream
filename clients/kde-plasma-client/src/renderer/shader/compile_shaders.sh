#!/bin/bash
# Shader compilation script for RootStream Vulkan renderer
# Requires glslangValidator or glslc (from Vulkan SDK)

set -e

SHADER_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Compiling shaders in: $SHADER_DIR"

# Check for shader compiler
if command -v glslangValidator &> /dev/null; then
    COMPILER="glslangValidator"
    COMPILE_CMD="glslangValidator -V"
elif command -v glslc &> /dev/null; then
    COMPILER="glslc"
    COMPILE_CMD="glslc"
else
    echo "Error: No shader compiler found!"
    echo "Please install Vulkan SDK or glslangValidator/glslc"
    echo ""
    echo "Ubuntu/Debian: sudo apt install glslang-tools"
    echo "Arch: sudo pacman -S glslang"
    echo "Fedora: sudo dnf install glslang"
    exit 1
fi

echo "Using compiler: $COMPILER"
echo ""

# Compile vertex shader
echo "Compiling fullscreen.vert..."
$COMPILE_CMD "$SHADER_DIR/fullscreen.vert" -o "$SHADER_DIR/fullscreen.vert.spv"
echo "  ✓ Generated fullscreen.vert.spv"

# Compile fragment shader
echo "Compiling nv12_to_rgb.frag..."
$COMPILE_CMD "$SHADER_DIR/nv12_to_rgb.frag" -o "$SHADER_DIR/nv12_to_rgb.frag.spv"
echo "  ✓ Generated nv12_to_rgb.frag.spv"

echo ""
echo "Shader compilation complete!"
echo ""
echo "Generated files:"
ls -lh "$SHADER_DIR"/*.spv
