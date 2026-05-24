# Scripts

Developer tooling — formatting, linting, git hook setup.

| Script | What it does |
|---|---|
| `setup-tools.ps1` | Installs LLVM (clang-format, clang-tidy) on Windows |
| `setup-git-hooks.ps1` / `.sh` | Installs the pre-commit hook that runs clang-format on staged files |
| `format-code.ps1` / `.sh` | Format all C++ sources with clang-format |
| `lint-code.sh` | Run clang-tidy across the codebase |

Run scripts from the repo root.
