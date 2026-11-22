# Contributing to Keyflow

## Config and Platform Changes

`src/config.json` is the shipped runtime fixture.
`designer/public/eduuh-dh-config.json` is a separate designer fixture with different
LeftWin/layer behavior. They are not interchangeable. Platform integration must
preserve each file byte-for-byte and validate both; do not copy one over the other.

For intentional config changes, test the affected runtime behavior and update the
designer where its schema/editor contract requires it, preserving deliberate
fixture differences. Document those changes together. Keep canonical Windows
`SC_*` identities: Linux device paths, permissions and native keycodes do not
belong in layout JSON. Physical mappings do not promise identical glyphs or
workspace bindings on every OS keymap/desktop.

## Build System

All builds go to `builds/` directory:
- On Windows, use `.\build.ps1` for automated builds
- On Linux, use direct CMake with pkg-config/libevdev for the experimental runtime
- Use explicit `CMAKE_SYSTEM_NAME == Linux` guards, not a generic non-Windows branch
- `ENABLE_LINUX_BACKEND=OFF` supports portable tests without libevdev or a runtime target
- Preserve C++17, warnings-as-errors and the Windows resource/Interception build behavior
- See [BUILD.md](BUILD.md) for detailed instructions

## Code Style

- C++17 standard
- Pre-commit hooks run `clang-format` and `clang-tidy`
- Keep code simple and focused

## Testing

Run tests with:
```powershell
.\build.ps1 -Tests
```

This defaults to Release; use `.\build.ps1 -Config Debug -Tests` for Debug.
Linux commands and prerequisites are in [BUILD.md](BUILD.md); test boundaries are
in [tests/README.md](tests/README.md).

CI requires separate Windows and Ubuntu 24.04 Debug/Release builds of `keyflow`
and `keyflow_tests`, CTest, and non-capturing validation of both fixtures. Keep
Linux device tests out of hosted CI. Parser/engine tests, native kernel IO, and
application-visible acceptance must be reported independently; portable tests
are not runtime proof. Keep Linux support **EXPERIMENTAL** until the later live
kernel/application gate is satisfied.

Live IO requires separate authorization for an isolated environment, explicitly
selected devices and scoped access, with an unaffected stop channel. Do not
change permissions, services or desktop bindings as part of a build/test run,
or suggest broad input-group/world-writable access.

## Git Workflow

1. Work on feature branches
2. Commit with descriptive messages
3. Ensure tests pass
4. Push to remote

## Architecture Notes

- **Single-instance enforcement**: Only one Keyflow process can run
- **Zero-latency design**: No artificial delays in event processing
- **Pipeline architecture**: Events flow through processor chain
