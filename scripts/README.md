# Scripts

Developer tooling — LLVM install and git-hook setup. Run from the repo root.

| Script | What it does |
|---|---|
| `setup-tools.ps1` | Installs LLVM (clang-format, clang-tidy) via scoop on Windows |
| `setup-git-hooks.ps1` | Installs the pre-commit hook that runs clang-format + clang-tidy on staged files |

Day-to-day formatting and linting are handled automatically by the pre-commit
hook and by CI. Whole-repo passes can be run on demand:

```powershell
# Format every C++ source file in src/
Get-ChildItem -Path src -Recurse -Include *.cpp,*.h | ForEach-Object { clang-format -i $_.FullName }

# Run clang-tidy on every .cpp in src/
Get-ChildItem -Path src -Recurse -Filter *.cpp | ForEach-Object { clang-tidy $_.FullName -- -Isrc -Isrc/external -std=c++17 }
```
