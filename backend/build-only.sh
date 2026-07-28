#!/bin/bash

# Build-only script (without running)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "Building Cortex Code Intelligence Platform..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
cmake --build .

echo "✓ Build complete. Executable: $BUILD_DIR/bin/cortex"
echo "Run it with: ./build.sh"
