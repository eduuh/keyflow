// ANSI keyboard layout data
export type KeyLayoutData = {
  id: string;
  label: string;
  keyCode: string;
  x: number; // Grid position
  y: number;
  width: number; // in key units (1u = 1 key width)
  height: number;
};

// Standard ANSI 104-key layout
export const ansiLayout: KeyLayoutData[] = [
  // Number row
  { id: "grave", label: "`", keyCode: "Grave", x: 0, y: 0, width: 1, height: 1 },
  { id: "1", label: "1", keyCode: "1", x: 1, y: 0, width: 1, height: 1 },
  { id: "2", label: "2", keyCode: "2", x: 2, y: 0, width: 1, height: 1 },
  { id: "3", label: "3", keyCode: "3", x: 3, y: 0, width: 1, height: 1 },
  { id: "4", label: "4", keyCode: "4", x: 4, y: 0, width: 1, height: 1 },
  { id: "5", label: "5", keyCode: "5", x: 5, y: 0, width: 1, height: 1 },
  { id: "6", label: "6", keyCode: "6", x: 6, y: 0, width: 1, height: 1 },
  { id: "7", label: "7", keyCode: "7", x: 7, y: 0, width: 1, height: 1 },
  { id: "8", label: "8", keyCode: "8", x: 8, y: 0, width: 1, height: 1 },
  { id: "9", label: "9", keyCode: "9", x: 9, y: 0, width: 1, height: 1 },
  { id: "0", label: "0", keyCode: "0", x: 10, y: 0, width: 1, height: 1 },
  { id: "minus", label: "-", keyCode: "Minus", x: 11, y: 0, width: 1, height: 1 },
  { id: "equals", label: "=", keyCode: "Equals", x: 12, y: 0, width: 1, height: 1 },
  { id: "backspace", label: "⌫", keyCode: "Backspace", x: 13, y: 0, width: 2, height: 1 },

  // Top row
  { id: "tab", label: "TAB", keyCode: "Tab", x: 0, y: 1, width: 1.5, height: 1 },
  { id: "q", label: "Q", keyCode: "Q", x: 1.5, y: 1, width: 1, height: 1 },
  { id: "w", label: "W", keyCode: "W", x: 2.5, y: 1, width: 1, height: 1 },
  { id: "e", label: "E", keyCode: "E", x: 3.5, y: 1, width: 1, height: 1 },
  { id: "r", label: "R", keyCode: "R", x: 4.5, y: 1, width: 1, height: 1 },
  { id: "t", label: "T", keyCode: "T", x: 5.5, y: 1, width: 1, height: 1 },
  { id: "y", label: "Y", keyCode: "Y", x: 6.5, y: 1, width: 1, height: 1 },
  { id: "u", label: "U", keyCode: "U", x: 7.5, y: 1, width: 1, height: 1 },
  { id: "i", label: "I", keyCode: "I", x: 8.5, y: 1, width: 1, height: 1 },
  { id: "o", label: "O", keyCode: "O", x: 9.5, y: 1, width: 1, height: 1 },
  { id: "p", label: "P", keyCode: "P", x: 10.5, y: 1, width: 1, height: 1 },
  { id: "leftbracket", label: "[", keyCode: "LeftBracket", x: 11.5, y: 1, width: 1, height: 1 },
  { id: "rightbracket", label: "]", keyCode: "RightBracket", x: 12.5, y: 1, width: 1, height: 1 },
  { id: "backslash", label: "\\", keyCode: "Backslash", x: 13.5, y: 1, width: 1.5, height: 1 },

  // Home row
  { id: "capslock", label: "CAPS", keyCode: "CapsLock", x: 0, y: 2, width: 1.75, height: 1 },
  { id: "a", label: "A", keyCode: "A", x: 1.75, y: 2, width: 1, height: 1 },
  { id: "s", label: "S", keyCode: "S", x: 2.75, y: 2, width: 1, height: 1 },
  { id: "d", label: "D", keyCode: "D", x: 3.75, y: 2, width: 1, height: 1 },
  { id: "f", label: "F", keyCode: "F", x: 4.75, y: 2, width: 1, height: 1 },
  { id: "g", label: "G", keyCode: "G", x: 5.75, y: 2, width: 1, height: 1 },
  { id: "h", label: "H", keyCode: "H", x: 6.75, y: 2, width: 1, height: 1 },
  { id: "j", label: "J", keyCode: "J", x: 7.75, y: 2, width: 1, height: 1 },
  { id: "k", label: "K", keyCode: "K", x: 8.75, y: 2, width: 1, height: 1 },
  { id: "l", label: "L", keyCode: "L", x: 9.75, y: 2, width: 1, height: 1 },
  { id: "semicolon", label: ";", keyCode: "Semicolon", x: 10.75, y: 2, width: 1, height: 1 },
  { id: "apostrophe", label: "'", keyCode: "Apostrophe", x: 11.75, y: 2, width: 1, height: 1 },
  { id: "enter", label: "↵", keyCode: "Enter", x: 12.75, y: 2, width: 2.25, height: 1 },

  // Bottom row
  { id: "leftshift", label: "SHIFT", keyCode: "LeftShift", x: 0, y: 3, width: 2.25, height: 1 },
  { id: "z", label: "Z", keyCode: "Z", x: 2.25, y: 3, width: 1, height: 1 },
  { id: "x", label: "X", keyCode: "X", x: 3.25, y: 3, width: 1, height: 1 },
  { id: "c", label: "C", keyCode: "C", x: 4.25, y: 3, width: 1, height: 1 },
  { id: "v", label: "V", keyCode: "V", x: 5.25, y: 3, width: 1, height: 1 },
  { id: "b", label: "B", keyCode: "B", x: 6.25, y: 3, width: 1, height: 1 },
  { id: "n", label: "N", keyCode: "N", x: 7.25, y: 3, width: 1, height: 1 },
  { id: "m", label: "M", keyCode: "M", x: 8.25, y: 3, width: 1, height: 1 },
  { id: "comma", label: ",", keyCode: "Comma", x: 9.25, y: 3, width: 1, height: 1 },
  { id: "period", label: ".", keyCode: "Period", x: 10.25, y: 3, width: 1, height: 1 },
  { id: "slash", label: "/", keyCode: "Slash", x: 11.25, y: 3, width: 1, height: 1 },
  { id: "rightshift", label: "SHIFT", keyCode: "RightShift", x: 12.25, y: 3, width: 2.75, height: 1 },

  // Space row
  { id: "leftctrl", label: "CTRL", keyCode: "LeftCtrl", x: 0, y: 4, width: 1.25, height: 1 },
  { id: "leftwin", label: "WIN", keyCode: "LeftWin", x: 1.25, y: 4, width: 1.25, height: 1 },
  { id: "leftalt", label: "ALT", keyCode: "LeftAlt", x: 2.5, y: 4, width: 1.25, height: 1 },
  { id: "space", label: "SPACE", keyCode: "Space", x: 3.75, y: 4, width: 6.25, height: 1 },
  { id: "rightalt", label: "ALT", keyCode: "RightAlt", x: 10, y: 4, width: 1.25, height: 1 },
  { id: "rightwin", label: "WIN", keyCode: "RightWin", x: 11.25, y: 4, width: 1.25, height: 1 },
  { id: "menu", label: "MENU", keyCode: "Menu", x: 12.5, y: 4, width: 1.25, height: 1 },
  { id: "rightctrl", label: "CTRL", keyCode: "RightCtrl", x: 13.75, y: 4, width: 1.25, height: 1 },
];
