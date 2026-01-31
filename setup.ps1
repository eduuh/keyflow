# keyflow Development Environment Setup
# Installs all required tools using Scoop package manager

param(
    [switch]$SkipGitHooks,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

Write-Info "================================================"
Write-Info "  keyflow Development Environment Setup"
Write-Info "================================================"
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning "Not running as Administrator."
    Write-Warning "Some features (like Interception driver) require Admin rights."
    Write-Host ""
}

#region Scoop Installation
Write-Info "[1/5] Checking Scoop package manager..."

$scoopInstalled = Get-Command scoop -ErrorAction SilentlyContinue
if (-not $scoopInstalled) {
    Write-Warning "Scoop not found. Installing..."

    try {
        # Set execution policy for current user
        Set-ExecutionPolicy RemoteSigned -Scope CurrentUser -Force

        # Install Scoop
        Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression

        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

        Write-Success "  ✓ Scoop installed successfully"
    } catch {
        Write-Error "  ✗ Failed to install Scoop: $_"
        Write-Host ""
        Write-Host "Manual installation:"
        Write-Host "  1. Open PowerShell as Admin"
        Write-Host "  2. Run: Set-ExecutionPolicy RemoteSigned -Scope CurrentUser"
        Write-Host "  3. Run: irm get.scoop.sh | iex"
        exit 1
    }
} else {
    $scoopVersion = scoop --version
    Write-Success "  ✓ Scoop already installed ($scoopVersion)"
}

# Update Scoop
Write-Info "  Updating Scoop..."
scoop update | Out-Null
Write-Host ""

#endregion

#region Git Installation
Write-Info "[2/5] Checking Git..."

$gitInstalled = Get-Command git -ErrorAction SilentlyContinue
if (-not $gitInstalled) {
    Write-Warning "  Git not found. Installing..."
    scoop install git

    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "  ✓ Git installed"
} else {
    $gitVersion = git --version
    Write-Success "  ✓ Git already installed ($gitVersion)"
}
Write-Host ""

#endregion

#region CMake Installation
Write-Info "[3/6] Checking CMake..."

$cmakeInstalled = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeInstalled) {
    Write-Warning "  CMake not found. Installing..."
    scoop install cmake

    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "  ✓ CMake installed"
} else {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Success "  ✓ CMake already installed ($cmakeVersion)"
}
Write-Host ""

#endregion

#region C++ Compiler Installation
Write-Info "[4/6] Checking C++ Compiler..."

# Check for various compilers
$msvcInstalled = Get-Command cl -ErrorAction SilentlyContinue
$gccInstalled = Get-Command g++ -ErrorAction SilentlyContinue
$clangInstalled = Get-Command clang++ -ErrorAction SilentlyContinue

if ($msvcInstalled) {
    Write-Success "  ✓ MSVC compiler found (Visual Studio)"
} elseif ($gccInstalled) {
    $gccVersion = g++ --version | Select-Object -First 1
    Write-Success "  ✓ GCC compiler found ($gccVersion)"
} elseif ($clangInstalled) {
    $clangVersion = clang++ --version | Select-Object -First 1
    Write-Success "  ✓ Clang compiler found ($clangVersion)"
} else {
    Write-Warning "  No C++ compiler found. Installing MinGW (GCC)..."
    Write-Info "  This may take a few minutes..."

    scoop install mingw

    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "  ✓ MinGW (GCC) installed"
}
Write-Host ""

#endregion

#region LLVM Installation (clang-format, clang-tidy)
Write-Info "[5/6] Checking LLVM (clang-format, clang-tidy)..."

$clangFormatInstalled = Get-Command clang-format -ErrorAction SilentlyContinue
$clangTidyInstalled = Get-Command clang-tidy -ErrorAction SilentlyContinue

if (-not $clangFormatInstalled -or -not $clangTidyInstalled) {
    Write-Warning "  LLVM tools not found. Installing..."
    Write-Info "  This may take a few minutes (LLVM is ~500MB)..."

    scoop install llvm

    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "  ✓ LLVM installed"
} else {
    $clangVersion = clang-format --version | Select-Object -First 1
    Write-Success "  ✓ LLVM already installed ($clangVersion)"
}
Write-Host ""

#endregion

#region Verification
Write-Info "[6/6] Verifying installations..."
Write-Host ""

