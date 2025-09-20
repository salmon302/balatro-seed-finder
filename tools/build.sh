#!/bin/bash

# Simple build script for immolate program with filter selection

if [ $# -eq 0 ]; then
    echo "Usage: $0 <filter_name>"
    echo "Available filters:"
    echo "  perkeo            - Original Perkeo + Triboulet + Yorick filter"
    echo "  perkeo_only       - Only Perkeo filter"
    echo "  any_legendary     - Any legendary joker filter"
    echo "  charm_tag         - Simple charm tag filter"
    echo "  synergy_enum      - Early-game Joker synergy filter"
    echo ""
    echo "Example: $0 perkeo_only"
    exit 1
fi

FILTER_NAME=$1

# Validate filter exists
FILTER_FILE="filters/${FILTER_NAME}_filter.hpp"
if [ ! -f "$FILTER_FILE" ]; then
    echo "Error: Filter file $FILTER_FILE not found!"
    echo "Available filters:"
    for filter in filters/*_filter.hpp; do
        if [ -f "$filter" ]; then
            basename="$(basename "$filter" _filter.hpp)"
            echo "  $basename"
        fi
    done
    exit 1
fi

echo "Building immolate with filter: $FILTER_NAME"

# Create a preprocessor definition for the filter
FILTER_DEF="-DSELECTED_FILTER=\"filters/${FILTER_NAME}_filter.hpp\""

# Compile directly with g++, defining the filter to include
g++ -std=c++14 -g -DENABLE_LOGS -O3 "$FILTER_DEF" -ffp-contract=off -fexcess-precision=standard -o "dist/immolate_${FILTER_NAME}" immolate.cpp env.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful! Executable: ../dist/immolate_${FILTER_NAME}"
    echo "Filter: $FILTER_NAME"
else
    echo "Build failed!"
    exit 1
fi