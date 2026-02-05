# Build Instructions

All builds are organized under the `builds/` directory.

## Quick Build

### Using PowerShell Script (Recommended)

```powershell
# Release build (default)
.\build.ps1

# Debug build
.\build.ps1 -Config Debug

# Release build with tests
.\build.ps1 -Tests

# Clean and rebuild
.\build.ps1 -Clean
```

### Manual CMake

```powershell
# Release build
mkdir -p builds\Release
cd builds\Release
cmake ..\.. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Debug build
mkdir -p builds\Debug
cd builds\Debug
cmake ..\.. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

# With tests
cmake ..\.. -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build . --config Release
ctest -C Release
```

## Build Output

Executables are placed in:
- **Release**: `builds\Release\Release\bin\Release\keyflow.exe`
- **Debug**: `builds\Debug\Debug\bin\Debug\keyflow.exe`

## Directory Structure

```
keyflow/
├── builds/           # All build outputs (gitignored)
│   ├── Debug/       # Debug builds
│   └── Release/     # Release builds
├── src/             # Source code
├── tests/           # Test files
└── build.ps1        # Build script
```

## Important Notes

- **Do NOT use `build/` directory** - everything goes in `builds/`
- Both `build/` and `builds/` are in `.gitignore`
- The build script ensures consistent build locations

## Config Changes

⚠️ **IMPORTANT**: When modifying `src/config.json`, you MUST also update the designer codebase!

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.
