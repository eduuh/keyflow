"use client";

import { useRef, useState } from "react";
import { Download, Loader2, Image as ImageIcon } from "lucide-react";
import { toPng } from "html-to-image";
import JSZip from "jszip";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { PLATFORMS } from "@/lib/config";
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
          "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "~", "_", "+", "{", "}", "|", ":", '"', "<", ">", "?",
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
        new Set(["Escape", "Tab", "Enter", "Backspace", "Delete", "Insert"]).has(o) || /^F\d+$/.test(o),
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
  const { platform } = useConfigStore();
  const config = useEditingConfig();
  const [activeCategory, setActiveCategory] = useState<string>("layout");
  const [exporting, setExporting] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const exportRef = useRef<HTMLDivElement>(null);

  // Build a context for every key in the layout so categories can be matched.
  const keyContexts = buildKeyContexts(config);

  const current = CATEGORIES.find((c) => c.id === activeCategory) ?? CATEGORIES[0];
  const fileName = `${config.name || "keyflow"}-${platform}`;

  const exportOne = async (category: ExportCategory) => {
    if (!exportRef.current) return;
    // Switch to the requested category, wait a tick for re-render, then snapshot.
    setActiveCategory(category.id);
    setExporting(category.id);
    setError(null);
    await new Promise((r) => setTimeout(r, 100));
    try {
      const dataUrl = await toPng(exportRef.current, {
        pixelRatio: 2,
        backgroundColor: "#0a0a0a",
        preferredFontFormat: "woff2",
      });
      const a = document.createElement("a");
      a.href = dataUrl;
      a.download = `${fileName}-${category.id}.png`;
      a.click();
    } catch (error) {
      setError(error instanceof Error ? error.message : "PNG export failed");
    } finally {
      setExporting(null);
    }
  };

  const exportAll = async () => {
    if (!exportRef.current) return;
    const zip = new JSZip();
    setExporting("all");
    setError(null);
    const previousCategory = activeCategory;
    try {
      for (const cat of CATEGORIES) {
        setActiveCategory(cat.id);
        await new Promise((r) => setTimeout(r, 150));
        const dataUrl = await toPng(exportRef.current, {
          pixelRatio: 2,
          backgroundColor: "#0a0a0a",
          preferredFontFormat: "woff2",
        });
        const base64 = dataUrl.split(",")[1];
        zip.file(`${fileName}-${cat.id}.png`, base64, { base64: true });
      }
      const blob = await zip.generateAsync({ type: "blob" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = `${fileName}-categories.zip`;
      a.click();
      URL.revokeObjectURL(url);
    } catch (error) {
      setError(error instanceof Error ? error.message : "ZIP export failed");
    } finally {
      setExporting(null);
      setActiveCategory(previousCategory);
    }
  };

  return (
    <Dialog open={isOpen} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-5xl max-h-[90vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>Export keyboard layout images</DialogTitle>
          <DialogDescription>
            ANSI main block for {platform === "shared" ? "Shared" : PLATFORMS[platform]}, including all layer bindings.
            Cyan highlights keys matching the selected category. JSON export preserves every physical key and environment.
          </DialogDescription>
        </DialogHeader>

        <Tabs value={activeCategory} onValueChange={(category) => !exporting && setActiveCategory(category)} className="space-y-4">
          <TabsList className="flex flex-wrap justify-start h-auto w-full">
            {CATEGORIES.map((cat) => (
              <TabsTrigger key={cat.id} value={cat.id} className="text-xs" disabled={!!exporting}>
                {cat.label}
              </TabsTrigger>
            ))}
          </TabsList>

          <div
            className="rounded-md overflow-auto max-w-full"
            aria-label="Export preview"
            style={{ background: "#0a0a0a" }}
          >
            <div ref={exportRef} className="w-max p-6" style={{ background: "#0a0a0a" }}>
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
          </div>
        </Tabs>
        {error && <p role="alert" className="text-sm text-destructive">{error}</p>}

        <div className="flex flex-wrap gap-3 items-center justify-between">
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
