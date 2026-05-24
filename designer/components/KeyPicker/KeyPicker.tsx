"use client";

import { useState } from "react";
import { Trash2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { Card, CardContent, CardHeader } from "@/components/ui/card";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { getKeyType, KEY_TYPE_STYLES } from "@/lib/keyTypes";
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
      <Card className="w-full">
        <CardContent className="py-12 text-center">
          <p className="text-base text-muted-foreground">
            Click any key on the keyboard above to assign a mapping.
          </p>
        </CardContent>
      </Card>
    );
  }

  const inLayer = currentLayerIndex >= 0;
  const layer = inLayer ? config.layers?.[currentLayerIndex] : undefined;
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

  const handleClear = () => {
    if (!currentTarget && !currentShift) return;
    if (
      !confirm(
        `Clear mappings for ${selectedKey}?` +
          (currentTarget ? `\n• → ${currentTarget}` : "") +
          (currentShift ? `\n• → Shift+${currentShift.output}` : ""),
      )
    )
      return;
    if (currentTarget) clearKeyMapping(currentLayerIndex, selectedKey);
    if (currentShift) clearShiftMapping(currentLayerIndex, selectedKey);
  };

  return (
    <Card data-tour="picker" className="w-full">
      <CardHeader className="flex-row items-center justify-between space-y-0 pb-4 border-b">
        <div className="flex items-center gap-3 text-base flex-wrap">
          <span className="text-muted-foreground text-sm uppercase tracking-wider">Editing</span>
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
                {currentTarget}
              </Badge>
            </>
          )}
          {currentShift && (
            <Badge variant="default" className="font-mono text-base px-3 py-1">
              ⇧ {currentShift.output}
            </Badge>
          )}
        </div>
        <Button
          variant="ghost"
          size="default"
          onClick={handleClear}
          disabled={!currentTarget && !currentShift}
          className="text-destructive hover:text-destructive"
        >
          <Trash2 className="h-4 w-4 mr-1.5" />
          Clear
        </Button>
      </CardHeader>
      <CardContent className="pt-6">
        <Tabs value={category} onValueChange={(v) => setCategory(v as KeyCategory)}>
          <TabsList className="grid grid-cols-6 w-full mb-4 h-11">
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
              category === "modifiers"
                ? "grid-cols-[repeat(auto-fill,minmax(96px,1fr))]"
                : category === "navigation"
                ? "grid-cols-[repeat(auto-fill,minmax(76px,1fr))]"
                : "grid-cols-[repeat(auto-fill,minmax(60px,1fr))]",
            )}
          >
            {KEYS[category].map((k) => {
              const isCurrent =
                (k.isShifted ? currentShift?.output : currentTarget) === k.code &&
                (k.isShifted ? !!currentShift : !!currentTarget);
              const typeStyle = KEY_TYPE_STYLES[getKeyType(k.code)];
              return (
                <button
                  key={`${k.code}-${k.isShifted ? "s" : "n"}`}
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
      </CardContent>
    </Card>
  );
}
