# Keyflow Designer

Visual editor for Keyflow's `config.json`. Click-to-remap keys, manage layers,
edit number-row symbol combos, define custom modifiers, and export the layout
as category-filtered PNG images.

The designer's TypeScript types mirror `src/config/JsonConfig.h` exactly — a
round-trip (import → export) preserves every C++ schema field.

## What's in it

- **Color-coded keyboard view**: each key tints by the category of its output
  (modifier / letter / number / symbol / navigation / function / other) so
  scanning a layer tells you at a glance what kind of keys it produces.
- **Layer system**: unlimited layers, each with one or more trigger modifiers
  (multi-trigger layers OR'd together). Click the pencil icon on a layer tab
  to edit name and triggers.
- **Inline key mapping**: select a key on the board → the picker card below
  shows the current binding and tabs for `A-Z / 0-9 / Sym / Shift+ / Mod / Nav`.
  Click a target to assign. Shift mappings only work inside layers.
- **No-modifier combos**: editor card at the bottom for `noModCombos` —
  remap the number row to symbols without holding Shift (the C++ side blocks
  the combo from firing if any modifier is held).
- **Custom modifiers**: editor card for `customModifiers` — promote any key
  (Space, Tab, etc.) to a layer trigger by name. Space-Cadet style.
- **Category-filtered PNG export**: click Export to open the export dialog.
  Categories: Layout / Letters / Numbers / Symbols / Directional / Functional
  / Return. Each renders the full keyboard with the matching keys highlighted
  in cyan. "Download all (.zip)" bundles every category in one go.

## Run

```bash
npm install
npm run dev          # http://localhost:3000
```

## Test

```bash
npm test             # Playwright headless
npm run test:ui      # Playwright with UI mode
```

The tests live in `designer/tests/`:
- `round-trip.spec.ts` — imports `src/config.json` and confirms re-exporting
  preserves every field (the contract this whole rewrite is built around).
- `smoke.spec.ts` — page loads, key click opens picker, add-layer is gated
  on a BASE modifier remap.

## Architecture

```
designer/
├── app/
│   ├── layout.tsx        Root with fonts and theme provider
│   ├── page.tsx          Composes Header + LayerTabs + Keyboard + KeyPicker + editors
│   └── globals.css       shadcn neutral theme tokens (light + dark)
├── components/
│   ├── ui/               shadcn primitives (button, card, dialog, ...)
│   ├── Header.tsx        Brand + Import/Export/JSON/theme buttons
│   ├── ThemeProvider.tsx Theme context (light/dark/system)
│   ├── BaseModifierSetup.tsx  Quick-start banner shown until ≥1 modifier remapped
│   ├── Keyboard/
│   │   ├── Keyboard.tsx  ANSI 104-key layout + KeyTypeLegend
│   │   ├── Key.tsx       One key — color-coded by output type
│   │   ├── KeyTypeLegend.tsx  Color swatches with category names
│   │   ├── LayerTabs.tsx Layer selector with multi-trigger label
│   │   └── LayerEditor.tsx  Dialog for layer name and triggers[]
│   ├── KeyPicker/
│   │   └── KeyPicker.tsx Tabbed grid of mapping targets (color-coded)
│   ├── Editors/
│   │   ├── CombosEditor.tsx          noModCombos table
│   │   └── CustomModifiersEditor.tsx customModifiers table
│   └── Export/
│       └── CategoryExport.tsx  PNG export with category highlighting
└── lib/
    ├── types.ts          KeyFlowConfig / Layer / NoModCombo / CustomModifier / ShiftMapping
    ├── store.ts          Zustand store; preserves all schema fields on round-trip
    ├── keyTypes.ts       Categorize a key code; expose Tailwind class fragments
    ├── allKeys.ts        Flat list of all valid C++ key names (used in Select dropdowns)
    ├── keyboardLayout.ts ANSI 104-key positions and labels
    └── utils.ts          cn() utility (clsx + tailwind-merge)
```

## Schema contract

Whenever `src/config/JsonConfig.h` changes, `designer/lib/types.ts` must too.
The contract:

| C++ field | TS field | Notes |
|---|---|---|
| `version`           | `version`        | `"X.Y"` string |
| `name`              | `name`           | optional |
| `disableCapsLock`   | `disableCapsLock`| optional bool |
| `remapping`         | `remapping`      | `Record<string,string>`; `_comment*` keys are stripped by the C++ parser |
| `noModCombos`       | `noModCombos`    | array of `{key, output, shift?, description?}` |
| `customModifiers`   | `customModifiers`| array of `{key, modifierName, blockOutput?}` |
| `layers[].triggers` | `triggers: string[]` | canonical form; legacy `trigger: string \| string[]` is coerced on import |
| `layers[].mappings` | `mappings`       | `Record<string,string>` |
| `layers[].shiftMappings` | `shiftMappings` | `{key, output, shift?, description?}[]` |

Obsolete (dropped on import): `strictMode`, `debug`.

## Tech stack

- Next.js 16 (App Router)
- Tailwind CSS + [shadcn/ui](https://ui.shadcn.com/) (Radix primitives)
- Zustand (state) with localStorage persistence (key: `keyflow-config-storage`, version `2`)
- `html-to-image` + `jszip` for PNG export
- Playwright for end-to-end tests
- TypeScript strict mode

## License

MIT — same as Keyflow.
