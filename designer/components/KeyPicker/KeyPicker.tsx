"use client";

import { useState } from "react";
import { Trash2 } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { KEY_GROUPS, canonicalKey } from "@/lib/allKeys";
import { keyMapping } from "@/lib/config";
import { Card, CardContent, CardHeader } from "@/components/ui/card";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { getKeyType, KEY_TYPE_STYLES } from "@/lib/keyTypes";
import { displayLabel, SHIFTED_KEYS } from "@/lib/keyDisplay";
import { cn } from "@/lib/utils";

type KeyCategory = "basic" | "modifiers" | "navigation" | "numbers" | "symbols" | "shifted";

type PickerKey = { label: string; code: string; isShifted?: boolean };

const KEYS: Record<KeyCategory, PickerKey[]> = {
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
  shifted: SHIFTED_KEYS.map((k) => ({ ...k, isShifted: true })),
};

export function KeyPicker() {
  const [category, setCategory] = useState<KeyCategory>("basic");
  const {
    config: document,
    platform,
    selectedKey,
    currentLayerIndex,
    setKeyMapping,
    clearKeyMapping,
    setShiftMapping,
    clearShiftMapping,
    removeNoModCombo,
  } = useConfigStore();
  const config = useEditingConfig();

  if (!selectedKey) {
    return (
      <Card data-tour="picker" className="w-full">
        <CardContent className="py-10 text-center">
          <p className="text-sm font-medium mb-3">Key mapping</p>
          <p className="text-sm text-muted-foreground">
            Select a key on the canvas or choose another physical key to edit its output.
          </p>
        </CardContent>
      </Card>
    );
  }

  const inLayer = currentLayerIndex >= 0;
  const layer = inLayer ? config.layers?.[currentLayerIndex] : undefined;
  const currentBase = keyMapping(config.remapping, selectedKey);
  const currentLayer = keyMapping(layer?.mappings, selectedKey);
  const currentShift = layer?.shiftMappings?.find((m) => canonicalKey(m.key) === canonicalKey(selectedKey));
  const combo = !inLayer ? config.noModCombos?.find((m) => canonicalKey(m.key) === canonicalKey(selectedKey)) : undefined;
  const currentTarget = inLayer ? currentLayer : currentBase;
  const activeCategory = !inLayer && category === "shifted" ? "basic" : category;
  const ownBase = platform === "shared" || !!keyMapping(document.platformOverrides?.[platform]?.remapping, selectedKey);
  const canClear = !!currentShift || !!combo || (!!currentTarget && (inLayer || ownBase));

  const handleAssign = (code: string, isShifted = false) => {
    if (isShifted && inLayer) {
      setShiftMapping(currentLayerIndex, selectedKey, code);
    } else {
      if (combo) removeNoModCombo(combo.key);
      setKeyMapping(currentLayerIndex, selectedKey, code);
    }
  };

  const handleClear = () => {
    if (!canClear) return;
    if (
      !confirm(
        `${!inLayer && platform !== "shared" ? "Revert" : "Clear"} mappings for ${selectedKey}?` +
          (currentTarget ? `\n• → ${currentTarget}` : "") +
          (currentShift ? `\n• → Shift+${currentShift.output}` : ""),
      )
    )
      return;
    if (currentTarget && (inLayer || ownBase)) clearKeyMapping(currentLayerIndex, selectedKey);
    if (currentShift) clearShiftMapping(currentLayerIndex, selectedKey);
    if (combo) removeNoModCombo(combo.key);
  };

  return (
    <Card data-tour="picker" className="w-full min-w-0 xl:sticky xl:top-24">
      <CardHeader className="space-y-3 p-4 border-b">
        <div className="flex items-center gap-2 text-sm flex-wrap">
          <span className="text-muted-foreground text-sm">Editing</span>
          <Badge variant="secondary" className="font-mono text-base px-3 py-1">
            {selectedKey}
          </Badge>
          <span className="text-muted-foreground">·</span>
          <span className="font-medium">
            {inLayer ? layer?.name || `Layer ${currentLayerIndex + 1}` : "BASE"}
          </span>
          {currentTarget && (
            <>
              <span className="text-muted-foreground text-xl">→</span>
              <Badge variant="default" className="font-mono text-base px-3 py-1">
                {displayLabel(currentTarget)}
              </Badge>
            </>
          )}
          {(currentShift || combo) && (
            <Badge variant="default" className="font-mono text-base px-3 py-1">
              {(currentShift ? currentShift.shift !== false : combo?.shift) ? "⇧ " : ""}
              {displayLabel((currentShift ?? combo)!.output)}
            </Badge>
          )}
        </div>
        <Button
          variant="ghost"
          size="default"
          onClick={handleClear}
          disabled={!canClear}
          className="text-destructive hover:text-destructive"
        >
          <Trash2 className="h-4 w-4 mr-1.5" />
          {!inLayer && platform !== "shared" ? "Use Shared / clear combo" : "Clear"}
        </Button>
      </CardHeader>
      <CardContent className="p-4 space-y-4">
        <Tabs value={activeCategory} onValueChange={(v) => setCategory(v as KeyCategory)}>
          <TabsList className="grid grid-cols-3 sm:grid-cols-6 xl:grid-cols-3 w-full mb-4 h-auto gap-1">
            <TabsTrigger value="basic" className="text-sm">A-Z</TabsTrigger>
            <TabsTrigger value="numbers" className="text-sm">0-9</TabsTrigger>
            <TabsTrigger value="symbols" className="text-sm">Sym</TabsTrigger>
            <TabsTrigger value="shifted" disabled={!inLayer} className="text-sm" data-tour="picker-shift">
              Shift+
            </TabsTrigger>
            <TabsTrigger value="modifiers" className="text-sm">Mod</TabsTrigger>
            <TabsTrigger value="navigation" className="text-sm">Nav</TabsTrigger>
          </TabsList>

          <div
            className={cn(
              "grid gap-2",
              activeCategory === "modifiers"
                ? "grid-cols-[repeat(auto-fill,minmax(96px,1fr))]"
                : activeCategory === "navigation"
                ? "grid-cols-[repeat(auto-fill,minmax(76px,1fr))]"
                : "grid-cols-[repeat(auto-fill,minmax(60px,1fr))]",
            )}
          >
            {KEYS[activeCategory].map((k) => {
              const isCurrent =
                canonicalKey((k.isShifted ? currentShift?.output : currentTarget) ?? "") === k.code &&
                (k.isShifted ? !!currentShift && currentShift.shift !== false : !!currentTarget);
              const typeStyle = KEY_TYPE_STYLES[getKeyType(k.code)];
              return (
                <button
                  key={`${k.code}-${k.isShifted ? "s" : "n"}`}
                  type="button"
                  aria-label={`Map to ${k.isShifted ? "Shift+" : ""}${k.code}`}
                  aria-pressed={isCurrent}
                  onClick={() => handleAssign(k.code, k.isShifted)}
                  className={cn(
                    "h-12 inline-flex items-center justify-center rounded-md border-2 font-mono text-base font-medium",
                    "transition-colors active:scale-95",
                    typeStyle.tint,
                    isCurrent && "ring-2 ring-primary ring-offset-2 ring-offset-background",
                    k.isShifted && "italic",
                  )}
                >
                  {k.isShifted && <span className="opacity-60 mr-0.5 text-sm">⇧</span>}
                  {k.label}
                </button>
              );
            })}
          </div>
        </Tabs>
        <div className="space-y-2 border-t pt-3">
          <label htmlFor="all-outputs" className="text-xs font-medium">All output keys</label>
          <select id="all-outputs" value="" onChange={(event) => event.target.value && handleAssign(event.target.value)}
            className="w-full border rounded-md bg-background p-2 text-sm">
            <option value="">Function, numpad, media, block key...</option>
            {KEY_GROUPS.map((group) => <optgroup key={group.group} label={group.group}>
              {group.keys.map((key) => <option key={key.code} value={key.code}>{key.label}</option>)}
            </optgroup>)}
          </select>
          <p className="text-xs text-muted-foreground" aria-live="polite">
            {!inLayer && platform !== "shared" && !ownBase ? "Inherited from Shared. Assign an output to override it." :
              currentTarget || currentShift || combo ? "Binding saved automatically in this environment." : "No mapping. The key passes through."}
          </p>
        </div>
      </CardContent>
    </Card>
  );
}
