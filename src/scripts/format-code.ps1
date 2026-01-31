# Format all C++ source files using clang-format (PowerShell version)

Write-Host "Formatting C++ code with clang-format..." -ForegroundColor Cyan

# Find all .cpp and .h files in src/
Get-ChildItem -Path src -Recurse -Include *.cpp,*.h | ForEach-Object {
    Write-Host "  Formatting: $($_.FullName)" -ForegroundColor Gray
    clang-format -i $_.FullName
}

Write-Host "✓ Code formatting complete!" -ForegroundColor Green
