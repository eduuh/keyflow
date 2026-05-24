# Keyflow Build Script
# Usage: .\build.ps1 [-Config Debug|Release] [-Tests] [-RunTests] [-Clean]
#
#   -Config <type>  Debug or Release (default: Release)
#   -Tests          Configure + build with tests enabled
#   -RunTests       Run the existing test binary without reconfiguring/rebuilding
#                   (requires a prior `-Tests` build)
#   -Clean          Wipe the build directory before configuring

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$Tests,
    [switch]$RunTests,
    [switch]$Clean
)

$BuildDir = "builds\$Config"

# Fast path: just run tests, skip configure/build entirely.
if ($RunTests) {
    $TestExe = "$BuildDir\tests\$Config\keyflow_tests.exe"
    if (-not (Test-Path $TestExe)) {
        Write-Host "Test binary not found: $TestExe" -ForegroundColor Red
        Write-Host "Run './build.ps1 -Tests' first to build it." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "Running tests..." -ForegroundColor Cyan
    & $TestExe @args
    exit $LASTEXITCODE
}

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
