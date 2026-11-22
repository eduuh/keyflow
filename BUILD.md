# Build Instructions

All builds are organized under the `builds/` directory.

## Quick Build

### Windows: PowerShell Script (Recommended)

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

### Windows: Manual CMake

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

### Linux: Direct CMake (Experimental)

The initial build/CI target is Linux x86_64 on Ubuntu 24.04. Other distributions
are not a verified support matrix. Use a C++17 compiler and build tool, CMake
(3.20+ for the CTest commands below), pkg-config and libevdev development headers.
Ubuntu package names are `build-essential`, `cmake`, `pkg-config`, and
`libevdev-dev`. No libudev or desktop framework is required. Google Test is
fetched when tests are enabled unless a suitable installed GTest package is found.

Run from the repository root:

```sh
cmake -S . -B builds/Linux-Debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build builds/Linux-Debug --target keyflow keyflow_tests --parallel 2
test -x builds/Linux-Debug/debug/bin/keyflow
cmp src/config.json builds/Linux-Debug/debug/bin/config.json
builds/Linux-Debug/debug/bin/keyflow --validate "$PWD/src/config.json"
builds/Linux-Debug/debug/bin/keyflow --validate "$PWD/designer/public/eduuh-dh-config.json"
ctest --test-dir builds/Linux-Debug --output-on-failure --no-tests=error

cmake -S . -B builds/Linux-Release -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build builds/Linux-Release --target keyflow keyflow_tests --parallel 2
test -x builds/Linux-Release/release/bin/keyflow
cmp src/config.json builds/Linux-Release/release/bin/config.json
builds/Linux-Release/release/bin/keyflow --validate "$PWD/src/config.json"
builds/Linux-Release/release/bin/keyflow --validate "$PWD/designer/public/eduuh-dh-config.json"
ctest --test-dir builds/Linux-Release --output-on-failure --no-tests=error
```

`ENABLE_LINUX_BACKEND` defaults ON only when `CMAKE_SYSTEM_NAME` is `Linux`.
Missing pkg-config/libevdev is a configure error for the runtime. To build only
portable tests without libevdev:

```sh
cmake -S . -B builds/Portable -DENABLE_LINUX_BACKEND=OFF -DENABLE_TESTS=ON
cmake --build builds/Portable --target keyflow_tests --parallel 2
ctest --test-dir builds/Portable --output-on-failure --no-tests=error
```

This OFF configuration has **no `keyflow` runtime target** and cannot satisfy Linux
runtime acceptance. macOS and other platforms without a backend remain tests-only.

## Build Output

The runtime output directory is lower-case `debug/bin` or `release/bin`.
Visual Studio appends the configuration directory:

- **Windows Release**: `builds\Release\release\bin\Release\keyflow.exe`
- **Windows Debug**: `builds\Debug\debug\bin\Debug\keyflow.exe`
- **Linux Release**: `builds/Linux-Release/release/bin/keyflow`
- **Linux Debug**: `builds/Linux-Debug/debug/bin/keyflow`

The source config is copied unchanged next to the executable. Windows also copies
Interception and its icon; Linux has no Interception DLL dependency. Linux links
the system libevdev library, so its shared library must also be available when
running a downloaded native artifact.

## Linux Operation and Safety

**Runtime support is EXPERIMENTAL until live kernel and application verification
is recorded.** Compilation, CTest and non-capturing `--validate` are separate
evidence from evdev capture/uinput output and desktop-visible text or shortcuts.
Hosted CI does not access devices or run privileged kernel tests.

The runtime runs in the foreground, with no built-in tray or autostart.
An opt-in [systemd installation guide](docs/LINUX_SERVICE.md) covers the checked-in
service/install/rollback files, scoped access and guarded Kanata migration.
It is a separately authorized system change, not part of a normal build or test.
It preserves the launch working directory; use an absolute config path.
`--help` and `--validate file.json` exit without selecting or capturing keyboards.
For runtime, repeat `--device` for every explicitly selected input. Prefer stable
by-id keyboard paths; do not select arbitrary event nodes or the virtual output.

Before any activation, obtain explicit authorization for a scoped access procedure
covering only the selected input devices and `/dev/uinput`. Keyflow does not elevate
itself or provision permissions. Do not prescribe input-group membership,
world-writable nodes, or persistent system changes as installation defaults.
Denied device access is a blocker, not a reason to broaden access silently.
The per-user singleton also requires a valid private, user-owned
`XDG_RUNTIME_DIR`; do not substitute a shared `/tmp` directory.

After that gate, replace the example paths with verified selected keyboards:

```sh
builds/Linux-Debug/debug/bin/keyflow \
  --device /dev/input/by-id/your-keyboard-event-kbd \
  --device /dev/input/by-id/your-second-keyboard-event-kbd \
  "$PWD/src/config.json"
```

Keep all selected keys released during startup and retain an unaffected control
channel. Physical `Ctrl+Escape` (either Ctrl) is the emergency stop before remapping.
Foreground `Ctrl+C` also requests orderly shutdown. From an unaffected terminal,
identify the process with `ps -eo pid,args`, verify its executable and arguments,
then send `kill -TERM <verified-keyflow-pid>` to that specific PID. Do not use
name-based killing. SIGKILL cannot run orderly cleanup and is not the normal stop.

Orderly stop releases owned outputs and device grabs and destroys the virtual
keyboard. Confirm ordinary input returns; failure to recover is a failed live
acceptance check, not evidence of compatibility. Rollback is stopping the manually
launched process and ceasing to run it: Keyflow changes no services, autostart,
desktop bindings or permissions. Any separately authorized access setup needs its
own scoped rollback, outside this build procedure.

Configs describe physical key mappings. Glyphs, NumLock/AltGr behavior, compositor
repeat and workspace/desktop shortcuts depend on the declared environment.
`src/config.json` and `designer/public/eduuh-dh-config.json` are distinct witnesses;
validate both without synchronizing or substituting their contents.

## CI and Artifacts

CI has distinct Windows Debug/Release and Ubuntu 24.04 Debug/Release entries with
tests ON. Each builds the actual runtime and tests, runs CTest, checks the copied
config and validates both fixtures without input capture. These gates do not prove
kernel acceptance or application-visible behavior.

After all four entries pass, the main-only Linux workflow artifact contains
`keyflow`, the unchanged config and licensing/build documentation.

The release workflow resolves one existing tag to a commit, builds and tests
Windows and Ubuntu 24.04 in Debug and Release, then publishes both platform
archives from that commit through one publisher:

- **KeyFlow Windows**: `keyflow-release.zip` and its SHA256 file. The existing
  Windows asset name is retained; it includes `keyflow.exe`, `interception.dll`,
  the unchanged config and licensing documentation.
- **KeyFlow Linux (experimental)**: `keyflow-linux-x86_64.tar.gz` and its SHA256
  file. It includes the native binary, unchanged config, documentation and the
  optional `scripts/linux/` installation files.

Both archives record `SOURCE_COMMIT`. The Linux workflow binary is built on
Ubuntu 24.04 and links system libraries; check the stated runtime requirements.
Earlier manually published Arch assets retain their explicit distribution label.
Neither archive installs a Linux service or provisions device access when
extracted. Linux remains experimental even when a Windows release is stable.

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

Preserve the distinct source/designer fixtures during platform integration.
For intentional config or designer-contract changes, see [CONTRIBUTING.md](CONTRIBUTING.md).
