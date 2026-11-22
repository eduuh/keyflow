# keyflow

[![CI](https://github.com/eduuh/keyflow/actions/workflows/ci.yml/badge.svg)](https://github.com/eduuh/keyflow/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/eduuh/keyflow?include_prereleases)](https://github.com/eduuh/keyflow/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

keyflow remaps a keyboard from a JSON config. Use it to put navigation on
a layer, change modifier keys, or assign key combos without changing keyboard
firmware. The bundled config includes the Eduuh-DH layout.

The remapper uses C++17 and the Interception driver on Windows, or libevdev with
evdev/uinput on Linux. A processor pipeline handles modifiers, layers, remappings,
and combos before sending keys back to the OS.
The [web designer](designer/) uses Next.js, React, TypeScript, and Zustand to edit
configs visually.

Windows is the established runtime. **Linux is EXPERIMENTAL**: the initial target
is x86_64 with Ubuntu 24.04 build/CI integration. Kernel IO and application-visible
behavior require separate live verification; a green build or unit suite does not
prove them. macOS remains tests-only. Verbose logging is opt-in and writes per-key
events to disk, so leave it off for normal use.

## Windows quick start

1. Install the [Interception driver](https://github.com/oblitum/Interception/releases) (one-time, requires admin) and reboot.
2. The bundled `config.json` ships the Eduuh-DH layout — see [`docs/EDUUH_DH.md`](docs/EDUUH_DH.md) for the full spec.
3. Run `keyflow.exe`. No admin needed once the driver is installed.

Press `Ctrl+Escape` to exit.

## Linux quick start (experimental)

Build with CMake, a C++17 compiler, pkg-config and libevdev development headers;
see [BUILD.md](BUILD.md) for commands and the device-access gate. The executable
runs in the foreground without a tray or automatic device discovery.
Validation needs no device access:

```sh
builds/Linux-Debug/debug/bin/keyflow --validate "$PWD/src/config.json"
builds/Linux-Debug/debug/bin/keyflow --help
```

Only after explicitly authorizing scoped access to the selected keyboard and
`/dev/uinput`, replace the example device path with your verified keyboard path:

```sh
builds/Linux-Debug/debug/bin/keyflow \
  --device /dev/input/by-id/your-keyboard-event-kbd \
  "$PWD/src/config.json"
```

Repeat `--device` for each explicitly selected keyboard. Keep an unaffected stop
channel. Stop with physical `Ctrl+Escape` (either Ctrl), foreground `Ctrl+C`, or
SIGTERM to the verified Keyflow PID. No services or permission settings are changed
by Keyflow; stopping it is the runtime rollback. See [BUILD.md](BUILD.md) for safety
and rollback details. Do not use broad input-group membership or world-writable
device nodes as a shortcut.

Existing configs retain physical Windows `SC_*` key identities, not universal
glyphs or desktop/workspace shortcuts. The OS keymap, NumLock/AltGr, compositor
and desktop bindings can change visible results.

For separately authorized boot startup, use the opt-in
[Linux systemd installation and Kanata rollback guide](docs/LINUX_SERVICE.md).
The installer is for fresh deployments and does not start or enable capture.
Do not run it over an existing working service.

## Example config

```json
{
  "version": "1.0",
  "remapping": {
    "CapsLock": "LeftCtrl"
  },
  "layers": [
    {
      "name": "Vim Nav",
      "triggers": ["RightAlt"],
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

See [docs/CONFIG_USAGE.md](docs/CONFIG_USAGE.md) for the full reference.

## Build from source

```powershell
.\build.ps1            # Release
.\build.ps1 -Tests     # Release + tests
```

Windows requirements: CMake 3.15+ and MSVC or another compatible C++17 compiler.
Linux uses direct CMake commands under `builds/`, not `build.ps1`.
Details in [BUILD.md](BUILD.md).

## CLI flags

| Flag | Effect |
|---|---|
| `--validate file.json` | Validate config and exit without capturing input |
| `--help` | Show usage and exit without capturing input |
| `--device path` | Linux: explicitly select a keyboard; repeatable, required for runtime |
| `-d`, `--debug` | Verbose config-load logging |
| `-v`, `--verbose` | Write per-event log to `keyflow_debug.log` |
| `[file.json]` | Use a non-default config; Linux keeps the launch CWD, so prefer an absolute path |

## Docs

- [Config reference](docs/CONFIG_USAGE.md) · [Eduuh-DH layout](docs/EDUUH_DH.md) · [Custom modifiers](docs/CUSTOM_MODIFIERS.md) · [Privacy](docs/PRIVACY.md)
- [Contributing](CONTRIBUTING.md) · [Build](BUILD.md) · [Changelog](CHANGELOG.md)

Released under the [MIT License](LICENSE). Windows builds bundle the
[Interception](https://github.com/oblitum/Interception) driver under
LGPL-3.0 — see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
Linux uses the system libevdev library and does not bundle Interception.
