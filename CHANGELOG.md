# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.1.1] - 2026-05-28

Build and CI maintenance release — no changes to remapping behavior.

### Added
- CI now builds a Release binary on every push to `main` and uploads it as a
  workflow artifact, so a tested build is always available without cutting a tag.

### Changed
- `keyflow.exe` now embeds a Windows VERSIONINFO resource, so the publisher,
  version, and product name appear in the file's Properties → Details dialog.

## [2.1.0] - 2026-05-25

### Highlights

v2.1.0 brings KeyFlow's first **web designer** — a visual editor
for remappings, layers, and combos with a GMK Olivia-themed keyboard
view and share-via-Gist. A **system tray app** now hosts the running
daemon with modal error dialogs and auto-takeover. **Admin elevation
is no longer required** — KeyFlow starts as a normal user.

Under the hood: cross-platform abstraction layer (Windows backend
today, others tomorrow), `--verbose` logging, Rewire's lookup table
dropped from 128 KB to <1 KB, and two bug fixes around combos and
shutdown.


### Added
- **Web designer** (`designer/`) — a Next.js + shadcn/ui visual editor for
  KeyFlow configs. Color-coded keycaps with GMK Olivia theme, per-layer
  accents, onboarding tour, Get-keyflow.exe button, community layouts,
  Share dialog (Gist upload, URL load, PR-snippet builder), Playwright
  tests, and a GitHub Pages deploy pipeline.
- **System tray app** linked as a WINDOWS subsystem with modal error
  dialogs and auto-takeover when another instance is detected.
- **Cross-platform abstraction layer** with a Windows backend — the
  groundwork for non-Windows support without touching pipeline code.
- **`--verbose` flag** for runtime debug logging from the CLI.
- `THIRD_PARTY_NOTICES.md` documenting LGPL-3.0 attribution for
  Interception. Required because the release zip bundles `interception.dll`.
- `docs/EDUUH_DH.md` documenting the default layout's design, side-by-side
  comparison with Colemak-DH, and full physical→logical key tables.
- `ConfigRemapping.FinalStateMatchesConfig` integration test that drives every
  behavior in `src/config.json` through the real Pipeline and asserts the
  final output matches the config (88 behaviors covered today).
- `src/config/BehaviorEnumerator.h` and `tests/fixtures/PipelineHarness.h` as
  the supporting helpers behind the integration test.
- LAlt+U → `+` shiftMapping on the Numpad layer (positional Colemak-J).
- CI workflow at `.github/workflows/ci.yml` for PR validation (build + test).
- Test step in the release workflow — failing tests now block release.
- `LICENSE` file (MIT) — README claim is now backed by a real file.
- `CHANGELOG.md`, `.editorconfig`, and GitHub issue/PR templates.
- `.clangd` config, `build.ps1 -RunTests`, and CLI diagnostics.
- Eduuh-DH screenshot pipeline for layer documentation.

### Changed
- **Dropped admin-elevation requirement.** Startup errors are now clearer
  about what failed and why, instead of demanding a re-launch as admin.
- **Eduuh-DH layout:** remapped LeftWin → LeftAlt to match modern keyboard
  conventions.
- **Rewrote release workflow** as tag-only with a single concern; failures
  are loud instead of silent. Manual `workflow_dispatch` re-run available.
- **Slimmed Rewire's lookup table from 128 KB to <1 KB** by switching from
  a flat array to a sparse map keyed on what's actually remapped.
- Refactored `main` into focused helpers and optimized the hot path.
- Refactored `ConfigBuilder::addComboProcessor` into smaller helpers.
- Renamed the default layout from "Key ~ Flow Default" to "Eduuh-DH".
- Restructured: `src/scripts/` → `scripts/` (devtools out of source tree);
  `src/DebugLog.{cpp,h}` → `src/app/DebugLog.{cpp,h}` (joins peers).
- Tests are now opt-in everywhere (`-DENABLE_TESTS=ON`); the Debug-on default
  is gone.
- Test suite slimmed: 247 → 38 tests. Surviving tests target invariants the
  integration test cannot catch (named in file-level comments).
- CI: bumped GitHub Actions versions for Node 24 compatibility;
  `deploy-designer` auto-enables Pages on first run.

### Fixed
- **noModCombo double-transformation** by matching on the physical key
  instead of the remapped output, so combos no longer fire twice when the
  trigger key is itself remapped.
- **Shutdown race** between the signal handler and the main loop — shared
  state is now atomic so Ctrl-C during startup can't tear down a
  half-initialized pipeline.

### Removed
- `StrictModeFilter` feature: processor, wiring in `ConfigBuilder`, `strictMode`
  field on `JsonConfig`, parsing in `ConfigLoader`, and all related tests/docs.
  Config has been on `strictMode: false` for so long that it was dead code.
- `examples/` directory and its sole `custom-modifiers.json` file. The bundled
  `src/config.json` (Eduuh-DH) is now the only example shipped.
- Redundant scripts: `format-code.{ps1,sh}` and `lint-code.sh`. The pre-commit
  hook covers per-commit formatting/linting; CI covers whole-repo checks.
- Empty `tests/unit/hardware/` directory.
- Tests deleted as redundant with the integration test:
  `test_context.cpp`, `test_rewire.cpp`, `test_json_config.cpp`, plus large
  trims of the per-processor unit test files.

## [2.0.2] - 2026-02

### Fixed
- Stuck capitalization when typing fast with layer shiftMappings. The safety
  cleanup was checking internal modifier state before sending SHIFT UP, but
  the internal state already included the injected shift, so the SHIFT UP was
  skipped and the OS saw a stuck shift on the next keystroke. The fix sends
  the safety SHIFT UP unconditionally whenever a shift has been injected.

[Unreleased]: https://github.com/eduuh/keyflow/compare/v2.1.0...HEAD
[2.1.0]: https://github.com/eduuh/keyflow/compare/v2.0.2...v2.1.0
[2.0.2]: https://github.com/eduuh/keyflow/releases/tag/v2.0.2
