// Flat list of every valid C++ key name, with a human-friendly display label.
// Used by the inline editors (CombosEditor, CustomModifiersEditor) to populate
// key-picker dropdowns. Should track src/config/KeyNameMapper.h.
//
// Categories are display-only (used for option-group rendering); the schema
// is flat — any of these strings is a legal key name.

export type KeyOption = {
  /** The exact string the C++ side expects in JSON. */
  code: string;
  /** What to show in the picker dropdown. */
  label: string;
};

export const KEY_GROUPS: { group: string; keys: KeyOption[] }[] = [
  {
    group: "Letters",
    keys: "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("").map((c) => ({ code: c, label: c })),
  },
  {
    group: "Numbers",
    keys: "0123456789".split("").map((c) => ({ code: c, label: c })),
  },
  {
    group: "Symbols",
    keys: [
      { code: "Grave", label: "` Grave" },
      { code: "Minus", label: "- Minus" },
      { code: "Equals", label: "= Equals" },
      { code: "LeftBracket", label: "[ LeftBracket" },
      { code: "RightBracket", label: "] RightBracket" },
      { code: "Backslash", label: "\\ Backslash" },
      { code: "Semicolon", label: "; Semicolon" },
      { code: "Apostrophe", label: "' Apostrophe" },
      { code: "Comma", label: ", Comma" },
      { code: "Period", label: ". Period" },
      { code: "Slash", label: "/ Slash" },
    ],
  },
  {
    group: "Modifiers",
    keys: [
      { code: "LeftShift", label: "LeftShift" },
      { code: "RightShift", label: "RightShift" },
      { code: "LeftCtrl", label: "LeftCtrl" },
      { code: "RightCtrl", label: "RightCtrl" },
      { code: "LeftAlt", label: "LeftAlt" },
      { code: "RightAlt", label: "RightAlt" },
      { code: "LeftWin", label: "LeftWin" },
      { code: "RightWin", label: "RightWin" },
      { code: "CapsLock", label: "CapsLock" },
    ],
  },
  {
    group: "Navigation",
    keys: [
      { code: "Up", label: "↑ Up" },
      { code: "Down", label: "↓ Down" },
      { code: "Left", label: "← Left" },
      { code: "Right", label: "→ Right" },
      { code: "Home", label: "Home" },
      { code: "End", label: "End" },
      { code: "PageUp", label: "PageUp" },
      { code: "PageDown", label: "PageDown" },
      { code: "Insert", label: "Insert" },
      { code: "Delete", label: "Delete" },
    ],
  },
  {
    group: "Other",
    keys: [
      { code: "Escape", label: "Escape" },
      { code: "Tab", label: "Tab" },
      { code: "Enter", label: "Enter" },
      { code: "Space", label: "Space" },
      { code: "Backspace", label: "Backspace" },
      { code: "PrintScreen", label: "PrintScreen" },
      { code: "Menu", label: "Menu" },
    ],
  },
  {
    group: "Function",
    keys: Array.from({ length: 12 }, (_, i) => {
      const n = i + 1;
      return { code: `F${n}`, label: `F${n}` };
    }),
  },
];

// Flat list for quick "is this a valid code" checks.
export const ALL_KEY_CODES: string[] = KEY_GROUPS.flatMap((g) => g.keys.map((k) => k.code));
