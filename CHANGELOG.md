# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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

### Changed
- Renamed the default layout from "Key ~ Flow Default" to "Eduuh-DH".
- Restructured: `src/scripts/` → `scripts/` (devtools out of source tree);
  `src/DebugLog.{cpp,h}` → `src/app/DebugLog.{cpp,h}` (joins peers).
- Tests are now opt-in everywhere (`-DENABLE_TESTS=ON`); the Debug-on default
  is gone.
- Test suite slimmed: 247 → 38 tests. Surviving tests target invariants the
  integration test cannot catch (named in file-level comments).

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

[Unreleased]: https://github.com/eduuh/keyflow/compare/v2.0.2...HEAD
[2.0.2]: https://github.com/eduuh/keyflow/releases/tag/v2.0.2
