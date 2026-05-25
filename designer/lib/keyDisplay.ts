// Display labels for key codes. The C++ schema speaks in key-code names
// ("Slash", "Grave", "LeftShift") but the UI needs glyphs ("/", "`", "⇧"),
// otherwise users see literal strings on the keycaps. One place to look up
// what to render — shared between the keyboard view and the picker.

// Glyph for an unshifted key code. Unknown codes fall through unchanged so
// nothing ever vanishes — worst case the user sees the raw name.
const UNSHIFTED_LABELS: Record<string, string> = {
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

  LeftShift: "⇧",
  RightShift: "⇧",
  LeftCtrl: "CTRL",
  RightCtrl: "CTRL",
  LeftAlt: "ALT",
  RightAlt: "ALT",
  LeftWin: "WIN",
  RightWin: "WIN",
  CapsLock: "CAPS",
  PrintScreen: "PRTSC",
  Menu: "MENU",

  Tab: "TAB",
  Enter: "↵",
  Space: "SPACE",
  Backspace: "⌫",
  Escape: "ESC",
  Delete: "DEL",
  Insert: "INS",
  Home: "HOME",
  End: "END",
  PageUp: "PGUP",
  PageDown: "PGDN",

  Up: "↑",
  Down: "↓",
  Left: "←",
  Right: "→",
};

// Glyph for shift + key code. Mirrors the QWERTY shifted-symbol layout —
// same 21 entries that used to live inline in KeyPicker.
const SHIFTED_LABELS: Record<string, string> = {
  Grave: "~",
  "1": "!",
  "2": "@",
  "3": "#",
  "4": "$",
  "5": "%",
  "6": "^",
  "7": "&",
  "8": "*",
  "9": "(",
  "0": ")",
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

export function displayLabel(keyCode: string): string {
  return UNSHIFTED_LABELS[keyCode] ?? keyCode;
}

export function shiftedDisplayLabel(keyCode: string): string {
  // Letters: shift just uppercases — already the canonical render.
  if (/^[A-Z]$/.test(keyCode)) return keyCode;
  const glyph = SHIFTED_LABELS[keyCode];
  if (glyph) return glyph;
  // Fallback: prefix the unshifted label with the shift glyph so the user
  // still sees something legible for codes outside the QWERTY shifted set.
  return `⇧${displayLabel(keyCode)}`;
}

// The "shifted" tab in the picker needs the full list to render buttons.
// Exported so KeyPicker can iterate without redefining the table.
export type ShiftedKey = { label: string; code: string };
export const SHIFTED_KEYS: ShiftedKey[] = Object.entries(SHIFTED_LABELS).map(
  ([code, label]) => ({ code, label }),
);

// Modifier-name trigger ("LALT") → physical ANSI key code ("LeftAlt").
// Layer triggers in C++ speak the modifier-name language; the keyboard view
// renders physical keys. Use this map to highlight the trigger key when
// viewing its layer.
export const TRIGGER_ALIAS: Record<string, string> = {
  LALT: "LeftAlt",
  RALT: "RightAlt",
  LCTRL: "LeftCtrl",
  RCTRL: "RightCtrl",
  LSHIFT: "LeftShift",
  RSHIFT: "RightShift",
  LWIN: "LeftWin",
  RWIN: "RightWin",
  CAPS: "CapsLock",
};
