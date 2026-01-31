# Configuration Usage Guide

## How Configuration Works

keyflow loads `config.json` **at runtime** from the filesystem. The configuration is **NOT compiled into the binary**, so you can modify it without recompiling.

## File Locations

### Development
```
keyflow/
├── config.json              ← Edit this during development
└── build/bin/Release/
    ├── keyflow.exe
    └── config.json          ← Auto-copied by CMake on build
```

### Deployment
When you distribute keyflow, users get:
```
keyflow/
├── keyflow.exe
├── config.json              ← Users edit this!
└── interception.dll
```

## Loading Behavior

### Default (loads config.json from current directory)
```bash
./keyflow.exe
# Loads: ./config.json
```

### Custom config file
```bash
./keyflow.exe custom.json
# Loads: ./custom.json

./keyflow.exe C:\configs\colemak.json
# Loads: C:\configs\colemak.json
```

### Search Order
1. Command-line argument (if provided)
2. `config.json` in current working directory
3. Error if not found

## Modifying Configuration

### ✅ Correct: Edit and restart
```bash
1. Edit config.json
2. Restart keyflow.exe
3. New config loads automatically ✓
```

### ❌ Wrong: Don't need to recompile!
```bash
# NO NEED TO:
cmake --build build        # ❌ Not needed
```

The whole point of JSON config is to avoid recompilation!

## Configuration File Format

See `config.json` for full example. Basic structure:

```json
{
  "version": "1.0",
  "name": "My Layout",

  "remapping": {
    "CapsLock": "LeftShift",
    "Enter": "RightShift"
  },

  "noModCombos": [
    {"key": "1", "output": "1", "shift": true}
  ],

  "layers": [
    {
      "name": "Numpad",
      "trigger": "RALT",
      "mappings": {
        "K": "1",
        "L": "2"
      }
    }
  ]
}
```

## Deployment

### Option 1: Single directory (Recommended)
```
keyflow-package/
├── keyflow.exe
├── config.json          ← Users customize this
├── interception.dll
└── README.txt
```

Distribute as a ZIP. Users extract and run.

### Option 2: System-wide installation
```
C:\Program Files\keyflow\
├── keyflow.exe
├── interception.dll
└── config.json          ← Default config

C:\Users\YourName\
└── my-keyflow-config.json

# Run with custom config:
cd "C:\Program Files\keyflow"
keyflow.exe C:\Users\YourName\my-keyflow-config.json
```

### Option 3: Multiple configs
```
keyflow/
├── keyflow.exe
├── interception.dll
├── colemak.json
├── dvorak.json
├── qwerty.json
└── gaming.json

# Switch layouts:
keyflow.exe colemak.json
keyflow.exe gaming.json
```

## Validating Configuration

If your config has errors, keyflow will report them on startup:

```bash
$ ./keyflow.exe
[Config] Loading: config.json

[Config] ERROR: Unknown key name 'InvalidKey'
[Config] Failed to load config file: config.json

Make sure:
  1. config.json exists in the current directory
  2. JSON syntax is valid
  3. All key names are recognized
```

### Valid Key Names
See `CODING_STANDARDS.md` or `src/config/KeyNameMapper.h` for full list:

**Common keys:**
- Letter keys: `A`, `B`, `C`, ... `Z`
- Number keys: `1`, `2`, `3`, ... `0`
- Modifiers: `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl`, `LeftAlt`, `RightAlt`, `LeftWin`, `RightWin`
- Special: `CapsLock`, `Enter`, `Space`, `Tab`, `Backspace`, `Escape`
- Arrows: `Up`, `Down`, `Left`, `Right`
- Symbols: `Comma`, `Period`, `Slash`, `Semicolon`, `Apostrophe`, `Grave`, `Minus`, `Equals`
- Brackets: `LeftBracket`, `RightBracket`, `Backslash`

**Case-insensitive:** `leftshift` = `LeftShift` = `LEFTSHIFT`

## Example: Switching Layouts

### Create multiple configs:

**colemak.json:**
```json
{
  "version": "1.0",
  "name": "Colemak-DH",
  "remapping": {
    "E": "F",
    "R": "P",
    "T": "G"
  }
}
```

**qwerty.json:**
```json
{
  "version": "1.0",
  "name": "QWERTY + Caps→Ctrl",
  "remapping": {
    "CapsLock": "LeftCtrl"
  }
}
```

### Switch between them:
```bash
keyflow.exe colemak.json    # Use Colemak
# Ctrl+C to exit

keyflow.exe qwerty.json     # Use QWERTY
# Ctrl+C to exit
```

## Best Practices

### ✅ Do:
- Keep config.json next to keyflow.exe
- Create backup copies before editing
- Use descriptive layer names
- Add comments via `"_comment"` keys
- Test changes incrementally

### ❌ Don't:
- Don't compile config into the binary
- Don't modify config while keyflow is running (won't reload)
- Don't use invalid key names (will fail to load)
- Don't remove the `version` field

## Hot Reload (Future Feature)

Currently, you must restart keyflow to reload config. Future versions may support:
```bash
# Press F9 (hypothetical) to reload config.json without restarting
```

---

**Key Point:** Configuration is loaded at runtime, not compiled in. Edit `config.json` and restart - no recompilation needed!
