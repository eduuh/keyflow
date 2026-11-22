# Privacy and Local Logging

Keyflow processes keyboard events locally. The native runtime has no telemetry,
analytics, automatic update checks, cloud sync or event-upload service. This is
not a guarantee that it never writes files.

## Default Behavior

Keyflow reads the selected JSON config and transforms keyboard events in memory
before sending output to the local OS. Per-key verbose logging is **off by
default**. Startup, configuration, device-path and error diagnostics can contain
lifecycle metadata; console output may also be retained by a launcher or terminal.
Linux singleton ownership uses a local runtime lock file.

Windows uses Interception; the experimental Linux runtime uses selected evdev
inputs and uinput through libevdev. Linux does not discover or capture every
keyboard automatically. `--validate file.json` and `--help` need no device access.

## Opt-in Per-key Disk Logging

**`-v` / `--verbose` enables sensitive per-key event logging**, including key
identities, press/release information, modifiers and timestamps. The existing
logger appends to `keyflow_debug.log`; it is not limited to Debug builds.
These records can reveal typed content even when they are not stored as text.
Leave verbose logging off for normal use.

The log path is relative to the process working directory. Linux preserves the
launch directory; Windows keeps its platform-specific path behavior. Turning
verbose logging off does not delete previous log contents. Treat existing logs
as sensitive, review them before sharing, and remove only the specific log file
when it is no longer needed. `--debug` is config/debug diagnostic output, not the
per-key disk-logging switch.

## Verification Boundaries

Review `src/app/DebugLog.{h,cpp}` and its `VERBOSE_LOG` call sites for disk
logging, `src/config/` for config reads, and `src/platform/` for device IO and
lifecycle behavior. On Windows, Process Monitor can distinguish config reads,
local log writes and network activity. Do not interpret an absence of verbose
logs as an absence of all file activity.

Use synthetic sequences in test evidence, not captured personal typing.
Runtime build/config checks do not authorize physical capture. Linux live
testing requires separately approved scoped device access and an isolated
environment; see [BUILD.md](../BUILD.md).

This document describes the native runtime, not the separately hosted web
designer, build-time dependency downloads, or OS/device-driver logging.
Audit those components independently.
