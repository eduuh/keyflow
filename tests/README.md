# Tests

Unit tests for keyflow processors, config loading and platform boundaries. Built
with Google Test (found from an installed package or fetched via CMake).

## Run

```powershell
.\build.ps1 -Tests
```

The script defaults to Release and runs CTest when `-Tests` is supplied.
For Debug, use `.\build.ps1 -Config Debug -Tests`. To run an existing Debug suite:

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

The config-coverage integration test drives `src/config.json` through the real
pipeline without hardware. It is engine evidence, not proof of injected OS output.
The designer fixture is a distinct witness with different expectations; preserve
both JSON files unchanged for Linux integration. Literal expected traces must
complement config-derived coverage rather than merely counting mappings.

Adding a new unit test: drop a `test_*.cpp` under the right `unit/` folder and
reconfigure CMake so its source glob picks it up.

## Linux and Portable Tests

Use direct CMake under `builds/`. For a Linux runtime plus nonprivileged tests:

```sh
cmake -S . -B builds/Linux-Debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build builds/Linux-Debug --target keyflow keyflow_tests --parallel 2
ctest --test-dir builds/Linux-Debug --output-on-failure --no-tests=error
builds/Linux-Debug/debug/bin/keyflow --validate "$PWD/src/config.json"
builds/Linux-Debug/debug/bin/keyflow --validate "$PWD/designer/public/eduuh-dh-config.json"
```

This needs pkg-config and libevdev development headers. See [BUILD.md](../BUILD.md)
for prerequisites and the independent Release gate. Validation captures no input.
To run portable tests without libevdev or a runtime target:

```sh
cmake -S . -B builds/Portable -DENABLE_LINUX_BACKEND=OFF -DENABLE_TESTS=ON
cmake --build builds/Portable --target keyflow_tests --parallel 2
ctest --test-dir builds/Portable --output-on-failure --no-tests=error
```

Backend-OFF success is not a Linux runtime build or device acceptance.
macOS remains tests-only. CI builds actual Windows and Ubuntu 24.04 runtimes
in separate Debug/Release entries; it does not run privileged device IO.

## Evidence and Device-test Gate

Report parser/validation, engine traces, kernel IO, and application-visible
behavior separately. A real executable and passing nonprivileged tests cannot
prove exclusive capture, consumer readiness, balanced kernel output, recovery,
glyphs, compositor repeat or desktop shortcuts. Linux remains **EXPERIMENTAL**
until the later live kernel/application verification is recorded.

Device tests are opt-in and outside hosted CI. Enabling the planned
`ENABLE_LINUX_DEVICE_TESTS` / `linux-device` gate requires separate authorization
for a disposable VM or dedicated seat, explicitly selected synthetic devices,
scoped access and an unaffected control channel. An explicitly enabled run must
fail when access is unavailable; a disabled gate means not executed, not passed.
Synthetic uinput devices can still be visible to desktop consumers, so a shared
workstation is not a safe substitute. Do not enumerate/grab physical keyboards,
change permissions or services, or log personal typing as part of ordinary tests.
