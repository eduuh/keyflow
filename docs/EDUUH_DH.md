# Eduuh-DH Layout

A personal Colemak-inspired keyboard layout shipped as the default Keyflow config (`src/config.json`).

## What it is

Eduuh-DH is **not** Colemak-DH. It borrows ideas from old Colemak and Colemak Mod-DH, then makes three structural changes:

1. **Right-hand letters shift right by one position**, opening a slot for V (top row) and K (home row) on the right hand's leftmost columns.
2. **Apostrophe (`'`) becomes a letter slot**, holding the O that Colemak puts on `;`.
3. **Number row and symbol punctuation are aggressively repurposed** via no-modifier combos — typing `2` produces `[`, `3` produces `]`, `8` produces `/`, etc. The shift-modified versions of these keys are also rebound.

The result is a layout optimized for the author's preferences: more letters on the home row's right hand, programming punctuation on the number row, and two thumb-accessible layers (LAlt = numpad, RAlt = arrows & symbols).

## Comparison with Colemak-DH

```
Physical QWERTY  Q W E R T   Y U I O P     A S D F G   H J K L ;   Z X C V B   N M , . /
Colemak Mod-DH:  Q W F P B   J L U Y ;     A R S T G   M N E I O   Z X C D V   K H , . /
Eduuh-DH:        Q W F P G   V J L U Y     A R S T D   / K N E I   X C B V -   Z M H , .
                 ─── ─────   ─────────     ─── ─── ─   ─ ───────   ───────── ─── ─────
                 same diff   right-shift   same diff   ! shifted    very diff    shifted
                                           (old C)     right by 1
```

### Specific divergences

| Position | Colemak-DH | Eduuh-DH | Note                                                          |
|----------|------------|----------|---------------------------------------------------------------|
| T        | B          | G        | Matches **old Colemak**, not Mod-DH                           |
| G        | G          | D        | Matches **old Colemak**; Mod-DH puts G here                   |
| H        | M          | `/`      | H pulled off home, slash takes its place                      |
| J K L ;  | N E I O    | K N E I  | Right-hand shifted right by one; K inserted at the leftmost   |
| `'`      | (none)     | O        | Apostrophe repurposed as a letter slot                        |
| Y, top   | J          | V        | V pulled up from bottom row, J L U Y shifted right            |
| Bottom   | Z X C D V K H | X C B V - Z M H | Bottom row significantly customized              |

## Base remapping (physical → letter)

### Top row
| Physical | Output |   | Physical | Output |
|----------|--------|---|----------|--------|
| Q        | Q      |   | Y        | V      |
| W        | W      |   | U        | J      |
| E        | F      |   | I        | L      |
| R        | P      |   | O        | U      |
| T        | G      |   | P        | Y      |

### Home row
| Physical | Output |   | Physical | Output |
|----------|--------|---|----------|--------|
| A        | A      |   | H        | `/`    |
| S        | R      |   | J        | K      |
| D        | S      |   | K        | N      |
| F        | T      |   | L        | E      |
| G        | D      |   | `;`      | I      |
|          |        |   | `'`      | O      |

### Bottom row
| Physical | Output |   | Physical | Output |
|----------|--------|---|----------|--------|
| Z        | X      |   | N        | Z      |
| X        | C      |   | M        | M      |
| C        | B      |   | `,`      | H      |
| V        | V      |   | `.`      | `,`    |
| B        | `-`    |   | `/`      | `.`    |

## Modifier remaps

| Physical key | Remapped to |
|--------------|-------------|
| CapsLock     | LeftShift   |
| LeftShift    | LeftCtrl    |
| RightShift   | RightCtrl   |
| Enter        | RightShift  |
| RightCtrl    | PrintScreen |

`CapsLock → Shift` is the foundational ergonomic move. `Enter → RightShift` puts a second shift under the right pinky, freeing both shift keys for use as Ctrl modifiers. CapsLock blocking (`"disableCapsLock": true`) prevents accidental caps-lock toggles.

## Number row (no-mod combos)

These fire when no modifier is held — typing the number row produces programming punctuation by default. Hold Shift to get the original number/symbol.

| Press | Produces | | Press | Produces |
|-------|----------|---|-------|----------|
| `` ` `` | `?` (Shift+/) | | `7`   | `*` (Shift+8) |
| `1`   | `!` (Shift+1) | | `8`   | `/`           |
| `2`   | `[`           | | `9`   | `'`           |
| `3`   | `]`           | | `0`   | `)` (Shift+0) |
| `4`   | `"` (Shift+') | | `=`   | `+` (Shift+=) |
| `5`   | `{` (Shift+[) | | `]`   | `:` (Shift+;) |
| `6`   | `}` (Shift+]) | | Tab   | Tab (Shift+Tab passthrough) |

## Layers

### Numpad Layer (LAlt)

Hold LeftAlt to turn the right side of the keyboard into a numeric keypad. Physical positions, not Colemak positions.

```
                 9   0   -      → 7   8   9
                 I   O   P      → 4   5   6
                 K   L   ;   '  → 1   2   3   0
                 J              → \
```

Shift-injected combos in the Numpad layer:

| LAlt + key | Produces |
|------------|----------|
| N          | `$`      |
| M          | `%`      |
| `,`        | `#`      |
| U          | `+` (numeric operator on Colemak-J position) |

### Arrows & Symbols Layer (RAlt)

Hold RightAlt for navigation and the rest of the programming symbol set.

| RAlt + key | Produces |   | RAlt + key | Produces |
|------------|----------|---|------------|----------|
| K          | Left     |   | A          | `\`      |
| L          | Down     |   | S          | Escape   |
| O          | Up       |   | D          | `9`      |
| `;`        | Right    |   | X          | `` ` ``  |
| `'`        | Enter    |   |            |          |

Shift-injected symbols:

| RAlt + key | Produces |   | RAlt + key | Produces |
|------------|----------|---|------------|----------|
| E          | `<`      |   | G          | `?`      |
| R          | `>`      |   | Z          | `*`      |
| U          | `+` (physical U = Colemak J) | | C  | `&`      |
| Q          | `@`      |   | V          | `\|`     |
| W          | `~`      |   | T          | `^`      |
| F          | `!`      |   |            |          |

## Design notes

- **No `;` letter on `;` position.** Colemak's iconic ";" → "O" mapping is gone; instead `;` produces `I` and `'` produces `O`. Saves the apostrophe-as-letter for the right pinky.
- **Numpad layer is physical-positional, not Colemak-letter-positional.** Pressing LAlt + the J key (Colemak position N) produces `\`. To get a numpad mapping for the Colemak-J letter, press LAlt + physical U.
- **Layer-shift mappings inject Shift.** When `LAlt + U → +` fires, Keyflow sends `Shift+Equals` to the OS. Shift cleanup runs before the next keystroke so the shift doesn't leak.

## Verifying the layout works

`tests/unit/integration/test_config_coverage.cpp` walks every behavior in `src/config.json` and asserts the pipeline produces the configured output. Edit `config.json`, rebuild tests, and the test surface tracks automatically:

```powershell
./build.ps1 -Tests
./builds/Debug/tests/Debug/keyflow_tests.exe --gtest_filter=ConfigRemapping.*
```
