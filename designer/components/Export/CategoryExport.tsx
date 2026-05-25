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
import { ansiLayout } from "@/lib/keyboardLayout";
import {
  ExportKeyboard,
  buildKeyContexts,
  type KeyContext,
} from "@/components/Export/ExportKeyboard";

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
  const keyContexts = buildKeyContexts(config);

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
