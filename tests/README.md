# Tests

Unit tests for keyflow processors and config loading. Built with Google Test (auto-fetched via CMake).

## Run

```powershell
.\build.ps1 -Tests
```

Tests run as part of the Debug build. To run individually:

```powershell
cd builds\Debug
ctest -C Debug --output-on-failure
```

## Layout

```
tests/unit/
├── config/        # ConfigLoader, JsonConfig, KeyNameMapper
├── pipeline/      # Context, Modifiers, Pipeline
└── processors/    # Rewire, ModifierTracker, ComboAdvanced, StrictModeFilter,
                  # CapsLockBlocker, LayerTriggerBlocker, bug regressions
tests/fixtures/    # MockProcessor for processor tests
```

Tests use inline JSON strings rather than file fixtures. Adding a new test: drop a `test_*.cpp` under the right `unit/` folder; CMake picks it up automatically.
