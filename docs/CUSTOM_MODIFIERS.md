# Custom Modifiers

Custom modifiers allow any key to act as a modifier for triggering layers, enabling advanced keyboard layouts like Space Cadet navigation.

## Overview

KeyFlow supports up to **23 custom modifiers** in addition to the 9 standard modifiers (Shift, Ctrl, Alt, Win, PrintScreen). Custom modifiers use bits 9-31 in the modifier bitmask.

## Configuration

### Basic Syntax

```json
{
  "customModifiers": [
    {
      "key": "Space",
      "modifierName": "SPACE_MOD",
      "blockOutput": true
    }
  ],
  "layers": [
    {
      "name": "Space Navigation",
      "trigger": "SPACE_MOD",
      "mappings": {
        "H": "Left",
        "J": "Down",
        "K": "Up",
        "L": "Right"
      }
    }
  ]
}
```

### Custom Modifier Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `key` | string | Yes | Physical key name (e.g., "Space", "Tab", "A") |
| `modifierName` | string | Yes | Unique modifier name for layer triggers (e.g., "SPACE_MOD") |
| `blockOutput` | boolean | No (default: true) | Block the key's normal output when used as modifier |

### blockOutput Behavior

- **`true` (default)**: The key is consumed when pressed alone (doesn't output Space, Tab, etc.)
  - Best for dedicated modifier keys like Space Cadet
  - Example: Space+H → Left arrow, Space alone → no output

- **`false`**: The key outputs normally when pressed alone
  - Best for dual-purpose keys
  - Example: Space+H → Left arrow, Space alone → Space character
  - **Note**: Tap-hold detection is not yet implemented, so there may be a delay

## Examples

### Space Cadet Navigation

Convert Space into a navigation modifier:

```json
{
  "customModifiers": [
    {"key": "Space", "modifierName": "SPACE_MOD", "blockOutput": true}
  ],
  "layers": [
    {
      "name": "Space Navigation",
      "trigger": "SPACE_MOD",
      "mappings": {
        "H": "Left",
        "J": "Down",
        "K": "Up",
        "L": "Right",
        "U": "Home",
        "I": "PageUp",
        "O": "PageDown",
        "P": "End"
      }
    }
  ]
}
```

### Multiple Custom Modifiers

Use both Space and Tab as custom modifiers:

```json
{
  "customModifiers": [
    {"key": "Space", "modifierName": "SPACE_MOD", "blockOutput": true},
    {"key": "Tab", "modifierName": "TAB_MOD", "blockOutput": true}
  ],
  "layers": [
    {
      "name": "Space Layer",
      "trigger": "SPACE_MOD",
      "mappings": {
        "H": "Left",
        "J": "Down",
        "K": "Up",
        "L": "Right"
      }
    },
    {
      "name": "Tab Layer",
      "trigger": "TAB_MOD",
      "mappings": {
        "A": "F1",
        "S": "F2",
        "D": "F3",
        "F": "F4"
      }
    }
  ]
}
```

### Combining Standard and Custom Modifiers

You can use custom modifiers alongside standard modifiers:

```json
{
  "customModifiers": [
    {"key": "Semicolon", "modifierName": "SEMI_MOD", "blockOutput": true}
  ],
  "layers": [
    {
      "name": "RALT Layer",
      "trigger": "RALT",
      "mappings": {
        "K": "Left",
        "L": "Right"
      }
    },
    {
      "name": "Semicolon Layer",
      "trigger": "SEMI_MOD",
      "mappings": {
        "H": "Home",
        "E": "End"
      }
    }
  ]
}
```

## Limitations

1. **Maximum 23 custom modifiers** (bits 9-31)
2. **No tap-hold detection yet** - `blockOutput: false` doesn't work perfectly
3. **Custom modifier names must be unique** across all custom modifiers
4. **Custom modifiers cannot be remapped** - they use the physical scancode before remapping
5. **StrictMode compatible** - Custom modifier keys are automatically added to allowed keys

## How It Works

### Internal Architecture

1. **Modifier Bits (Modifiers.h)**
   - Standard modifiers: bits 0-8
   - Custom modifiers: bits 9-31
   - All tracked in a single 32-bit bitmask

2. **ModifierTracker (ModifierTracker.h)**
   - Maintains mapping: `scancode → custom modifier bit`
   - Maintains mapping: `modifier name → custom modifier bit`
   - Tracks custom modifier state alongside standard modifiers

3. **Layer Resolution (ConfigBuilder.h)**
   - Resolves layer triggers at config load time
   - First checks standard modifiers (LALT, RALT, etc.)
   - Then checks custom modifiers (SPACE_MOD, TAB_MOD, etc.)

4. **Key Blocking (LayerTriggerBlocker.h)**
   - Blocks custom modifier keys with `blockOutput: true`
   - Prevents them from reaching the OS when used as modifiers

### Processing Pipeline

```
Physical Key Press
    ↓
1. Rewire Processor (remaps keys)
    ↓
2. ModifierTracker (tracks standard + custom modifiers)
    ↓
3. ComboAdvanced (applies layer mappings based on active modifiers)
    ↓
4. LayerTriggerBlocker (blocks modifier keys with blockOutput=true)
    ↓
5. StrictModeFilter (if enabled)
    ↓
Output to OS
```

## Validation

The config loader validates:
- ✅ Key names are valid
- ✅ Modifier names are unique
- ✅ Maximum 23 custom modifiers
- ✅ Modifier names are not empty

Error messages clearly indicate which field has the problem.

## Future Enhancements

### Tap-Hold Detection (Planned)
- Hold Space → Space modifier (Space+H → Left)
- Tap Space → Space character
- Requires timing infrastructure (3-4 hours implementation)

### Modifier Chaining (Possible)
- Space+Tab+H → Combined modifier action
- Requires multi-modifier layer support

## See Also

- [Configuration Guide](CONFIGURATION.md) - General config documentation
- [Layer System](LAYERS.md) - How layers work
- [Examples](../examples/) - More configuration examples
