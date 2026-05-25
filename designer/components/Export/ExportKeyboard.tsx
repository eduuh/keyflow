"use client";

import { ansiLayout } from "@/lib/keyboardLayout";
import { useConfigStore } from "@/lib/store";

// Render target for keyboard exports.
//
// Used by:
//   - CategoryExport dialog: highlights category-matching keys in cyan, shows
//     base + every layer output stacked per keycap (layerFilter="all").
//   - The /screenshot route: produces documentation images for a specific
//     layer (layerFilter="base" or a layer index) or the full composite.
//
// Visual design:
//   - Each key is a tall keycap-style card with a colored face by role:
//     modifiers = amber, letters/digits = slate, punctuation = teal,
//     navigation = indigo, unchanged-from-physical = muted. Layer outputs in
//     non-"all" views accent the active layer in violet.
//   - Physical key reference (Q, W, E…) lives in the top-right corner so the
//     reader can always tell which physical key produces which output.
//   - Center label is the primary output for the current layer view.
//   - Subtle drop shadow + top highlight gradient gives the cards a sense of
//     elevation without going full skeuomorph.

export type KeyContext = {
  physicalKey: string;
  /** Every value this physical key can produce: base + layer outputs + shift outputs. */
  outputs: string[];
};

type Config = ReturnType<typeof useConfigStore.getState>["config"];

export type LayerFilter = "all" | "base" | number;

/** Build the per-key context map used for category matching. */
export function buildKeyContexts(config: Config): Map<string, KeyContext> {
  const map = new Map<string, KeyContext>();
  ansiLayout.forEach((k) => {
    const outputs: string[] = [];
    const base = config.remapping?.[k.keyCode];
    if (base) outputs.push(base);
    config.layers?.forEach((layer) => {
      const m = layer.mappings?.[k.keyCode];
      if (m) outputs.push(m);
      layer.shiftMappings?.forEach((sm) => {
        if (sm.key === k.keyCode) outputs.push(sm.output);
      });
    });
    config.noModCombos?.forEach((c) => {
      if (c.key === k.keyCode) outputs.push(c.output);
    });
    map.set(k.keyCode, { physicalKey: k.keyCode, outputs });
  });
  return map;
}

export type ExportKeyboardProps = {
  keyContexts: Map<string, KeyContext>;
  isHighlighted: (keyCode: string) => boolean;
  config: Config;
  /** Pixels per key unit. Default 64 matches the dialog; 80 is good for high-DPI export. */
  scale?: number;
  /** Which layers to surface in the labels. Default "all". */
  layerFilter?: LayerFilter;
};

export function ExportKeyboard({
  keyContexts,
  isHighlighted,
  config,
  scale = 64,
  layerFilter = "all",
}: ExportKeyboardProps) {
  const width = 15 * scale;
  const height = 5 * scale;

  return (
    <div
      className="relative"
      style={{
        width: `${width}px`,
        height: `${height}px`,
        fontFamily:
          "ui-sans-serif, system-ui, -apple-system, 'Segoe UI', sans-serif",
      }}
    >
      {ansiLayout.map((keyData) => (
        <Keycap
          key={keyData.id}
          keyData={keyData}
          scale={scale}
          config={config}
          layerFilter={layerFilter}
          highlighted={isHighlighted(keyData.keyCode)}
        />
      ))}
    </div>
  );
}

// ─── Per-key card ──────────────────────────────────────────────────────────

type KeycapProps = {
  keyData: (typeof ansiLayout)[number];
  scale: number;
  config: Config;
  layerFilter: LayerFilter;
  highlighted: boolean;
};

