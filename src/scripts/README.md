# Development Scripts

Automation scripts for code formatting, linting, and git hooks.

## Quick Start

### 1. Install Tools (Windows)

```powershell
# Install LLVM (includes clang-format, clang-tidy)
.\scripts\setup-tools.ps1
```

**Alternative methods:**
```powershell
# Using scoop
scoop install llvm

# Using chocolatey
choco install llvm

# Or download from: https://releases.llvm.org/
```

### 2. Format Code

```powershell
# PowerShell (Windows)
.\scripts\format-code.ps1

# Bash (Git Bash / WSL / Linux)
./scripts/format-code.sh
```

### 3. Run Linter

```bash
./scripts/lint-code.sh
```

### 4. Setup Git Hooks

```powershell
.\scripts\setup-git-hooks.ps1
```

After this, code will be automatically formatted before each commit!

---

## Scripts Overview

| Script | Purpose | When to Use |
|--------|---------|-------------|
| `setup-tools.ps1` | Install clang-format & clang-tidy | Once, after cloning |
| `format-code.ps1` | Format all C++ code | Before committing |
| `format-code.sh` | Format all C++ code (bash) | Before committing |
| `lint-code.sh` | Run static analysis | Check for issues |
| `setup-git-hooks.ps1` | Install pre-commit hook | Once, after cloning |

---

## Pre-commit Hook

Once installed, the git hook will:
- ✓ Run automatically before `git commit`
- ✓ Format staged files with clang-format
- ✓ Run static analysis with clang-tidy
- ✓ Block commits with code quality issues
- ✓ Re-stage formatted files
- ✓ Show detailed check results

**Bypass hook** (not recommended):
```bash
git commit --no-verify
```

---

## Manual Formatting

Format a single file:
```bash
clang-format -i src/main.cpp
```

Check formatting without modifying:
```bash
clang-format --dry-run src/main.cpp
```

Format specific files:
```bash
clang-format -i src/main.cpp src/Config.h
```

---

## Linting

Run clang-tidy on single file:
```bash
clang-tidy src/main.cpp -- -Isrc -Iexternal -Isrc/platform
```

Fix issues automatically (use with caution):
```bash
clang-tidy src/main.cpp --fix-errors -- -Isrc -Iexternal
```

---

## Configuration Files

| File | Purpose |
|------|---------|
| `.clang-format` | Code formatting rules |
| `.clang-tidy` | Linting rules |
| `.editorconfig` | Editor settings |

---

## VS Code Integration

### Recommended Extensions

```json
{
  "recommendations": [
    "llvm-vs-code-extensions.vscode-clangd",
    "xaver.clang-format",
    "notskm.clang-tidy",
    "editorconfig.editorconfig"
  ]
}
```

### Settings (`.vscode/settings.json`)

```json
{
  "editor.formatOnSave": true,
  "editor.defaultFormatter": "xaver.clang-format",
  "C_Cpp.clang_format_style": "file",
  "clang-tidy.checks": ["*"]
}
```

---

## Troubleshooting

### "clang-format: command not found"

**Windows:**
1. Install LLVM: `scoop install llvm`
2. Restart terminal
3. Verify: `clang-format --version`

**Linux:**
```bash
sudo apt install clang-format clang-tidy
```

**macOS:**
```bash
brew install llvm
```

### "Permission denied" on scripts

**Git Bash / WSL:**
```bash
chmod +x scripts/*.sh
```

### Hook not running

```bash
# Check if hook exists
ls -la .git/hooks/pre-commit

# Re-install hook
.\scripts\setup-git-hooks.ps1 -Force
```

---

## CI/CD Integration

Add to GitHub Actions (`.github/workflows/format-check.yml`):

```yaml
name: Format Check

on: [push, pull_request]

jobs:
  format:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install clang-format
        run: sudo apt install clang-format
      - name: Check formatting
        run: |
          ./scripts/format-code.sh
          git diff --exit-code
```

---

## Best Practices

✅ **Do:**
- Format before every commit
- Run linter on new code
- Keep formatting config in git
- Use editor integration

❌ **Don't:**
- Commit unformatted code
- Bypass pre-commit hooks without reason
- Modify formatting config without team discussion
- Ignore linter warnings

---

## Resources

- [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
