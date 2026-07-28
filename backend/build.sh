#!/bin/bash

# Cortex Code Intelligence Platform - Build and Run Script
# This script builds the backend and starts the Drogon server

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Define directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
EXECUTABLE="$BUILD_DIR/bin/cortex"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Cortex Code Intelligence Platform${NC}"
echo -e "${BLUE}Build & Run Script${NC}"
echo -e "${BLUE}========================================${NC}"

# Step 1: Create build directory if it doesn't exist
echo -e "\n${BLUE}[1/4]${NC} Setting up build directory..."
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo -e "${GREEN}✓${NC} Created $BUILD_DIR"
else
    echo -e "${GREEN}✓${NC} Build directory exists"
fi

# Step 2: Run CMake
echo -e "\n${BLUE}[2/4]${NC} Running CMake..."
cd "$BUILD_DIR"
cmake .. || { echo -e "${RED}✗ CMake failed${NC}"; exit 1; }
echo -e "${GREEN}✓${NC} CMake configuration complete"

# Step 3: Build the project
echo -e "\n${BLUE}[3/4]${NC} Building project..."
cmake --build . || { echo -e "${RED}✗ Build failed${NC}"; exit 1; }
echo -e "${GREEN}✓${NC} Build successful"

# Copy config file to build directory
echo -e "\n${BLUE}[3.5/4]${NC} Copying configuration..."
mkdir -p "$BUILD_DIR/config"
cp "$SCRIPT_DIR/config/config.json" "$BUILD_DIR/config/" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Configuration files ready"

# Step 4: Run the executable
echo -e "\n${BLUE}[4/4]${NC} Starting Drogon server..."
echo -e "${BLUE}========================================${NC}"
if [ -f "$EXECUTABLE" ]; then
    "$EXECUTABLE"
else
    echo -e "${RED}✗ Executable not found: $EXECUTABLE${NC}"
    exit 1
fi
