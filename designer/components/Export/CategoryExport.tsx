"use client";

import { useRef, useState } from "react";
import { Download, Loader2, Image as ImageIcon } from "lucide-react";
import { toPng } from "html-to-image";
import JSZip from "jszip";
import { useConfigStore } from "@/lib/store";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { cn } from "@/lib/utils";
import { ansiLayout } from "@/lib/keyboardLayout";
import { getKeyType } from "@/lib/keyTypes";

// Category-filtered keyboard exports.
//
// Each category renders the full keyboard but HIGHLIGHTS only the keys that
// fall in that category (in cyan), with all other keys dimmed and showing
// their full mapping data (base + each layer output) as multi-line labels.
// The visual convention follows the keycap-reference style used by community
// layouts: each category shows the same full layout but with different keys
// highlighted (letters.png highlights all Colemak letters, numbers.png
// highlights the layer keys mapped to digits, etc.).
//
// Categories ask "which keys produce a member of this category in ANY layer
// or via the BASE remap?" — so e.g. Numbers includes both physical 1-0 and
// any layer-mapped digit (like LAlt+I → 4).

type ExportCategory = {
  id: string;
  label: string;
  /** Returns true if the given physical key produces anything in this category. */
  match: (ctx: KeyContext) => boolean;
};

type KeyContext = {
  physicalKey: string;
  /** Every value this physical key can produce: base + layer outputs + shift outputs. */
  outputs: string[];
};

const CATEGORIES: ExportCategory[] = [
  { id: "layout", label: "Full layout", match: () => true },
  {
    id: "letters",
    label: "Letters",
    match: (ctx) => ctx.outputs.some((o) => /^[A-Z]$/.test(o)),
  },
  {
    id: "numbers",
    label: "Numbers",
    match: (ctx) => ctx.outputs.some((o) => /^\d$/.test(o)),
  },
  {
    id: "symbols",
    label: "Symbols",
    match: (ctx) =>
      ctx.outputs.some((o) =>
        new Set([
          "Grave", "Minus", "Equals", "LeftBracket", "RightBracket",
          "Backslash", "Semicolon", "Apostrophe", "Comma", "Period", "Slash",
        ]).has(o),
      ),
  },
  {
    id: "directional",
    label: "Directional",
    match: (ctx) =>
      ctx.outputs.some((o) =>
        new Set([
          "Up", "Down", "Left", "Right",
          "Home", "End", "PageUp", "PageDown",
        ]).has(o),
      ),
  },
  {
    id: "functional",
    label: "Functional",
    match: (ctx) =>
      ctx.outputs.some((o) =>
        new Set(["Escape", "Tab", "Enter", "Backspace", "Delete", "Insert"]).has(o),
      ),
  },
  {
    id: "return",
    label: "Return",
    match: (ctx) => ctx.outputs.some((o) => o === "Enter"),
  },
];

type CategoryExportProps = {
  isOpen: boolean;
  onClose: () => void;
};

