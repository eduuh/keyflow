"use client";

import { useState } from "react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";

type KeyCategory = "basic" | "modifiers" | "navigation" | "numbers" | "symbols" | "shifted";

const keysByCategory: Record<KeyCategory, { label: string; code: string; isShifted?: boolean; displayLabel?: string }[]> = {
  basic: [
    { label: "A", code: "A" },
    { label: "B", code: "B" },
    { label: "C", code: "C" },
    { label: "D", code: "D" },
    { label: "E", code: "E" },
    { label: "F", code: "F" },
    { label: "G", code: "G" },
    { label: "H", code: "H" },
    { label: "I", code: "I" },
    { label: "J", code: "J" },
    { label: "K", code: "K" },
    { label: "L", code: "L" },
    { label: "M", code: "M" },
    { label: "N", code: "N" },
    { label: "O", code: "O" },
    { label: "P", code: "P" },
    { label: "Q", code: "Q" },
    { label: "R", code: "R" },
    { label: "S", code: "S" },
    { label: "T", code: "T" },
    { label: "U", code: "U" },
    { label: "V", code: "V" },
    { label: "W", code: "W" },
    { label: "X", code: "X" },
    { label: "Y", code: "Y" },
    { label: "Z", code: "Z" },
  ],
  modifiers: [
    { label: "LCtrl", code: "LeftCtrl" },
    { label: "RCtrl", code: "RightCtrl" },
    { label: "LShift", code: "LeftShift" },
    { label: "RShift", code: "RightShift" },
    { label: "LAlt", code: "LeftAlt" },
    { label: "RAlt", code: "RightAlt" },
    { label: "LWin", code: "LeftWin" },
    { label: "RWin", code: "RightWin" },
    { label: "Caps", code: "CapsLock" },
  ],
  navigation: [
    { label: "↑", code: "Up" },
    { label: "↓", code: "Down" },
    { label: "←", code: "Left" },
    { label: "→", code: "Right" },
    { label: "Home", code: "Home" },
    { label: "End", code: "End" },
    { label: "PgUp", code: "PageUp" },
    { label: "PgDn", code: "PageDown" },
    { label: "Ins", code: "Insert" },
    { label: "Del", code: "Delete" },
  ],
  numbers: [
    { label: "0", code: "0", displayLabel: "0" },
    { label: "1", code: "1", displayLabel: "1" },
    { label: "2", code: "2", displayLabel: "2" },
    { label: "3", code: "3", displayLabel: "3" },
    { label: "4", code: "4", displayLabel: "4" },
    { label: "5", code: "5", displayLabel: "5" },
    { label: "6", code: "6", displayLabel: "6" },
    { label: "7", code: "7", displayLabel: "7" },
    { label: "8", code: "8", displayLabel: "8" },
    { label: "9", code: "9", displayLabel: "9" },
  ],
  symbols: [
    { label: "`", code: "Grave", displayLabel: "`" },
    { label: "-", code: "Minus", displayLabel: "-" },
    { label: "=", code: "Equals", displayLabel: "=" },
    { label: "[", code: "LeftBracket", displayLabel: "[" },
    { label: "]", code: "RightBracket", displayLabel: "]" },
    { label: "\\", code: "Backslash", displayLabel: "\\" },
    { label: ";", code: "Semicolon", displayLabel: ";" },
    { label: "'", code: "Apostrophe", displayLabel: "'" },
    { label: ",", code: "Comma", displayLabel: "," },
    { label: ".", code: "Period", displayLabel: "." },
    { label: "/", code: "Slash", displayLabel: "/" },
  ],
  shifted: [
    { label: "~", code: "Grave", isShifted: true, displayLabel: "~" },
    { label: "!", code: "1", isShifted: true, displayLabel: "!" },
    { label: "@", code: "2", isShifted: true, displayLabel: "@" },
    { label: "#", code: "3", isShifted: true, displayLabel: "#" },
    { label: "$", code: "4", isShifted: true, displayLabel: "$" },
    { label: "%", code: "5", isShifted: true, displayLabel: "%" },
    { label: "^", code: "6", isShifted: true, displayLabel: "^" },
    { label: "&", code: "7", isShifted: true, displayLabel: "&" },
    { label: "*", code: "8", isShifted: true, displayLabel: "*" },
    { label: "(", code: "9", isShifted: true, displayLabel: "(" },
    { label: ")", code: "0", isShifted: true, displayLabel: ")" },
    { label: "_", code: "Minus", isShifted: true, displayLabel: "_" },
    { label: "+", code: "Equals", isShifted: true, displayLabel: "+" },
    { label: "{", code: "LeftBracket", isShifted: true, displayLabel: "{" },
    { label: "}", code: "RightBracket", isShifted: true, displayLabel: "}" },
    { label: "|", code: "Backslash", isShifted: true, displayLabel: "|" },
    { label: ":", code: "Semicolon", isShifted: true, displayLabel: ":" },
    { label: '"', code: "Apostrophe", isShifted: true, displayLabel: '"' },
    { label: "<", code: "Comma", isShifted: true, displayLabel: "<" },
    { label: ">", code: "Period", isShifted: true, displayLabel: ">" },
    { label: "?", code: "Slash", isShifted: true, displayLabel: "?" },
  ],
};

