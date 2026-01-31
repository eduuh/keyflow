# Architecture

**keyflow** - Technical Architecture Documentation

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                         │
│                                                                  │
│  ┌──────────────┐         ┌─────────────────┐                  │
│  │   main.cpp   │────────▶│ ConfigLoader    │                  │
│  │              │         │ (JSON Parser)   │                  │
│  └──────┬───────┘         └────────┬────────┘                  │
│         │                          │                            │
│         │                          ▼                            │
│         │                 ┌─────────────────┐                  │
│         │                 │ ConfigBuilder   │                  │
│         │                 │ (Pipeline Setup)│                  │
│         │                 └────────┬────────┘                  │
│         │                          │                            │
│         ▼                          ▼                            │
│  ┌──────────────────────────────────────────┐                  │
│  │         Main Event Loop                  │                  │
│  │  • waitForKey()                          │                  │
│  │  • process(event)                        │                  │
│  │  • sendKey()                             │                  │
│  └──────┬───────────────────────┬───────────┘                  │
└─────────┼───────────────────────┼──────────────────────────────┘
          │                       │
          │                       │
┌─────────▼─────────┐   ┌─────────▼──────────────────────────────┐
│   Hardware Layer  │   │      Processing Pipeline               │
│   (hardware/)     │   │      (pipeline/)                       │
│                   │   │                                        │
│ ┌───────────────┐ │   │  ┌──────────────────────────────────┐ │
│ │ HardwareIO    │ │   │  │  Processor 1: Rewire             │ │
│ │               │ │   │  │  • Load key mappings             │ │
│ │ • initialize()│ │   │  │  • O(1) scancode lookup          │ │
│ │ • waitForKey()│ │   │  │  • Replace CapsLock → Ctrl       │ │
│ │ • sendKey()   │ │   │  └────────────┬─────────────────────┘ │
│ │ • shutdown()  │ │   │               │                        │
│ └───────┬───────┘ │   │               ▼                        │
│         │         │   │  ┌──────────────────────────────────┐ │
│         │         │   │  │  Processor 2: ModifierTracker    │ │
│ ┌───────▼───────┐ │   │  │  • Track held keys               │ │
│ │ Interception  │ │   │  │  • Shift/Ctrl/Alt/Win state      │ │
│ │ Driver API    │ │   │  │  • Output: 32-bit bitmask        │ │
│ │               │ │   │  └────────────┬─────────────────────┘ │
│ │ • context     │ │   │               │                        │
│ │ • wait()      │ │   │               ▼                        │
│ │ • receive()   │ │   │  ┌──────────────────────────────────┐ │
│ │ • send()      │ │   │  │  Processor 3: ComboAdvanced      │ │
│ └───────────────┘ │   │  │  • Check modifier state          │ │
│                   │   │  │  • Activate layers               │ │
│  Uses:            │   │  │  • Apply combo mappings          │ │
│  • KeyEvent       │   │  │  • RALT+K → 1, LALT+H → Left     │ │
│  • Scancodes.h    │   │  └────────────┬─────────────────────┘ │
└───────────────────┘   │               │                        │
                        │               ▼                        │
                        │  ┌──────────────────────────────────┐ │
                        │  │  ProcessingResult                │ │
                        │  │  • action: Forward/Replace/Block │ │
                        │  │  • outputScancode                │ │
                        │  │  • modifiers                     │ │
                        │  └──────────────────────────────────┘ │
                        └───────────────────────────────────────┘
```

---

## Data Flow

```
Keyboard Press (Hardware)
         │
         ▼
┌─────────────────────┐
│ Interception Driver │  Kernel-level capture
│ (interception.dll)  │  Scancode: 0x3A (CapsLock)
└──────────┬──────────┘
           │ InterceptionKeyStroke
           ▼
┌─────────────────────┐
│ HardwareIO          │  Convert to KeyEvent
│ waitForKey()        │
└──────────┬──────────┘
           │ KeyEvent{0x3A, down=true}
           ▼
┌─────────────────────┐
│ Main Loop           │  Check hotkeys (F10/F11/F12)
└──────────┬──────────┘
           │
           ▼
┌────────────────────────────────────┐
│ Pipeline.process()                 │
│                                    │
│  1. Rewire                         │
│     0x3A (CapsLock) → 0x1D (Ctrl) │
│                                    │
│  2. ModifierTracker                │
│     Update modifier bitmask        │
│                                    │
│  3. ComboAdvanced                  │
│     Check layer + apply combos     │
│                                    │
│  Return: Replace with 0x1D         │
└────────────┬───────────────────────┘
             │ ProcessingResult
             ▼
┌─────────────────────┐
│ HardwareIO          │  Inject modified key
│ sendKey(0x1D, true) │
└──────────┬──────────┘
           │ InterceptionKeyStroke
           ▼
┌─────────────────────┐
│ Interception Driver │  Send to Windows
└──────────┬──────────┘
           │
           ▼
    Windows receives Ctrl press
    (Applications see Ctrl, not CapsLock)
```

---

## Core Components

### Hardware Layer (`src/hardware/`)

**HardwareIO** - Low-level driver interface
- Wraps Interception driver API
- Blocks on `waitForKey()` with 2ms timeout
- Zero-copy key injection via `sendKey()`

**Data Structures:**
```cpp
struct KeyEvent {
  uint16_t scancode;  // Hardware scancode
  bool isDown;        // Press/release
};
```

### Processing Layer (`src/pipeline/`)

**Pipeline** - Sequential processor chain
- Maintains ordered list of processors
- Passes `Context` through each processor
- Early exit on `process() = false`

**Processors** (`src/processors/`)
1. **Rewire** - O(1) key remapping using `std::array<uint16_t, 512>`
2. **ModifierTracker** - Tracks 9 modifiers (Shift/Ctrl/Alt/Win + PrintScreen)
3. **ComboAdvanced** - Layer system with modifier-triggered mappings

**Context:**
```cpp
struct Context {
  uint16_t scancode;          // Current key
  bool isDown;                // State
  uint32_t modifiers;         // Modifier bitmask
  Action action;              // Forward/Replace/Consume
  uint16_t outputScancode;    // Replacement key
};
```

### Configuration Layer (`src/config/`)

**ConfigLoader** - JSON parsing
- Uses nlohmann/json library
- Validates required fields
- Throws on parse errors

**ConfigBuilder** - Pipeline construction
- Converts JSON → Processor chain
- Maps key names → scancodes
- Order: Rewire → ModifierTracker → ComboAdvanced

**Flow:**
```
config.json → ConfigLoader → JsonConfig → ConfigBuilder → Pipeline
```

---

## Performance Characteristics

| Component | Latency | Memory |
|-----------|---------|--------|
| Driver capture | <0.1ms | - |
| Rewire lookup | ~50ns | 1KB |
| ModifierTracker | ~20ns | 4 bytes |
| ComboAdvanced | ~200ns | ~2KB |
| Driver inject | <0.1ms | - |
| **Total** | **<2ms** | **~2MB** |

---

## Design Principles

1. **Zero-copy** - Pass by reference, minimal allocations
2. **Pipeline architecture** - Composable, ordered processing
3. **Header-only processors** - Fast compilation
4. **O(1) lookups** - Array-based remapping
5. **Privacy-first** - No I/O except config read
