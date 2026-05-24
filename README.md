# keyflow

[![CI](https://github.com/eduuh/keyflow/actions/workflows/ci.yml/badge.svg)](https://github.com/eduuh/keyflow/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/eduuh/keyflow?include_prereleases)](https://github.com/eduuh/keyflow/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Privacy-first keyboard remapper for Windows. Zero data collection, driver-level interception, JSON config.

## Quick start

1. Install the [Interception driver](https://github.com/oblitum/Interception/releases) and reboot.
2. The bundled `config.json` ships the Eduuh-DH layout — see [`docs/EDUUH_DH.md`](docs/EDUUH_DH.md) for the full spec.
3. Run `keyflow.exe` as Administrator.

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
.\build.ps1 -Tests     # Debug + tests
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

Released under the [MIT License](LICENSE).