function Keycap({ keyData, scale, config, layerFilter, highlighted }: KeycapProps) {
  const view = resolveKeyView(keyData, config, layerFilter);
  const role = view.role;
  const palette = ROLE_PALETTE[role];

  // Highlighted state (category filter in dialog) shifts to a bright cyan ring.
  const accent = highlighted ? "#22D3EE" : palette.accent;
  const ringWidth = highlighted ? 2 : 1;

  const padding = 6;
  const w = keyData.width * scale - 4;
  const h = keyData.height * scale - 4;

  // Sizing scales with key size so a 2u Backspace doesn't dwarf its label.
  // Long words like "Shift" / "PrtSc" get a smaller primary font so they fit.
  const isWideLabel = view.primary.length >= 4 && /^[A-Za-z]+$/.test(view.primary);
  const baseFont = Math.round(scale * 0.32);
  const primaryFontSize = isWideLabel
    ? Math.max(13, Math.round(scale * 0.22))
    : Math.max(14, baseFont);
  const overlayFontSize = Math.max(10, Math.round(scale * 0.16));

  return (
    <div
      style={{
        position: "absolute",
        left: `${keyData.x * scale}px`,
        top: `${keyData.y * scale}px`,
        width: `${w}px`,
        height: `${h}px`,
        borderRadius: "8px",
        background: palette.face,
        backgroundImage: `linear-gradient(180deg, ${palette.faceTop} 0%, ${palette.face} 55%, ${palette.faceBottom} 100%)`,
        boxShadow: highlighted
          ? `0 0 0 ${ringWidth}px ${accent}, 0 1px 0 rgba(0,0,0,0.55), 0 4px 10px rgba(0,0,0,0.35)`
          : `inset 0 1px 0 ${palette.highlight}, 0 1px 0 rgba(0,0,0,0.55), 0 3px 6px rgba(0,0,0,0.32)`,
        border: highlighted ? "none" : `1px solid ${palette.accent}33`,
        color: palette.text,
        overflow: "hidden",
      }}
    >
      {/* Layer-0 overlay (top-left) — "all" view only. Surfaces the layer-0
          mapping next to the BASE output so you can see at a glance what each
          layer adds without leaving the composite view. */}
      {view.topLeft && (
        <div
          style={{
            position: "absolute",
            top: padding - 1,
            left: padding,
            fontSize: `${overlayFontSize}px`,
            color: view.topLeftColor ?? palette.muted,
            fontWeight: 600,
          }}
        >
          {view.topLeft}
        </div>
      )}

      {/* Center: the final output for the current view — what pressing this
          key actually produces. Color comes from the role palette. */}
      <div
        style={{
          position: "absolute",
          inset: 0,
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          padding: `${padding}px`,
        }}
      >
        <span
          style={{
            fontSize: `${primaryFontSize}px`,
            fontWeight: 700,
            letterSpacing: "-0.01em",
            lineHeight: 1,
            color: view.primaryColor ?? palette.text,
            textAlign: "center",
          }}
        >
          {view.primary}
        </span>
      </div>

      {/* Bottom-right overlay — "all" view only. The layer-1 mapping. */}
      {view.bottomRight && (
        <div
          style={{
            position: "absolute",
            bottom: padding - 1,
            right: padding,
            fontSize: `${overlayFontSize}px`,
            color: view.bottomRightColor ?? palette.muted,
            fontWeight: 600,
          }}
        >
          {view.bottomRight}
        </div>
      )}
    </div>
  );
}

// ─── View model: what each key shows ───────────────────────────────────────

type KeyView = {
  /** The big centered label — always the FINAL output for the current view. */
  primary: string;
  primaryColor?: string;
  /** Top-left overlay — "all" view only: layer-0 output. */
  topLeft?: string;
  topLeftColor?: string;
  /** Bottom-right overlay — "all" view only: layer-1 output. */
  bottomRight?: string;
  bottomRightColor?: string;
  /** Drives the card color. */
  role: KeyRole;
};

function resolveKeyView(
  keyData: (typeof ansiLayout)[number],
  config: Config,
  filter: LayerFilter,
): KeyView {
  const physicalLabel = keyData.label;
  const baseOutput = baseOutputFor(keyData.keyCode, config);
  const layers = config.layers ?? [];

  if (filter === "base") {
    // BASE view: every key shows what it produces with no modifier held —
    // including noModCombos so 1 shows "!", 2 shows "[", etc.
    const display = baseOutput ?? physicalLabel;
    return {
      primary: compactLabel(display),
      role: classifyRole(display, baseOutput != null),
    };
  }

  if (typeof filter === "number") {
    // Per-layer view: show the FINAL output for every key when this layer is
    // active. Layer-mapped keys produce the layer's mapping; everything else
    // falls through to the BASE output unchanged. Card color signals whether
    // the layer affected this key (violet) or not (the key's natural role).
    const layer = layers[filter];
    const mapping = layer?.mappings?.[keyData.keyCode];
    const shift = layer?.shiftMappings?.find((m) => m.key === keyData.keyCode);

    if (mapping) {
      return {
        primary: compactLabel(mapping),
        primaryColor: ACCENT_LAYER,
        role: "layerOutput",
      };
    }

    if (shift) {
      return {
        primary: `⇧${compactLabel(shift.output)}`,
        primaryColor: ACCENT_SHIFT,
        role: "layerOutput",
      };
    }

    // No layer activity on this key — pressing it with the layer trigger held
    // still produces whatever BASE produces. Show that at full clarity.
    const display = baseOutput ?? physicalLabel;
    return {
      primary: compactLabel(display),
      role: classifyRole(display, baseOutput != null),
    };
  }

  // "all" view: BASE output prominent, layer outputs in opposite corners.
  const layer0 = layers[0]?.mappings?.[keyData.keyCode];
  const layer1 = layers[1]?.mappings?.[keyData.keyCode];
  const shift0 = layers[0]?.shiftMappings?.find((m) => m.key === keyData.keyCode);
  const shift1 = layers[1]?.shiftMappings?.find((m) => m.key === keyData.keyCode);

  const topLeft =
    layer0 != null
      ? compactLabel(layer0)
      : shift0
      ? `⇧${compactLabel(shift0.output)}`
      : undefined;
  const bottomRight =
    layer1 != null
      ? compactLabel(layer1)
      : shift1
      ? `⇧${compactLabel(shift1.output)}`
      : undefined;

  const display = baseOutput ?? physicalLabel;

  return {
    primary: compactLabel(display),
    topLeft,
    topLeftColor: ACCENT_LAYER_DIM,
    bottomRight,
    bottomRightColor: ACCENT_LAYER_DIM,
    role: classifyRole(display, baseOutput != null),
  };
}

