"use client";

import { useState } from "react";
import { Trash2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";

type KeyCategory = "basic" | "modifiers" | "navigation" | "numbers" | "symbols" | "shifted";

type PickerKey = {
  label: string;
  code: string;
  isShifted?: boolean;
};

const KEYS_BY_CATEGORY: Record<KeyCategory, PickerKey[]> = {
  basic: "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("").map((c) => ({ label: c, code: c })),
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
    { label: "Esc", code: "Escape" },
    { label: "Tab", code: "Tab" },
    { label: "Enter", code: "Enter" },
    { label: "Space", code: "Space" },
    { label: "Bksp", code: "Backspace" },
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
  numbers: "0123456789".split("").map((c) => ({ label: c, code: c })),
  symbols: [
    { label: "`", code: "Grave" },
    { label: "-", code: "Minus" },
    { label: "=", code: "Equals" },
    { label: "[", code: "LeftBracket" },
    { label: "]", code: "RightBracket" },
    { label: "\\", code: "Backslash" },
    { label: ";", code: "Semicolon" },
    { label: "'", code: "Apostrophe" },
    { label: ",", code: "Comma" },
    { label: ".", code: "Period" },
    { label: "/", code: "Slash" },
  ],
  shifted: [
    { label: "~", code: "Grave", isShifted: true },
    { label: "!", code: "1", isShifted: true },
    { label: "@", code: "2", isShifted: true },
    { label: "#", code: "3", isShifted: true },
    { label: "$", code: "4", isShifted: true },
    { label: "%", code: "5", isShifted: true },
    { label: "^", code: "6", isShifted: true },
    { label: "&", code: "7", isShifted: true },
    { label: "*", code: "8", isShifted: true },
    { label: "(", code: "9", isShifted: true },
    { label: ")", code: "0", isShifted: true },
    { label: "_", code: "Minus", isShifted: true },
    { label: "+", code: "Equals", isShifted: true },
    { label: "{", code: "LeftBracket", isShifted: true },
    { label: "}", code: "RightBracket", isShifted: true },
    { label: "|", code: "Backslash", isShifted: true },
    { label: ":", code: "Semicolon", isShifted: true },
    { label: '"', code: "Apostrophe", isShifted: true },
    { label: "<", code: "Comma", isShifted: true },
    { label: ">", code: "Period", isShifted: true },
    { label: "?", code: "Slash", isShifted: true },
  ],
};

const CATEGORIES: { id: KeyCategory; label: string }[] = [
  { id: "basic", label: "A-Z" },
  { id: "numbers", label: "0-9" },
  { id: "symbols", label: "Sym" },
  { id: "shifted", label: "Shift+" },
  { id: "modifiers", label: "Mod" },
  { id: "navigation", label: "Nav" },
];

export function KeyPicker() {
  const [category, setCategory] = useState<KeyCategory>("basic");
  const {
    config,
    selectedKey,
    currentLayerIndex,
    setKeyMapping,
    clearKeyMapping,
    setShiftMapping,
    clearShiftMapping,
  } = useConfigStore();

  if (!selectedKey) {
    return (
      <div className="rounded-lg border border-dashed border-border bg-card/30 px-6 py-8 text-center font-mono">
        <p className="text-xs uppercase tracking-widest text-muted-foreground">
          Click a key above to assign a mapping
        </p>
      </div>
    );
  }

  const inLayer = currentLayerIndex >= 0;
  const layer = inLayer ? config.layers?.[currentLayerIndex] : undefined;

  // What's bound to the selected key right now?
  const currentBase = config.remapping?.[selectedKey];
  const currentLayer = layer?.mappings?.[selectedKey];
  const currentShift = layer?.shiftMappings?.find((m) => m.key === selectedKey);
  const currentTarget = inLayer ? currentLayer : currentBase;

  const handleAssign = (code: string, isShifted = false) => {
    if (isShifted && inLayer) {
      setShiftMapping(currentLayerIndex, selectedKey, code);
    } else {
      setKeyMapping(currentLayerIndex, selectedKey, code);
    }
  };

  const handleClearAll = () => {
    if (!currentTarget && !currentShift) return;
    if (
      !confirm(
        `Clear mappings for ${selectedKey}?\n\n` +
          (currentTarget ? `• ${selectedKey} → ${currentTarget}\n` : "") +
          (currentShift ? `• ${selectedKey} → Shift+${currentShift.output}\n` : ""),
      )
    )
      return;
    if (currentTarget) clearKeyMapping(currentLayerIndex, selectedKey);
    if (currentShift) clearShiftMapping(currentLayerIndex, selectedKey);
  };

  return (
    <div className="rounded-lg border border-border bg-card overflow-hidden font-mono">
      {/* Status strip — what we're editing right now */}
      <div className="flex items-center justify-between px-3 py-2 border-b border-border bg-muted/30 text-xs">
        <div className="flex items-center gap-3">
          <span className="uppercase tracking-widest text-muted-foreground">
            Editing
          </span>
          <span className="font-semibold text-primary text-sm">{selectedKey}</span>
          <span className="text-muted-foreground">·</span>
          <span className="uppercase tracking-wider text-muted-foreground">
            {inLayer ? layer?.name || `Layer ${currentLayerIndex + 1}` : "Base"}
          </span>
          {currentTarget && (
            <>
              <span className="text-muted-foreground">→</span>
              <span className="text-mapped font-semibold">{currentTarget}</span>
            </>
          )}
          {currentShift && (
            <span className="text-mapped font-semibold">
              <span className="text-muted-foreground">+ ⇧</span> {currentShift.output}
            </span>
          )}
        </div>
        <button
          onClick={handleClearAll}
          disabled={!currentTarget && !currentShift}
          className={cn(
            "inline-flex items-center gap-1 h-7 px-2 rounded-md text-xs uppercase tracking-wider",
            "border border-destructive/40 text-destructive",
            currentTarget || currentShift
              ? "hover:bg-destructive hover:text-destructive-foreground transition-colors"
              : "opacity-30 cursor-not-allowed",
          )}
        >
          <Trash2 size={12} />
          Clear
        </button>
      </div>

      {/* Category tabs */}
      <div className="flex items-center gap-0.5 px-2 pt-2">
        {CATEGORIES.map((cat) => {
          const isShiftCat = cat.id === "shifted";
          const disabled = isShiftCat && !inLayer;
          return (
            <button
              key={cat.id}
              onClick={() => !disabled && setCategory(cat.id)}
              disabled={disabled}
              title={disabled ? "Shift mappings only available in layers" : undefined}
              className={cn(
                "h-7 px-3 rounded-t-md text-[11px] uppercase tracking-wider font-semibold",
                "border border-b-0 transition-colors",
                disabled && "opacity-30 cursor-not-allowed",
                category === cat.id
                  ? "border-border bg-background text-primary"
                  : "border-transparent text-muted-foreground hover:text-foreground hover:bg-accent/40",
              )}
            >
              {cat.label}
            </button>
          );
        })}
      </div>

      {/* Key grid */}
      <div
        className={cn(
          "p-3 grid gap-1.5 bg-background border-t border-border",
          category === "modifiers"
            ? "grid-cols-[repeat(auto-fill,minmax(72px,1fr))]"
            : category === "navigation"
            ? "grid-cols-[repeat(auto-fill,minmax(58px,1fr))]"
            : "grid-cols-[repeat(auto-fill,minmax(44px,1fr))]",
        )}
      >
        {KEYS_BY_CATEGORY[category].map((k) => {
          const isCurrent =
            (k.isShifted ? currentShift?.output : currentTarget) === k.code &&
            (k.isShifted ? !!currentShift : !!currentTarget);
          return (
            <button
              key={`${k.code}-${k.isShifted ? "s" : "n"}`}
              onClick={() => handleAssign(k.code, k.isShifted)}
              className={cn(
                "key-face h-9 inline-flex items-center justify-center rounded-md",
                "text-sm font-semibold",
                category === "modifiers" && "px-2",
              )}
              data-mapped={isCurrent || undefined}
              data-selected={isCurrent || undefined}
            >
              <span className={cn(k.isShifted && "key-legend-mapped")}>{k.label}</span>
            </button>
          );
        })}
      </div>
    </div>
  );
}
