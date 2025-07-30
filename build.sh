#!/bin/bash
# Build script for IKRT project

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Run cmake to configure the project
cmake -Wno-dev ..

# Build the project
make -j$(nproc)

echo "Build completed. Executable is in build/IKRT"
