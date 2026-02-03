# Contributing to Keyflow

## Important: Config Changes

⚠️ **CRITICAL**: When making changes to `src/config.json`, you MUST also update the designer codebase.

The config file is used by both:
1. **Keyflow** (this codebase) - The runtime keyboard remapper
2. **Designer** (separate codebase) - The configuration GUI/editor

### Steps when modifying config.json:

1. Make changes to `src/config.json` in this repo
2. Test the changes with Keyflow
3. **Update the designer codebase** with the same changes
4. Commit both changes together or note in commit message

## Build System

All builds go to `builds/` directory:
- Use `.\build.ps1` for automated builds
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

## Git Workflow

1. Work on feature branches
2. Commit with descriptive messages
3. Ensure tests pass
4. Push to remote

## Architecture Notes

- **Single-instance enforcement**: Only one Keyflow process can run
- **Zero-latency design**: No artificial delays in event processing
- **Pipeline architecture**: Events flow through processor chain
