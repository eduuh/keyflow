# Development Guide

Complete setup guide for keyflow development.

## Quick Start

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/keyflow.git
cd keyflow
```

### 2. Install Development Tools

**Windows (PowerShell):**
```powershell
# Install LLVM (clang-format, clang-tidy)
.\scripts\setup-tools.ps1

# Setup git hooks
.\scripts\setup-git-hooks.ps1
```

**Linux:**
```bash
sudo apt install clang-format clang-tidy cmake build-essential

# Setup git hooks
./scripts/setup-git-hooks.sh
```

**macOS:**
```bash
brew install llvm cmake

# Setup git hooks
./scripts/setup-git-hooks.sh
```

### 3. Build Project

```bash
cmake -B build
cmake --build build --config Release
```

### 4. Run

```bash
cd build/bin/Release
./keyflow.exe  # Requires Administrator + Interception driver
```

---

## Project Structure

```
keyflow/
├── .clang-format          # Code formatting rules
├── .clang-tidy            # Linting rules
├── .editorconfig          # Editor settings
├── config.json            # Default configuration
├── CMakeLists.txt         # Build system
│
├── docs/                  # Documentation
│   ├── ARCHITECTURE.md
│   ├── CODING_STANDARDS.md
│   ├── CONFIG_USAGE.md
│   ├── DEVELOPMENT.md     # This file
│   ├── INSTALLATION.md
│   ├── PRIVACY.md
│   ├── REFACTORING.md
│   └── TROUBLESHOOTING.md
│
├── src/                   # Source code
│   ├── main.cpp           # Application entry point
│   ├── Config.h           # Runtime configuration
│   ├── hardware/          # Hardware abstraction (Interception)
│   ├── pipeline/          # Processing pipeline
│   ├── processors/        # Key processors (Rewire, Combo, etc)
│   └── config/            # Config loading system
│
├── external/              # Third-party libraries
│   └── nlohmann/json.hpp  # JSON parser
│
├── scripts/               # Development scripts
│   ├── format-code.ps1    # Format C++ code
│   ├── lint-code.sh       # Run linter
│   ├── setup-tools.ps1    # Install dev tools
│   └── setup-git-hooks.ps1
│
├── build/                 # Build output (gitignored)
└── archive/               # Archived files
```

---

## Development Workflow

### 1. Make Changes

Edit source files in `src/` or config files.

### 2. Format Code

**Automatic** (on commit with git hook):
```bash
git add .
git commit -m "Your message"
# Hook runs automatically, formats staged files
```

**Manual** (before commit):
```powershell
.\scripts\format-code.ps1
```

### 3. Check for Issues

```bash
./scripts/lint-code.sh
```

### 4. Build and Test

```bash
cmake --build build --config Release
cd build/bin/Release
./keyflow.exe --debug
```

### 5. Commit

```bash
git add .
git commit -m "feat: add new feature"
git push
```

---

## Code Quality

### Formatting

Uses **clang-format** with custom `.clang-format` config.

**Format all files:**
```powershell
.\scripts\format-code.ps1
```

**Format specific file:**
```bash
clang-format -i src/main.cpp
```

**Check without modifying:**
```bash
clang-format --dry-run src/main.cpp
```

### Linting

Uses **clang-tidy** with custom `.clang-tidy` config.

**Lint all files:**
```bash
./scripts/lint-code.sh
```

**Lint specific file:**
```bash
clang-tidy src/main.cpp -- -Isrc -Iexternal -Isrc/platform
```

**Auto-fix issues:**
```bash
clang-tidy src/main.cpp --fix-errors -- -Isrc -Iexternal
```

### Pre-commit Hook

Automatically formats staged C++ files before commit.

**Install:**
```powershell
.\scripts\setup-git-hooks.ps1
```

**Bypass** (not recommended):
```bash
git commit --no-verify
```

---

## Editor Setup

### VS Code

**Recommended Extensions:**
```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "llvm-vs-code-extensions.vscode-clangd",
    "xaver.clang-format",
    "notskm.clang-tidy",
    "editorconfig.editorconfig",
    "cschlosser.doxdocgen"
  ]
}
```

**Settings** (`.vscode/settings.json`):
```json
{
  "editor.formatOnSave": true,
  "editor.rulers": [100],
  "files.insertFinalNewline": true,

  "[cpp]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.tabSize": 4
  },

  "C_Cpp.clang_format_style": "file"
}
```

### Visual Studio

1. Tools → Options → Text Editor → C/C++ → Formatting
2. Enable "Format on paste" and "Format on save"
3. Use .clang-format file

### CLion

1. Settings → Editor → Code Style → C/C++
2. Set scheme to "ClangFormat"
3. Enable "Enable ClangFormat"

---

## Build System

### CMake Configuration

```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release build (default)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# With specific compiler
cmake -B build -DCMAKE_CXX_COMPILER=clang++
```

### Build Targets

```bash
# Build all
cmake --build build --config Release

