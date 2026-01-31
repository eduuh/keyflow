#!/bin/bash
# Run clang-tidy on all C++ source files

set -e

echo "Running clang-tidy analysis..."

# Find all .cpp files in src/
find src -type f -name "*.cpp" | while read file; do
    echo "  Analyzing: $file"
    clang-tidy "$file" -- -Isrc -Iexternal -Isrc/platform || true
done

echo "✓ Static analysis complete!"
