#!/bin/bash
# Format all C++ source files using clang-format

set -e

echo "Formatting C++ code with clang-format..."

# Find all .cpp and .h files in src/
find src -type f \( -name "*.cpp" -o -name "*.h" \) | while read file; do
    echo "  Formatting: $file"
    clang-format -i "$file"
done

echo "✓ Code formatting complete!"
