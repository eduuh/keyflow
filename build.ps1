# Keyflow Build Script
# Usage: .\build.ps1 [-Config Debug|Release] [-Tests]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$Tests,
    [switch]$Clean
)

$BuildDir = "builds\$Config"

Write-Host "=== Keyflow Build Script ===" -ForegroundColor Cyan
Write-Host "Build directory: $BuildDir" -ForegroundColor Green
Write-Host "Configuration: $Config" -ForegroundColor Green

# Clean build if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure CMake
Write-Host "`nConfiguring CMake..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    $TestsFlag = if ($Tests) { "-DENABLE_TESTS=ON" } else { "-DENABLE_TESTS=OFF" }

    cmake ..\.. -DCMAKE_BUILD_TYPE=$Config $TestsFlag
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }

    # Build
    Write-Host "`nBuilding..." -ForegroundColor Yellow
    cmake --build . --config $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }

    # Run tests if enabled
    if ($Tests) {
        Write-Host "`nRunning tests..." -ForegroundColor Yellow
        ctest -C $Config --output-on-failure
    }

    Write-Host "`n=== Build Complete ===" -ForegroundColor Green
    Write-Host "Executable: $BuildDir\$Config\bin\$Config\keyflow.exe" -ForegroundColor Green
} finally {
    Pop-Location
}
