# Scripts

Developer tooling — LLVM install and git-hook setup. Run from the repo root.

| Script | What it does |
|---|---|
| `setup-tools.ps1` | Installs LLVM (clang-format, clang-tidy) via scoop on Windows |
| `setup-git-hooks.ps1` | Installs the pre-commit hook that runs clang-format + clang-tidy on staged files |
| `linux/install.py` | Opt-in fresh Linux systemd installation with an explicit keyboard and scoped access; no automatic capture |
| `linux/rollback.py` | Installed recovery helper for raw input or an explicitly archived Kanata user service |

See the [Linux service guide](../docs/LINUX_SERVICE.md) before using deployment
tools. They are separate from developer setup and ordinary test runs.

Day-to-day formatting and linting are handled automatically by the pre-commit
hook and by CI. Whole-repo passes can be run on demand:

```powershell
# Format every C++ source file in src/
Get-ChildItem -Path src -Recurse -Include *.cpp,*.h | ForEach-Object { clang-format -i $_.FullName }

# Run clang-tidy on every .cpp in src/
Get-ChildItem -Path src -Recurse -Filter *.cpp | ForEach-Object { clang-tidy $_.FullName -- -Isrc -Isrc/external -std=c++17 }
```
