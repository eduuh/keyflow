# Setup development tools (clang-format, clang-tidy)
# Run this once to set up your development environment

param(
    [switch]$Force
)

Write-Host "keyflow Development Tools Setup" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

# Check if scoop is installed
$scoopInstalled = Get-Command scoop -ErrorAction SilentlyContinue

if (-not $scoopInstalled) {
    Write-Host "❌ Scoop package manager not found" -ForegroundColor Red
    Write-Host ""
    Write-Host "Option 1: Install Scoop (recommended):" -ForegroundColor Yellow
    Write-Host "  Run in PowerShell:" -ForegroundColor Gray
    Write-Host '  Set-ExecutionPolicy RemoteSigned -Scope CurrentUser' -ForegroundColor White
    Write-Host '  irm get.scoop.sh | iex' -ForegroundColor White
    Write-Host ""
    Write-Host "Option 2: Manual installation:" -ForegroundColor Yellow
    Write-Host "  1. Download LLVM from: https://releases.llvm.org/" -ForegroundColor Gray
    Write-Host "  2. Install and add to PATH" -ForegroundColor Gray
    Write-Host "  3. Verify: clang-format --version" -ForegroundColor Gray
    Write-Host ""
    exit 1
}

Write-Host "✓ Scoop found" -ForegroundColor Green

# Check if llvm is installed
$llvmInstalled = Get-Command clang-format -ErrorAction SilentlyContinue

if (-not $llvmInstalled -or $Force) {
    Write-Host "Installing LLVM (includes clang-format, clang-tidy)..." -ForegroundColor Cyan
    scoop install llvm
} else {
    Write-Host "✓ LLVM already installed" -ForegroundColor Green
}

# Verify installations
Write-Host ""
Write-Host "Verifying installations:" -ForegroundColor Cyan

$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if ($clangFormat) {
    $version = (clang-format --version)
    Write-Host "  ✓ clang-format: $version" -ForegroundColor Green
} else {
    Write-Host "  ❌ clang-format not found" -ForegroundColor Red
}

$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if ($clangTidy) {
    $version = (clang-tidy --version | Select-Object -First 1)
    Write-Host "  ✓ clang-tidy: $version" -ForegroundColor Green
} else {
    Write-Host "  ❌ clang-tidy not found" -ForegroundColor Red
}

Write-Host ""
Write-Host "✓ Setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next step:" -ForegroundColor Cyan
Write-Host "  Install the pre-commit hook: .\scripts\setup-git-hooks.ps1" -ForegroundColor Gray
Write-Host "  (Formatting/linting runs automatically on every commit.)" -ForegroundColor Gray
