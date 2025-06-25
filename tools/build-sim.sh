#!/bin/bash

# Simple build script for simulate
# Compile directly with g++, defining the filter to include
g++ -std=c++14 -g -DENABLE_LOGS -O3 -g -ffp-contract=off -fexcess-precision=standard -o "dist/simulate_enum" simulate_enum.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful! Executable: ../dist/simulate_enum"
else
    echo "Build failed!"
    exit 1
fi