export function CategoryExport({ isOpen, onClose }: CategoryExportProps) {
  const { config } = useConfigStore();
  const [activeCategory, setActiveCategory] = useState<string>("layout");
  const [exporting, setExporting] = useState<string | null>(null);
  const exportRef = useRef<HTMLDivElement>(null);

  // Build a context for every key in the layout so categories can be matched.
  const keyContexts = new Map<string, KeyContext>();
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
    keyContexts.set(k.keyCode, { physicalKey: k.keyCode, outputs });
  });

  const current = CATEGORIES.find((c) => c.id === activeCategory) ?? CATEGORIES[0];

  const exportOne = async (category: ExportCategory) => {
    if (!exportRef.current) return;
    // Switch to the requested category, wait a tick for re-render, then snapshot.
    setActiveCategory(category.id);
    setExporting(category.id);
    await new Promise((r) => setTimeout(r, 100));
    try {
      const dataUrl = await toPng(exportRef.current, {
        pixelRatio: 2,
        backgroundColor: "#0a0a0a",
      });
      const a = document.createElement("a");
      a.href = dataUrl;
      a.download = `${config.name || "keyflow"}-${category.id}.png`;
      a.click();
    } finally {
      setExporting(null);
    }
  };

  const exportAll = async () => {
    if (!exportRef.current) return;
    const zip = new JSZip();
    setExporting("all");
    try {
      for (const cat of CATEGORIES) {
        setActiveCategory(cat.id);
        await new Promise((r) => setTimeout(r, 150));
        const dataUrl = await toPng(exportRef.current, {
          pixelRatio: 2,
          backgroundColor: "#0a0a0a",
        });
        const base64 = dataUrl.split(",")[1];
        zip.file(`${config.name || "keyflow"}-${cat.id}.png`, base64, { base64: true });
      }
      const blob = await zip.generateAsync({ type: "blob" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = `${config.name || "keyflow"}-categories.zip`;
      a.click();
      URL.revokeObjectURL(url);
    } finally {
      setExporting(null);
    }
  };

  return (
    <Dialog open={isOpen} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-5xl">
        <DialogHeader>
          <DialogTitle>Export keyboard layout images</DialogTitle>
          <DialogDescription>
            Renders the full keyboard with cyan highlights on keys matching each category.
            Each key shows BASE + every layer output stacked.
          </DialogDescription>
        </DialogHeader>

        <Tabs value={activeCategory} onValueChange={setActiveCategory} className="space-y-4">
          <TabsList className="grid grid-cols-7 w-full">
            {CATEGORIES.map((cat) => (
              <TabsTrigger key={cat.id} value={cat.id} className="text-xs">
                {cat.label}
              </TabsTrigger>
            ))}
          </TabsList>

          <div
            ref={exportRef}
            className="rounded-md overflow-auto p-6"
            style={{ background: "#0a0a0a" }}
          >
            <ExportKeyboard
              keyContexts={keyContexts}
              isHighlighted={(keyCode) => {
                const ctx = keyContexts.get(keyCode);
                if (!ctx) return false;
                return current.match(ctx);
              }}
              config={config}
            />
          </div>
        </Tabs>

        <div className="flex items-center justify-between">
          <span className="text-xs text-muted-foreground">
            Highlighted:{" "}
            <Badge variant="secondary">
              {
                ansiLayout.filter((k) => {
                  const ctx = keyContexts.get(k.keyCode);
                  return ctx && current.match(ctx);
                }).length
              }
            </Badge>{" "}
            of {ansiLayout.length} keys
          </span>
          <div className="flex items-center gap-2">
            <Button
              variant="outline"
              size="sm"
              onClick={() => exportOne(current)}
              disabled={!!exporting}
            >
              {exporting === current.id ? (
                <Loader2 className="h-4 w-4 mr-1.5 animate-spin" />
              ) : (
                <ImageIcon className="h-4 w-4 mr-1.5" />
              )}
              Download this PNG
            </Button>
            <Button size="sm" onClick={exportAll} disabled={!!exporting}>
              {exporting === "all" ? (
                <Loader2 className="h-4 w-4 mr-1.5 animate-spin" />
              ) : (
                <Download className="h-4 w-4 mr-1.5" />
              )}
              Download all (.zip)
            </Button>
          </div>
        </div>
      </DialogContent>
    </Dialog>
  );
}

// ─── The render target for export ──────────────────────────────────────────

type ExportKeyboardProps = {
  keyContexts: Map<string, KeyContext>;
  isHighlighted: (keyCode: string) => boolean;
  config: ReturnType<typeof useConfigStore.getState>["config"];
};

function ExportKeyboard({ keyContexts, isHighlighted, config }: ExportKeyboardProps) {
  const scale = 64;
  const width = 15 * scale;
  const height = 5 * scale;

  return (
    <div
      className="relative font-mono"
      style={{ width: `${width}px`, height: `${height}px` }}
    >
      {ansiLayout.map((keyData) => {
        const highlighted = isHighlighted(keyData.keyCode);
        const ctx = keyContexts.get(keyData.keyCode);
        const labels = buildLabels(keyData, config);
        const keyType = getKeyType(
          // Use the first layer output for type, or fall back to base or physical
          ctx?.outputs[0] ?? keyData.keyCode,
        );
        const _ = keyType; // keep type info available; visual is binary highlighted vs not

        return (
          <div
            key={keyData.id}
            className="absolute rounded-md flex flex-col items-stretch justify-between p-1 overflow-hidden text-[10px] leading-tight"
            style={{
              left: `${keyData.x * scale}px`,
              top: `${keyData.y * scale}px`,
              width: `${keyData.width * scale - 4}px`,
              height: `${keyData.height * scale - 4}px`,
              background: highlighted ? "#5DADE2" : "#2D3540",
              color: highlighted ? "#0a0a0a" : "#fff",
              border: `1px solid ${highlighted ? "#7AC0E8" : "#1c2128"}`,
            }}
          >
            {/* Top: layer outputs (if any) */}
            <div className="flex justify-between gap-0.5">
              <span
                className="opacity-90 truncate font-semibold"
                style={{ fontSize: "10px" }}
              >
                {labels.topLeft || ""}
              </span>
              <span
                className="opacity-90 truncate text-right"
                style={{ fontSize: "9px" }}
              >
                {labels.topRight || ""}
              </span>
            </div>
            {/* Bottom: physical key + base output */}
            <div className="flex justify-between items-end gap-0.5">
              <span className="truncate font-semibold" style={{ fontSize: "12px" }}>
                {labels.bottomLeft || ""}
              </span>
              <span className="truncate opacity-70" style={{ fontSize: "9px" }}>
                {labels.bottomRight || ""}
              </span>
            </div>
          </div>
        );
      })}
    </div>
  );
}

type KeyLabels = {
  topLeft?: string;   // First layer output (e.g. RAlt layer)
  topRight?: string;  // Second layer output (e.g. LAlt layer)
  bottomLeft: string; // Base remap output (or physical label if no remap)
  bottomRight?: string; // Shift mapping output (e.g. ⇧+)
};

function buildLabels(
  keyData: (typeof ansiLayout)[number],
  config: ReturnType<typeof useConfigStore.getState>["config"],
): KeyLabels {
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers ?? [];

  const layer0Output = layers[0]?.mappings?.[keyData.keyCode];
  const layer1Output = layers[1]?.mappings?.[keyData.keyCode];

  // Show a shift mapping from any layer (prefer layer 1's, fall back to layer 0)
  const shiftFromLayer1 = layers[1]?.shiftMappings?.find((m) => m.key === keyData.keyCode);
  const shiftFromLayer0 = layers[0]?.shiftMappings?.find((m) => m.key === keyData.keyCode);
  const shiftMapping = shiftFromLayer1 || shiftFromLayer0;

  return {
    topLeft: layer0Output ? compactLabel(layer0Output) : undefined,
    topRight: layer1Output
      ? compactLabel(layer1Output)
      : shiftMapping
      ? `⇧${compactLabel(shiftMapping.output)}`
      : undefined,
    bottomLeft: baseMapping ? compactLabel(baseMapping) : compactLabel(keyData.label),
    bottomRight: undefined, // Reserved
  };
}

// Shrink long key names so they fit on a 60-px-wide keycap face.
function compactLabel(label: string): string {
  const short: Record<string, string> = {
    LeftShift: "L⇧",
    RightShift: "R⇧",
    LeftCtrl: "L⌃",
    RightCtrl: "R⌃",
    LeftAlt: "L⌥",
    RightAlt: "R⌥",
    LeftWin: "L⊞",
    RightWin: "R⊞",
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
    Space: "␣",
  };
  return short[label] || label;
}
