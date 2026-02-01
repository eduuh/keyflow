// Key type definitions and color schemes

export type KeyType = "letter" | "number" | "modifier" | "navigation" | "symbol" | "function" | "other";

export function getKeyType(keyCode: string): KeyType {
  // Modifiers
  if (keyCode.includes("Ctrl") || keyCode.includes("Shift") || keyCode.includes("Alt") || keyCode.includes("Win") || keyCode === "CapsLock") {
    return "modifier";
  }

  // Navigation
  if (["Up", "Down", "Left", "Right", "Home", "End", "PageUp", "PageDown", "Insert", "Delete"].includes(keyCode)) {
    return "navigation";
  }

  // Letters
  if (/^[A-Z]$/.test(keyCode)) {
    return "letter";
  }

  // Numbers
  if (/^[0-9]$/.test(keyCode)) {
    return "number";
  }

  // Function keys
  if (/^F[0-9]+$/.test(keyCode)) {
    return "function";
  }

  // Symbols
  if (["Grave", "Minus", "Equals", "LeftBracket", "RightBracket", "Backslash", "Semicolon", "Apostrophe", "Comma", "Period", "Slash"].includes(keyCode)) {
    return "symbol";
  }

  return "other";
}

export function getKeyTypeColor(keyType: KeyType): string {
  switch (keyType) {
    case "modifier":
      return "orange";
    case "navigation":
      return "blue";
    case "number":
      return "emerald";
    case "symbol":
      return "violet";
    case "function":
      return "rose";
    default:
      return "slate";
  }
}

export function getKeyTypeClasses(keyType: KeyType, isSelected: boolean, hasMappings: boolean): string {
  const color = getKeyTypeColor(keyType);

  if (isSelected) {
    return `border-${color}-500 bg-${color}-50 dark:bg-${color}-950 ring-2 ring-${color}-500/20 text-${color}-900 dark:text-${color}-100`;
  }

  if (hasMappings) {
    return `border-${color}-300 bg-${color}-50 dark:border-${color}-700 dark:bg-${color}-950/50 text-${color}-700 dark:text-${color}-300`;
  }

  if (keyType === "modifier") {
    return `border-${color}-200 dark:border-${color}-800/30 text-${color}-700 dark:text-${color}-400 hover:border-${color}-300 dark:hover:border-${color}-700`;
  }

  if (keyType === "navigation") {
    return `border-${color}-200 dark:border-${color}-800/30 text-${color}-700 dark:text-${color}-400 hover:border-${color}-300 dark:hover:border-${color}-700`;
  }

  return "border-border text-foreground hover:border-primary/50";
}
