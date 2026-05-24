# Setup git pre-commit hook to automatically format code

Write-Host "Setting up git pre-commit hook..." -ForegroundColor Cyan

$hookPath = ".git/hooks/pre-commit"

# Check if git repo
if (-not (Test-Path ".git")) {
    Write-Host "Error: Not a git repository" -ForegroundColor Red
    exit 1
}

# Create hooks directory if it does not exist
$hooksDir = ".git/hooks"
if (-not (Test-Path $hooksDir)) {
    New-Item -ItemType Directory -Path $hooksDir | Out-Null
}

# Pre-commit hook content
$hookContent = @'
#!/bin/bash
# Git pre-commit hook: Format and lint staged C++ files

echo "Running pre-commit checks..."

# Get list of staged C++ files
STAGED_CPP_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h)$')

if [ -z "$STAGED_CPP_FILES" ]; then
    echo "No C++ files to check"
    exit 0
fi

# Flag to track if any checks failed
CHECKS_FAILED=0

# ============================================================================
# 1. clang-format: Code Formatting
# ============================================================================
if command -v clang-format &> /dev/null; then
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  [1/2] Formatting Code (clang-format)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    FORMAT_COUNT=0
    echo "$STAGED_CPP_FILES" | while read file; do
        if [ -f "$file" ]; then
            echo "  ✓ Formatting: $file"
            clang-format -i "$file"
            git add "$file"
            FORMAT_COUNT=$((FORMAT_COUNT + 1))
        fi
    done

    echo "  ✓ Formatted files"
else
    echo "⚠ Warning: clang-format not found, skipping formatting"
    echo "  Install: scoop install llvm (Windows) or apt install clang-format (Linux)"
fi

# ============================================================================
# 2. clang-tidy: Static Analysis
# ============================================================================
if command -v clang-tidy &> /dev/null; then
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  [2/2] Running Static Analysis (clang-tidy)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    TIDY_FAILED=0

    # Only analyze .cpp files (not headers) to avoid duplicate checks
    STAGED_CPP_ONLY=$(echo "$STAGED_CPP_FILES" | grep '\.cpp$')

    if [ ! -z "$STAGED_CPP_ONLY" ]; then
        echo "$STAGED_CPP_ONLY" | while read file; do
            if [ -f "$file" ]; then
                echo "  → Analyzing: $file"

                # Run clang-tidy and capture output
                TIDY_OUTPUT=$(clang-tidy "$file" -- -Isrc -Isrc/external -Isrc/platform -std=c++17 2>&1)
                TIDY_RESULT=$?

                # Check for errors or warnings
                if echo "$TIDY_OUTPUT" | grep -qE "(error:|warning:)"; then
                    echo "    ✗ Issues found:"
                    echo "$TIDY_OUTPUT" | grep -E "(error:|warning:)" | head -5
                    TIDY_FAILED=1
                else
                    echo "    ✓ No issues"
                fi
            fi
        done

        if [ $TIDY_FAILED -ne 0 ]; then
            CHECKS_FAILED=1
            echo ""
            echo "⚠ clang-tidy found issues in your code"
            echo "  Run './src/scripts/lint-code.sh' to see all issues"
            echo "  You can bypass this check with: git commit --no-verify"
        fi
    else
        echo "  ℹ Only header files staged, skipping analysis"
    fi
else
    echo ""
    echo "⚠ Warning: clang-tidy not found, skipping static analysis"
    echo "  Install: scoop install llvm (Windows) or apt install clang-tidy (Linux)"
fi

# ============================================================================
# Summary
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ $CHECKS_FAILED -ne 0 ]; then
    echo "  ✗ Pre-commit checks FAILED"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    exit 1
else
    echo "  ✓ All pre-commit checks PASSED"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    exit 0
fi
'@

# Write hook file
Set-Content -Path $hookPath -Value $hookContent -NoNewline

# Make executable on Unix-like systems
if ($IsLinux -or $IsMacOS) {
    chmod +x $hookPath
}

# For Windows create a wrapper bat file
if ($IsWindows -or $env:OS -match "Windows") {
    $hookBatPath = ".git/hooks/pre-commit.bat"
    $batContent = '@echo off' + "`r`n" + 'bash.exe "%~dp0pre-commit" %*'
    Set-Content -Path $hookBatPath -Value $batContent
}

Write-Host "Git pre-commit hook installed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "The hook will:" -ForegroundColor Cyan
Write-Host "  - Run automatically before each commit" -ForegroundColor Gray
Write-Host "  - Format staged C++ files with clang-format" -ForegroundColor Gray
Write-Host "  - Run static analysis with clang-tidy" -ForegroundColor Gray
Write-Host "  - Block commits with code quality issues" -ForegroundColor Gray
Write-Host "  - Re-stage formatted files" -ForegroundColor Gray
Write-Host ""
Write-Host "To bypass the hook (not recommended):" -ForegroundColor Yellow
Write-Host "  git commit --no-verify" -ForegroundColor White
