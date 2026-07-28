#!/bin/bash

# Clean script - removes build artifacts
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "Cleaning build artifacts..."
rm -rf "$BUILD_DIR"
echo "✓ Clean complete"