// ─── Data helpers ──────────────────────────────────────────────────────────

/** What does pressing this key produce with no modifier held?
 *  Considers both explicit remapping AND noModCombos (which fire only when
 *  no modifier is held — they're effectively part of the BASE behavior).
 *  Shift-injected combos return the keycode prefixed with ⇧ so the doc shows
 *  the visible shifted character (e.g. 1 → ⇧1, then the consumer resolves
 *  ⇧1 to "!" via the QWERTY shift map).
 */
function baseOutputFor(keyCode: string, config: Config): string | null {
  const remap = config.remapping?.[keyCode];
  if (remap) return remap;
  const combo = config.noModCombos?.find((c) => c.key === keyCode);
  if (combo) {
    if (combo.shift) {
      const shifted = US_SHIFT[combo.output];
      return shifted ?? `⇧${combo.output}`;
    }
    return combo.output;
  }
  return null;
}

const US_SHIFT: Record<string, string> = {
  "1": "!", "2": "@", "3": "#", "4": "$", "5": "%",
  "6": "^", "7": "&", "8": "*", "9": "(", "0": ")",
  Grave: "~",
  Minus: "_",
  Equals: "+",
  LeftBracket: "{",
  RightBracket: "}",
  Backslash: "|",
  Semicolon: ":",
  Apostrophe: '"',
  Comma: "<",
  Period: ">",
  Slash: "?",
};

// ─── Role classification + palette ─────────────────────────────────────────

type KeyRole =
  | "modifier"      // CapsLock/Shift/Ctrl/Alt/Win/Enter/etc. (or a remap that produces a modifier)
  | "letter"        // A-Z output
  | "digit"         // 0-9 output (kept distinct so we can color number-row differently if we ever want)
  | "punctuation"   // -, =, [, ], etc. and their shifted forms
  | "navigation"    // arrows, Home/End, PgUp/Dn
  | "special"       // Space, Tab, Backspace, Esc, Delete
  | "layerOutput"   // any key showing a layer-specific mapping (per-layer view only)
  | "inactive"      // per-layer view: key has no layer activity
  | "unchanged";    // BASE/all view: key produces its physical label, nothing remapped

const MODIFIER_OUTPUTS = new Set([
  "LeftShift", "RightShift", "LeftCtrl", "RightCtrl", "LeftAlt", "RightAlt",
  "LeftWin", "RightWin", "CapsLock", "PrintScreen",
]);
const NAVIGATION_OUTPUTS = new Set([
  "Up", "Down", "Left", "Right", "Home", "End", "PageUp", "PageDown", "Insert",
]);
const SPECIAL_OUTPUTS = new Set([
  "Space", "Tab", "Backspace", "Enter", "Escape", "Delete", "Menu",
]);
const PUNCT_KEYCODES = new Set([
  "Grave", "Minus", "Equals", "LeftBracket", "RightBracket", "Backslash",
  "Semicolon", "Apostrophe", "Comma", "Period", "Slash",
]);
const PUNCT_CHARS = new Set([
  "!", "@", "#", "$", "%", "^", "&", "*", "(", ")",
  "~", "_", "+", "{", "}", "|", ":", '"', "<", ">", "?",
  "`", "-", "=", "[", "]", "\\", ";", "'", ",", ".", "/",
]);

function classifyRole(display: string, isRemapped: boolean): KeyRole {
  if (MODIFIER_OUTPUTS.has(display)) return "modifier";
  if (NAVIGATION_OUTPUTS.has(display)) return "navigation";
  if (SPECIAL_OUTPUTS.has(display)) return "special";
  if (PUNCT_KEYCODES.has(display) || PUNCT_CHARS.has(display)) return "punctuation";
  if (/^\d$/.test(display)) return "digit";
  if (/^[A-Z]$/.test(display)) return "letter";
  if (!isRemapped) return "unchanged";
  return "letter";
}