# Clean build
cmake --build build --clean-first

# Verbose build
cmake --build build --verbose
```

### Output Location

```
build/
├── bin/Release/
│   ├── keyflow.exe        # Executable
│   ├── config.json        # Runtime config
│   └── interception.dll   # Driver
└── ...
```

---

## Testing

### Manual Testing

1. Build project
2. Run as Administrator
3. Test key mappings
4. Check logs with `--debug` flag

```bash
cd build/bin/Release
./keyflow.exe --debug
```

### Runtime Controls

- **F10**: Toggle debug mode
- **F11**: Toggle keystroke logging
- **F12**: Toggle keyflow on/off
- **Ctrl+C**: Exit

### Configuration Testing

Test different layouts:
```bash
keyflow.exe config.json        # Default
keyflow.exe colemak.json       # Colemak
keyflow.exe test-config.json   # Test config
```

---

## Adding New Features

### 1. Plan

Document your approach:
- What problem does it solve?
- How will it work?
- What files need changes?

### 2. Implement

Follow coding standards:
- See `docs/CODING_STANDARDS.md`
- Use clang-format
- Write clear, concise code

### 3. Test

- Build and run
- Test edge cases
- Check with `--debug` flag

### 4. Document

Update relevant docs:
- `README.md` for user-facing features
- Code comments for complex logic
- `docs/` for architecture changes

### 5. Commit

```bash
git add .
git commit -m "feat: add feature name"
```

**Commit Message Format:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation
- `refactor:` Code refactoring
- `test:` Tests
- `chore:` Maintenance

---

## Common Tasks

### Add New Processor

1. Create `src/processors/MyProcessor.h`
2. Inherit from `IProcessor`
3. Implement `process()` method
4. Add to pipeline in `ConfigBuilder`

**Example:**
```cpp
class MyProcessor : public IProcessor {
public:
    bool process(Context& ctx) override {
        // Modify ctx.outputScancode, ctx.action, etc.
        return true;  // Continue pipeline
    }

    const char* name() const noexcept override {
        return "MyProcessor";
    }
};
```

### Add Config Option

1. Update `src/config/JsonConfig.h`
2. Update `src/config/ConfigLoader.h` parsing
3. Update `src/config/ConfigBuilder.h` to use it
4. Update `config.schema.json`

### Add Key Scancode

1. Update `src/hardware/Scancodes.h`
2. Update `src/config/KeyNameMapper.h`

---

## Debugging

### Build Issues

```bash
# Clean rebuild
rm -rf build
cmake -B build
cmake --build build --config Release
```

### Runtime Issues

```bash
# Enable all logging
keyflow.exe --verbose

# Check specific behavior
keyflow.exe --debug
```

### Interception Driver

```bash
# Verify driver installed
# Should see "Interception" in Device Manager → System Devices

# Test with sample app from:
# http://www.oblita.com/interception
```

---

## Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
- [CMake](https://cmake.org/documentation/)
- [Interception Driver](http://www.oblita.com/interception)

---

## Getting Help

- Check `docs/TROUBLESHOOTING.md`
- Open an issue on GitHub
- Read architecture docs in `docs/ARCHITECTURE.md`

---

**Happy coding! 🌶️**