export function KeyPicker() {
  const [activeCategory, setActiveCategory] = useState<KeyCategory>("basic");
  const [shiftMode, setShiftMode] = useState(false);
  const { config, selectedKey, currentLayerIndex, setKeyMapping, clearKeyMapping, setShiftMapping, clearShiftMapping } =
    useConfigStore();

  if (!selectedKey) {
    return (
      <div className="rounded-lg border bg-card p-4 text-center shadow-sm">
        <p className="text-sm text-muted-foreground">
          Select a key to assign a mapping
        </p>
      </div>
    );
  }

  const handleKeySelect = (targetCode: string, isShiftedSymbol: boolean = false) => {
    if ((shiftMode || isShiftedSymbol) && currentLayerIndex !== -1) {
      // Create shift mapping (only in layers, not BASE)
      // Shifted symbols from the "Shifted" tab automatically use shift mappings
      setShiftMapping(currentLayerIndex, selectedKey, targetCode);
    } else {
      // Create regular mapping
      setKeyMapping(currentLayerIndex, selectedKey, targetCode);
    }
  };

  const handleClear = () => {
    // Check what mappings exist
    let hasRegularMapping = false;
    let hasShiftMapping = false;
    let regularMappingValue = "";
    let shiftMappingValue = "";

    if (currentLayerIndex === -1) {
      // BASE layer - only regular mappings
      hasRegularMapping = !!config.remapping?.[selectedKey];
      regularMappingValue = config.remapping?.[selectedKey] || "";
    } else {
      // Layer - can have both regular and shift mappings
      const layer = config.layers?.[currentLayerIndex];
      hasRegularMapping = !!layer?.mappings?.[selectedKey];
      regularMappingValue = layer?.mappings?.[selectedKey] || "";

      hasShiftMapping = !!layer?.shiftMappings?.find((m) => m.key === selectedKey);
      const shiftMapping = layer?.shiftMappings?.find((m) => m.key === selectedKey);
      shiftMappingValue = shiftMapping?.output || "";
    }

    // Build confirmation message
    if (!hasRegularMapping && !hasShiftMapping) {
      alert("No mappings to clear for this key.");
      return;
    }

    let message = "Clear the following mappings?\n\n";
    if (hasRegularMapping) {
      message += `• Regular: ${selectedKey} → ${regularMappingValue}\n`;
    }
    if (hasShiftMapping) {
      message += `• Shift: ${selectedKey} → ⇧${shiftMappingValue}\n`;
    }

    if (!confirm(message)) {
      return;
    }

    // Clear the mappings
    if (hasRegularMapping) {
      clearKeyMapping(currentLayerIndex, selectedKey);
    }
    if (hasShiftMapping) {
      clearShiftMapping(currentLayerIndex, selectedKey);
    }
  };

  const categories: { id: KeyCategory; label: string }[] = [
    { id: "basic", label: "Basic" },
    { id: "modifiers", label: "Modifiers" },
    { id: "navigation", label: "Navigation" },
    { id: "numbers", label: "Numbers" },
    { id: "symbols", label: "Symbols" },
    { id: "shifted", label: "Shifted" },
  ];

  // Check if we're in a layer (shift mappings only work in layers)
  const canUseShiftMode = currentLayerIndex !== -1;

  return (
    <div className="rounded-lg border bg-card p-3 shadow-sm">
      {/* Header */}
      <div className="flex items-center justify-between mb-2">
        <div className="flex items-center gap-3">
          <div className="flex items-center gap-2">
            <span className="text-xs font-semibold text-muted-foreground">Map:</span>
            <span className="text-sm font-mono font-semibold text-primary">{selectedKey}</span>
            <span className="text-xs text-muted-foreground font-mono">
              ({currentLayerIndex === -1 ? "BASE" : `L${currentLayerIndex + 1}`})
            </span>
          </div>
          {canUseShiftMode && (
            <div className="flex items-center gap-1.5 pl-3 border-l">
              <span className="text-xs text-muted-foreground">+Shift:</span>
              <button
                role="switch"
                aria-checked={shiftMode}
                onClick={() => setShiftMode(!shiftMode)}
                className={cn(
                  "inline-flex h-4 w-7 shrink-0 cursor-pointer items-center rounded-full border-2 border-transparent shadow-sm transition-colors",
                  shiftMode ? "bg-primary" : "bg-input"
                )}
              >
                <span
                  className={cn(
                    "pointer-events-none block h-3 w-3 rounded-full bg-background shadow-lg ring-0 transition-transform",
                    shiftMode ? "translate-x-3" : "translate-x-0"
                  )}
                />
              </button>
            </div>
          )}
        </div>
        <button
          onClick={handleClear}
          className="inline-flex items-center justify-center rounded-md text-xs font-medium transition-colors bg-destructive text-destructive-foreground hover:bg-destructive/90 h-7 px-2"
        >
          Clear
        </button>
      </div>

      {/* Category Tabs */}
      <div className="inline-flex h-8 items-center justify-center rounded-lg bg-muted p-1 text-muted-foreground mb-2 w-full">
        {categories.map((cat) => (
          <button
            key={cat.id}
            onClick={() => setActiveCategory(cat.id)}
            className={cn(
              "inline-flex items-center justify-center whitespace-nowrap rounded-md px-2 py-1 text-xs font-medium transition-all flex-1",
              activeCategory === cat.id
                ? "bg-background text-foreground shadow"
                : "hover:bg-background/50"
            )}
          >
            {cat.label}
          </button>
        ))}
      </div>

      {/* Key Grid */}
      <div className={cn(
        "grid gap-1.5",
        activeCategory === "modifiers"
          ? "grid-cols-[repeat(auto-fill,minmax(70px,1fr))]" // Wider for modifier labels
          : "grid-cols-[repeat(auto-fill,minmax(40px,1fr))]"
      )}>
        {keysByCategory[activeCategory].map((key) => (
          <button
            key={`${key.code}-${key.isShifted ? 'shifted' : 'normal'}`}
            onClick={() => handleKeySelect(key.code, key.isShifted)}
            className={cn(
              "inline-flex items-center justify-center rounded-md text-xs font-semibold transition-colors border relative",
              key.isShifted
                ? "bg-purple-50 dark:bg-purple-950/30 border-purple-300 dark:border-purple-700/50 text-purple-700 dark:text-purple-300 hover:bg-purple-100 dark:hover:bg-purple-900/40"
                : "border-input bg-background hover:bg-accent hover:text-accent-foreground",
              activeCategory === "modifiers"
                ? "h-9 px-2 min-w-[70px]" // Rectangular for modifiers
                : "h-9 w-9" // Square for others
            )}
          >
            {key.isShifted && (
              <span className="absolute top-0.5 right-0.5 text-[8px] opacity-60">⇧</span>
            )}
            {key.displayLabel || key.label}
          </button>
        ))}
      </div>
    </div>
  );
}
