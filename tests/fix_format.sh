#!/bin/bash

# Format all C/C++ files in HAL git repository using clang-format
# Respects .clang-format-ignore file

set -e

# Go to git root
cd "$(dirname "$0")"
cd "$(git rev-parse --show-toplevel)"

# Build ignore pattern from .clang-format-ignore (skip comments and empty lines)
ignore_pattern=$(grep -v '^#\|^$' .clang-format-ignore | sed 's|/$||' | paste -sd'|')

# Find and format files, excluding ignored patterns
git ls-files '*.[ch]' | grep -vE "^($ignore_pattern)" | while read -r file; do
    echo "Formatting: $file"
    clang-format -i "$file"
done

echo "Formatting completed"
