#!/bin/bash

# Format all C/C++ files in HAL git repository using clang-format
# Respects .clang-format-ignore file
# With --check nothing is changed: the script lists the files that need formatting and fails

set -e

check=0
if [ "$1" = "--check" ]; then
    check=1
elif [ -n "$1" ]; then
    echo "Usage: $0 [--check]"
    exit 2
fi

# Go to git root
cd "$(dirname "$0")"
cd "$(git rev-parse --show-toplevel)"

# Build ignore pattern from .clang-format-ignore (skip comments and empty lines)
ignore_pattern=$(grep -v '^#\|^$' .clang-format-ignore | sed 's|/$||' | paste -sd'|')

# Find files, excluding ignored patterns
files=$(git ls-files '*.[ch]' | grep -vE "^($ignore_pattern)")

if [ "$check" -eq 1 ]; then
    failed=0
    for file in $files; do
        if ! clang-format --dry-run -Werror "$file" > /dev/null 2>&1; then
            echo "Needs formatting: $file"
            failed=1
        fi
    done
    if [ "$failed" -ne 0 ]; then
        echo "Run tests/fix_format.sh to fix"
        exit 1
    fi
    echo "Formatting is clean"
else
    for file in $files; do
        echo "Formatting: $file"
        clang-format -i "$file"
    done
    echo "Formatting completed"
fi