$allGood = $true

# Check scoop
if (Get-Command scoop -ErrorAction SilentlyContinue) {
    Write-Success "  ✓ scoop: $(scoop --version)"
} else {
    Write-Error "  ✗ scoop: Not found"
    $allGood = $false
}

# Check git
if (Get-Command git -ErrorAction SilentlyContinue) {
    $gitVer = git --version
    Write-Success "  ✓ git: $gitVer"
} else {
    Write-Error "  ✗ git: Not found"
    $allGood = $false
}

# Check cmake
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $cmakeVer = cmake --version | Select-Object -First 1
    Write-Success "  ✓ cmake: $cmakeVer"
} else {
    Write-Error "  ✗ cmake: Not found"
    $allGood = $false
}

# Check C++ compiler
if (Get-Command cl -ErrorAction SilentlyContinue) {
    Write-Success "  ✓ compiler: MSVC (Visual Studio)"
} elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
    $gccVer = g++ --version | Select-Object -First 1
    Write-Success "  ✓ compiler: $gccVer"
} elseif (Get-Command clang++ -ErrorAction SilentlyContinue) {
    $clangVer = clang++ --version | Select-Object -First 1
    Write-Success "  ✓ compiler: $clangVer"
} else {
    Write-Error "  ✗ compiler: No C++ compiler found"
    $allGood = $false
}

# Check clang-format
if (Get-Command clang-format -ErrorAction SilentlyContinue) {
    $clangFormatVer = clang-format --version
    Write-Success "  ✓ clang-format: $clangFormatVer"
} else {
    Write-Error "  ✗ clang-format: Not found"
    $allGood = $false
}

# Check clang-tidy
if (Get-Command clang-tidy -ErrorAction SilentlyContinue) {
    $clangTidyVer = clang-tidy --version | Select-Object -First 1
    Write-Success "  ✓ clang-tidy: $clangTidyVer"
} else {
    Write-Error "  ✗ clang-tidy: Not found"
    $allGood = $false
}

Write-Host ""

#endregion

#region Git Hooks Setup
if (-not $SkipGitHooks) {
    Write-Info "Setting up git pre-commit hooks..."

    if (Test-Path ".\src\scripts\setup-git-hooks.ps1") {
        try {
            & ".\src\scripts\setup-git-hooks.ps1"
            Write-Success "  ✓ Git hooks configured"
        } catch {
            Write-Warning "  ⚠ Failed to setup git hooks: $_"
            Write-Warning "    You can set them up manually later:"
            Write-Warning "    .\src\scripts\setup-git-hooks.ps1"
        }
    } else {
        Write-Warning "  ⚠ Git hooks script not found"
    }
    Write-Host ""
}
#endregion

#region Summary
Write-Host ""
Write-Info "================================================"
Write-Info "  Setup Complete!"
Write-Info "================================================"
Write-Host ""

if ($allGood) {
    Write-Success "✓ All tools installed successfully!"
    Write-Host ""
    Write-Info "Next steps:"
    Write-Host "  1. Build the project:"
    Write-Host "     cmake -B build"
    Write-Host "     cmake --build build --config Release"
    Write-Host ""
    Write-Host "  2. Format code:"
    Write-Host "     .\src\scripts\format-code.ps1"
    Write-Host ""
    Write-Host "  3. Run keyflow (requires Admin + Interception driver):"
    Write-Host "     cd build\bin\Release"
    Write-Host "     .\keyflow.exe"
    Write-Host ""
    Write-Info "Documentation:"
    Write-Host "  - Quick start: QUICK_START.md"
    Write-Host "  - Development: src\docs\DEVELOPMENT.md"
    Write-Host "  - Configuration: src\docs\CONFIG_USAGE.md"
} else {
    Write-Error "✗ Some tools failed to install"
    Write-Host ""
    Write-Host "Please check the errors above and:"
    Write-Host "  1. Make sure you have internet connection"
    Write-Host "  2. Run PowerShell as Administrator"
    Write-Host "  3. Try running this script again"
    Write-Host ""
    Write-Host "Or install tools manually:"
    Write-Host "  scoop install git cmake llvm"
}

Write-Host ""

# Open documentation if requested
if ($Verbose) {
    Write-Info "Opening documentation..."
    Start-Process "QUICK_START.md"
}

#endregion
