// Categorize a key code into a visual type. Used to tint keys on the
// keyboard view and in the picker so the eye can quickly tell modifier
// from letter from number from symbol.
//
// The palette below follows the GMK Olivia keycap-set convention:
//   - ALPHA keys (letters, numbers, symbols) sit on a CREAM card surface
//     with dark legends — this is the dominant visual.
//   - MODIFIER keys (Shift, Ctrl, Alt, etc. + Tab, Enter, Space, Backspace)
//     sit on a DARK MOCHA surface with cream legends — the "accent" color
//     block on a real Olivia keyboard.
//   - NAVIGATION and FUNCTION get DUSTY PINK and BURGUNDY tints respectively
//     (the real set uses pink/burgundy for special keys).
//
// The actual hex values come from CSS variables in globals.css; this file
// just selects which token to apply.

export type KeyType = "modifier" | "letter" | "number" | "symbol" | "navigation" | "function" | "other";

const MODIFIERS = new Set([
  "LeftShift", "RightShift",
  "LeftCtrl", "RightCtrl",
  "LeftAlt", "RightAlt",
  "LeftWin", "RightWin",
  "CapsLock",
  "PrintScreen",
]);

const SYMBOLS = new Set([
  "Grave", "Minus", "Equals",
  "LeftBracket", "RightBracket", "Backslash",
  "Semicolon", "Apostrophe",
  "Comma", "Period", "Slash",
]);

const NAVIGATION = new Set([
  "Up", "Down", "Left", "Right",
  "Home", "End", "PageUp", "PageDown",
  "Insert", "Delete",
]);

const OTHER_KEYS = new Set([
  "Escape", "Tab", "Enter", "Space", "Backspace", "Menu",
]);

export function getKeyType(keyCode: string): KeyType {
  if (MODIFIERS.has(keyCode)) return "modifier";
  if (SYMBOLS.has(keyCode)) return "symbol";
  if (NAVIGATION.has(keyCode)) return "navigation";
  if (OTHER_KEYS.has(keyCode)) return "other";
  if (/^F\d+$/.test(keyCode)) return "function";
  if (/^[A-Z]$/.test(keyCode)) return "letter";
  if (/^\d$/.test(keyCode)) return "number";
  return "other";
}

// Tailwind class fragments for each type. Two main blocks: alphas on cream,
// modifiers on mocha. Navigation gets a pink wash; function gets a burgundy
// tint. Picked from the theme's CSS variables so they follow light/dark.
export const KEY_TYPE_STYLES: Record<KeyType, { tint: string; dot: string; label: string }> = {
  modifier: {
    // Dark mocha surface with cream legends — the GMK Olivia accent block.
    tint: "bg-modifier text-modifier-foreground border-modifier hover:bg-modifier/90",
    dot: "bg-[hsl(var(--modifier))]",
    label: "Modifier",
  },
  letter: {
    // Cream surface, dark legends.
    tint: "bg-card text-foreground border-border hover:bg-accent/40",
    dot: "bg-[hsl(var(--card))] border border-[hsl(var(--border))]",
    label: "Letter",
  },
  number: {
    tint: "bg-card text-foreground border-border hover:bg-accent/40",
    dot: "bg-[hsl(var(--card))] border border-[hsl(var(--border))]",
    label: "Number",
  },
  symbol: {
    // Slightly tinted cream — distinguish symbol keys from pure alphas.
    tint: "bg-muted text-foreground border-border hover:bg-accent/40",
    dot: "bg-[hsl(var(--muted))] border border-[hsl(var(--border))]",
    label: "Symbol",
  },
  navigation: {
    // Dusty pink wash — the GMK Olivia "novelty" tone.
    tint: "bg-accent text-accent-foreground border-accent hover:bg-accent/80",
    dot: "bg-[hsl(var(--accent))]",
    label: "Navigation",
  },
  function: {
    // Burgundy tint — used for layer triggers and F-row.
    tint: "bg-primary/15 text-primary border-primary/40 hover:bg-primary/25",
    dot: "bg-[hsl(var(--primary))]",
    label: "Function",
  },
  other: {
    // Tab, Enter, Space, Backspace — these are functional too, treat as modifier-y.
    tint: "bg-modifier/90 text-modifier-foreground border-modifier hover:bg-modifier",
    dot: "bg-[hsl(var(--modifier))]",
    label: "Other",
  },
};
