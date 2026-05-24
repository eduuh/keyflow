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
├── config/        # ConfigLoader, KeyNameMapper (targeted invariants only)
├── pipeline/      # Modifiers parsing, Pipeline boundary
├── processors/    # ModifierTracker, ComboAdvanced, CapsLockBlocker,
│                  # LayerTriggerBlocker, bug_fixes, phantom_caps_fix
└── integration/   # test_config_coverage.cpp — drives every behavior in
                   # src/config.json through the real pipeline (88 mappings)
tests/fixtures/    # MockProcessor, PipelineHarness
```

The integration test treats src/config.json as the spec — edit the config, the
test surface updates automatically. Adding a new unit test: drop a `test_*.cpp`
under the right `unit/` folder; CMake picks it up automatically.
