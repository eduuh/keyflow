# keyflow

[![CI](https://github.com/eduuh/keyflow/actions/workflows/ci.yml/badge.svg)](https://github.com/eduuh/keyflow/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/eduuh/keyflow?include_prereleases)](https://github.com/eduuh/keyflow/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

keyflow remaps a Windows keyboard from a JSON config. Use it to put navigation on
a layer, change modifier keys, or assign key combos without changing keyboard
firmware. The bundled config includes the Eduuh-DH layout.

The remapper uses C++17 and the Interception driver. A processor pipeline handles
modifiers, layers, remappings, and combos before sending keys back to Windows.
The [web designer](designer/) uses Next.js, React, TypeScript, and Zustand to edit
configs visually.

The runtime is Windows-only. Linux and macOS can build the platform-independent
tests, but don't have runtime backends yet. Driver installation needs admin
rights and a reboot; running keyflow afterward doesn't. Verbose logging is
opt-in and writes per-event logs, so leave it off for normal use.

## Quick start

1. Install the [Interception driver](https://github.com/oblitum/Interception/releases) (one-time, requires admin) and reboot.
2. The bundled `config.json` ships the Eduuh-DH layout — see [`docs/EDUUH_DH.md`](docs/EDUUH_DH.md) for the full spec.
3. Run `keyflow.exe`. No admin needed once the driver is installed.

Press `Ctrl+Escape` to exit.

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

Requirements: CMake 3.15+, MSVC or any C++17 compiler. Details in [BUILD.md](BUILD.md).

## CLI flags

| Flag | Effect |
|---|---|
| `--validate` | Validate config and exit |
| `-d`, `--debug` | Verbose config-load logging |
| `-v`, `--verbose` | Write per-event log to `keyflow_debug.log` |
| `[file.json]` | Use a non-default config |

## Docs

- [Config reference](docs/CONFIG_USAGE.md) · [Eduuh-DH layout](docs/EDUUH_DH.md) · [Custom modifiers](docs/CUSTOM_MODIFIERS.md) · [Privacy](docs/PRIVACY.md)
- [Contributing](CONTRIBUTING.md) · [Build](BUILD.md) · [Changelog](CHANGELOG.md)

Released under the [MIT License](LICENSE). Bundles the
[Interception](https://github.com/oblitum/Interception) driver under
LGPL-3.0 — see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
