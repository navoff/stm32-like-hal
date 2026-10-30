#!/bin/bash

# Exit on any error
set -e

# Change to the script directory
cd "$(dirname "$0")"

echo "=== Building all presets ==="

rm -rf build

# Get list of configure presets
configure_presets=$(cmake --list-presets=configure 2>/dev/null | grep -E '^\s*"' | sed 's/.*"\([^"]*\)".*/\1/')

if [ -z "$configure_presets" ]; then
    echo "Error: No configure presets found"
    exit 1
fi

# Get list of build presets
build_presets=$(cmake --list-presets=build 2>/dev/null | grep -E '^\s*"' | sed 's/.*"\([^"]*\)".*/\1/')

if [ -z "$build_presets" ]; then
    echo "Error: No build presets found"
    exit 1
fi

echo "Found configure presets: $configure_presets"
echo "Found build presets: $build_presets"
echo ""

# Build each configure preset with all its build variants
for config_preset in $configure_presets; do
    echo "=== Processing configure preset: $config_preset ==="

    # Clean and configure
    echo "Configuring with preset: $config_preset"
    cmake --preset "$config_preset"

    # Find and build all related build presets
    for build_preset in $build_presets; do
        if [[ "$build_preset" == "$config_preset"* ]]; then
            echo "Building with preset: $build_preset"

            # Temporarily disable exit on error to catch build failures
            set +e
            cmake --build --preset "$build_preset"
            build_result=$?
            set -e

            if [ $build_result -ne 0 ]; then
                echo "ERROR: Build failed for preset: $build_preset"
                exit 1
            fi

            echo "Successfully built preset $build_preset"
            echo ""
        fi
    done

    echo "=== Finished configure preset: $config_preset ==="
    echo ""
done

echo "=== Build all completed ==="
