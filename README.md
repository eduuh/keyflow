# keyflow

**Privacy-first keyboard remapper for Windows**

Zero data collection • Driver-level interception • JSON configuration

---

## Quick Start

```bash
# 1. Install Interception driver (one-time)
# Download: https://github.com/oblitum/Interception
# Reboot after install

# 2. Create config.json
cp examples\simple.json config.json

# 3. Run as administrator
.\keyflow.exe
```

## Features

- **1:1 key remapping** - CapsLock → Ctrl
- **Layer system** - Vim navigation, symbols, numpad
- **JSON config** - Simple, human-readable
- **<2ms latency** - Driver-level capture
- **Zero logging** - No data collection, no network

## Configuration

```json
{
  "version": "1.0",
  "remapping": {
    "CapsLock": "LeftCtrl"
  },
  "layers": [
    {
      "name": "Vim Nav",
      "trigger": ["RightAlt"],
      "mappings": {
        "H": "LeftArrow",
        "J": "DownArrow",
        "K": "UpArrow",
        "L": "RightArrow"
      }
    }
  ]
}
```

See [JSON_CONFIG.md](JSON_CONFIG.md) for complete reference.

## Runtime Controls

| Key | Action |
|-----|--------|
| F12 | Toggle on/off |
| F11 | Toggle logging |
| F10 | Toggle debug |

## Build

**Requirements:** CMake 3.15+, MinGW-w64 (or any C++17 compiler)

```bash
# Debug build (includes logging with cout)
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build (no logging)
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Install MinGW-w64: https://www.msys2.org/

## Testing

**Unit Tests:** 152/152 (100%) ✅

KeyFlow includes comprehensive unit tests using Google Test:

```bash
# Build with tests enabled (automatic in Debug builds)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build --target keyflow_tests

# Run all tests
cd build
ctest --output-on-failure -C Debug

# Run specific test suite
./tests/Debug/keyflow_tests.exe --gtest_filter="ModifiersTest.*"
```

**Test Coverage:**
- ✅ Pipeline components (modifiers, context, orchestration)
- ✅ Processors (rewire, modifier tracking, combos)
- ✅ Hardware layer (KeyEvent struct)
- ✅ Config system (parsing, validation, key mapping)

See [tests/README.md](tests/README.md) for detailed testing documentation.

## Requirements

- Windows 10/11
- Administrator privileges
- Interception driver

## Privacy

- ✅ No data collection
- ✅ No network calls
- ✅ No disk logging
- ✅ Open source

---

## Documentation

- **[Installation Guide](docs/INSTALLATION.md)** - Setup instructions
- **[Architecture](docs/ARCHITECTURE.md)** - Technical details
- **[Privacy Policy](docs/PRIVACY.md)** - Privacy guarantees
- **[Troubleshooting](docs/TROUBLESHOOTING.md)** - Common issues
- **[Configuration Reference](JSON_CONFIG.md)** - Complete config guide

---

## License

MIT License - See LICENSE file

---

**keyflow** - Take control of your keyboard, keep control of your privacy.