type Palette = {
  face: string;
  faceTop: string;
  faceBottom: string;
  highlight: string;
  accent: string;
  text: string;
  muted: string;
};

const ROLE_PALETTE: Record<KeyRole, Palette> = {
  modifier: {
    face: "#7C5410",
    faceTop: "#A06B14",
    faceBottom: "#5A3D0B",
    highlight: "rgba(255,210,140,0.35)",
    accent: "#F59E0B",
    text: "#FFF1D6",
    muted: "rgba(255,241,214,0.55)",
  },
  letter: {
    face: "#2A3445",
    faceTop: "#384355",
    faceBottom: "#1F2735",
    highlight: "rgba(180,200,230,0.18)",
    accent: "#60A5FA",
    text: "#F1F5F9",
    muted: "rgba(241,245,249,0.5)",
  },
  digit: {
    face: "#2A3445",
    faceTop: "#384355",
    faceBottom: "#1F2735",
    highlight: "rgba(180,200,230,0.18)",
    accent: "#60A5FA",
    text: "#F1F5F9",
    muted: "rgba(241,245,249,0.5)",
  },
  punctuation: {
    face: "#0E5752",
    faceTop: "#147068",
    faceBottom: "#093E3A",
    highlight: "rgba(180,255,240,0.22)",
    accent: "#2DD4BF",
    text: "#CCFBF1",
    muted: "rgba(204,251,241,0.55)",
  },
  navigation: {
    face: "#2A2768",
    faceTop: "#363288",
    faceBottom: "#1E1C4B",
    highlight: "rgba(190,200,255,0.22)",
    accent: "#818CF8",
    text: "#E0E7FF",
    muted: "rgba(224,231,255,0.55)",
  },
  special: {
    face: "#293040",
    faceTop: "#363E51",
    faceBottom: "#1D222E",
    highlight: "rgba(200,210,230,0.16)",
    accent: "#94A3B8",
    text: "#E2E8F0",
    muted: "rgba(226,232,240,0.5)",
  },
  layerOutput: {
    face: "#3F1D7A",
    faceTop: "#522799",
    faceBottom: "#2A1455",
    highlight: "rgba(220,200,255,0.25)",
    accent: "#A78BFA",
    text: "#F3E8FF",
    muted: "rgba(243,232,255,0.55)",
  },
  inactive: {
    face: "#161B26",
    faceTop: "#1B2230",
    faceBottom: "#10141C",
    highlight: "rgba(120,140,180,0.06)",
    accent: "#1F2937",
    text: "rgba(255,255,255,0.42)",
    muted: "rgba(255,255,255,0.22)",
  },
  unchanged: {
    face: "#1A2030",
    faceTop: "#222A3D",
    faceBottom: "#131826",
    highlight: "rgba(180,200,230,0.10)",
    accent: "#334155",
    text: "rgba(241,245,249,0.78)",
    muted: "rgba(241,245,249,0.4)",
  },
};

const ACCENT_LAYER = "#C4B5FD";       // violet — primary text in per-layer view
const ACCENT_LAYER_DIM = "#A78BFA";   // softer violet — overlay layer outputs in "all"
const ACCENT_SHIFT = "#FCD34D";       // amber — shift-injected primary

// Shrink long key names so they fit on a keycap face. Uses readable words
// (Shift, Ctrl, Alt, Win) instead of cryptic glyphs (⇧, ⌃, ⌥, ⊞). Drops the
// Left/Right prefix because the physical position on the keyboard already
// tells you which one — left-side keys show "Shift", right-side show "Shift",
// and the layout makes the distinction visually obvious.
function compactLabel(label: string): string {
  const short: Record<string, string> = {
    LeftShift: "Shift",
    RightShift: "Shift",
    LeftCtrl: "Ctrl",
    RightCtrl: "Ctrl",
    LeftAlt: "Alt",
    RightAlt: "Alt",
    LeftWin: "Win",
    RightWin: "Win",
    CapsLock: "Caps",
    Backspace: "⌫",
    Enter: "↵",
    Escape: "Esc",
    PageUp: "PgUp",
    PageDown: "PgDn",
    Insert: "Ins",
    Delete: "Del",
    PrintScreen: "PrtSc",
    Up: "↑",
    Down: "↓",
    Left: "←",
    Right: "→",
    Grave: "`",
    Minus: "-",
    Equals: "=",
    LeftBracket: "[",
    RightBracket: "]",
    Backslash: "\\",
    Semicolon: ";",
    Apostrophe: "'",
    Comma: ",",
    Period: ".",
    Slash: "/",
    Space: "Space",
    Tab: "Tab",
    Menu: "Menu",
  };
  return short[label] || label;
}
