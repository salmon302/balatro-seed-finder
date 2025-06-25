#!/bin/bash

echo "Building for Instruments profiling..."

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

# Build with debug symbols but optimized
g++ -std=c++14 -O2 -g "$FILTER_DEF" -ffp-contract=off -fexcess-precision=standard -o dist/immolate_instruments immolate.cpp

if [ $? -eq 0 ]; then
    echo "Instruments build successful!"
    echo ""
    echo "To profile with Instruments:"
    echo "  1. Open Instruments.app"
    echo "  2. Choose 'Time Profiler' template"
    echo "  3. Click 'Choose Target' -> 'Existing Process' -> Browse to ../dist/immolate_instruments"
    echo "  4. Set arguments to: AAAAAAAA"
    echo "  5. Record for 30-60 seconds"
    echo ""
    echo "Or use command line:"
    echo "  xcrun xctrace record --template 'Time Profiler' --launch ../dist/immolate_instruments AAAAAAAA --time-limit 30s"
else
    echo "Instruments build failed!"
    exit 1
fi