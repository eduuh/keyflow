# Keyflow Designer

Visual editor for Keyflow's `config.json`. Click-to-remap keys, manage layers,
edit number-row symbol combos, define custom modifiers, preview and edit platform
overrides, and export the layout as category-filtered PNG images.

The designer's TypeScript types track `src/config/JsonConfig.h` and the
`platformOverrides` document format in `src/config/ConfigLoader.h`. Import → export
preserves supported fields, comments, optional values and all platform sections.

## What's in it

- **Themed keyboard workspace**: physical references above output legends,
  clear selection outlines and colored layer-binding badges. The ANSI main
  block scrolls on narrow screens instead of clipping keys. Use "Other physical
  keys" for function, navigation, numpad, media and browser keys.
- **Environment presets and overrides**: switch between Shared, Linux, Windows
  and macOS. The keyboard, editors and image export show the effective selected
  environment. JSON download and sharing always keep the complete portable document.
- **Layer system**: unlimited layers, each with one or more trigger modifiers
  (multi-trigger layers OR'd together). Click the pencil icon on a layer tab
  to edit name and triggers.
- **Inline key mapping**: select a key on the board → the picker card below
  shows the current binding and tabs for `A-Z / 0-9 / Sym / Shift+ / Mod / Nav`.
  Click a target to assign. Shift mappings only work inside layers. The "All
  output keys" selector includes every canonical runtime key, including `NOP`.
- **No-modifier combos**: editor card at the bottom for `noModCombos` —
  remap the number row to symbols without holding Shift (the C++ side blocks
  the combo from firing if any modifier is held).
- **Custom modifiers**: editor card for `customModifiers` — promote any key
  (Space, Tab, etc.) to a layer trigger by name. Space-Cadet style.
- **Category-filtered PNG export**: click Export to open the export dialog.
  Categories: Layout / Letters / Numbers / Symbols / Directional / Functional
  / Return. Each renders the full keyboard with the matching keys highlighted
  in cyan. "Download all (.zip)" bundles every category in one go.
  Images show the ANSI main block and every layer binding; filenames include
  the selected environment. Keys outside that block remain available in JSON.

## Platform editing

Shared is the portable base, separate from the keyboard's BASE mapping layer.
Selecting Linux, Windows or macOS previews the runtime merge and directs edits
to that platform's `platformOverrides` section:

- Remapping entries merge by source key. Clearing an override restores the
  Shared binding; use `NOP` to block a key instead.
- `layers`, `noModCombos` and `customModifiers` replace their entire Shared
  arrays. Editing an inherited array copies it first, leaving Shared untouched.
  An explicit empty array disables that section for the selected platform.
- `disableCapsLock` overrides the Shared boolean when present.
- "Use Shared" removes one section's override. "Reset environment" removes
  the whole platform section after confirmation. If layers and custom modifiers
  have linked renamed triggers, section reset asks before restoring both together.
- Presets set `PrintScreen` to `LeftWin` on Linux/macOS (Super/Command), or
  `RightCtrl` on Windows, without changing other bindings.

Imports reject malformed supported fields instead of silently discarding them.
Legacy `trigger` strings/arrays and `triggers` strings normalize to `triggers[]`.
Custom modifier renames update dependent layer triggers.

## Run

```bash
npm install
npm run dev          # http://localhost:3000
```

## Deploy (GitHub Pages)

The `.github/workflows/deploy-designer.yml` workflow runs on every push to
`main` that touches `designer/`. It produces a static export and publishes
it to GitHub Pages at **https://eduuh.github.io/keyflow/**.

One-time setup (in repo settings):
1. Settings → Pages → Source: **GitHub Actions**
2. Push any change under `designer/` to trigger the first deploy

Local production build for testing:

```bash
npm run build              # produces designer/out/ at root path
GITHUB_PAGES=1 npm run build   # production-equivalent build at /keyflow/ basePath
```

`next.config.ts` only applies the `/keyflow` basePath when `GITHUB_PAGES=1`,
so local `npm run dev` keeps serving at the root (matters for the
Playwright tests, which hard-code `/`).

## Test

```bash
npm test             # Playwright headless
npm run test:ui      # Playwright with UI mode
npm run typecheck    # TypeScript
npm run lint         # ESLint (Next.js 16 flat config)
```

The tests live in `designer/tests/`:
- `round-trip.spec.ts` — imports `src/config.json` and confirms re-exporting
  preserves every field (the contract this whole rewrite is built around).
- `smoke.spec.ts` — page loads, key click opens picker, add-layer is gated
  on a BASE modifier remap.
- `config-support.spec.ts` — complete document round-trip, runtime merge
  semantics, inheritance, trigger aliases, atomic validation and editor regressions.
- `designer-workflows.spec.ts` — platform presets, key categories, layers,
  combos, custom modifiers, JSON editing, PNG/ZIP export, mocked sharing and
  community loading, onboarding, persistence, and mobile/light/dark views.

Install the test browser once with `npx playwright install chromium`. Sharing
tests mock GitHub requests; they never create public Gists or PRs.

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
│   │   ├── Keyboard.tsx  ANSI main block + other physical key selector
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
Changes to the document parser in `src/config/ConfigLoader.h` must also be mirrored.
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
| `platformOverrides` | `platformOverrides` | `linux`, `windows`, `mac`; merge remaps, replace arrays |

Obsolete (dropped on import): `strictMode`, `debug`.

## Tech stack

- Next.js 16 (App Router)
- Tailwind CSS + [shadcn/ui](https://ui.shadcn.com/) (Radix primitives)
- Zustand (state) with localStorage persistence (key: `keyflow-config-storage`, version `2`)
- `html-to-image` + `jszip` for PNG export
- Playwright for end-to-end tests
- TypeScript strict mode
- ESLint with Next.js core-web-vitals and TypeScript rules

## License

MIT — same as Keyflow.
