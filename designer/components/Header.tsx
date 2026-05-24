"use client";

import { useState } from "react";
import { Moon, Sun, Download, Upload, FileJson, Keyboard } from "lucide-react";
import { useTheme } from "./ThemeProvider";
import { useConfigStore } from "@/lib/store";
import { ExportPreviewModal } from "./ExportPreviewModal";
import { cn } from "@/lib/utils";

// Lean header: brand on the left (icon + name + version), action group on the
// right (import, export-json, export-image, theme). No emojis — the icon is a
// Lucide Keyboard glyph that matches the hardware-y aesthetic.
//
// All buttons share a single `chromeBtn` class so the visual language stays
// uniform. The primary action (image export) gets the amber LED treatment.

const chromeBtn = cn(
  "inline-flex items-center justify-center gap-2 h-9 px-3",
  "rounded-md border border-border bg-card/60",
  "text-sm font-medium text-foreground/90",
  "transition-colors",
  "hover:bg-accent hover:text-accent-foreground hover:border-primary/40",
  "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 focus-visible:ring-offset-background",
);

const primaryBtn = cn(
  "inline-flex items-center justify-center gap-2 h-9 px-3",
  "rounded-md bg-primary text-primary-foreground",
  "text-sm font-semibold",
  "transition-colors hover:bg-primary/90",
  "shadow-[0_0_12px_hsl(var(--primary)/0.3)]",
  "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 focus-visible:ring-offset-background",
);

export function Header() {
  const { theme, toggleTheme } = useTheme();
  const { config, setConfig } = useConfigStore();
  const [showExportModal, setShowExportModal] = useState(false);

  const handleExportJSON = () => {
    const json = JSON.stringify(config, null, 2);
    const blob = new Blob([json], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `${config.name || "config"}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleImport = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json";
    input.onchange = (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = (e) => {
        try {
          const json = JSON.parse(e.target?.result as string);

          if (!json.version) {
            alert("Invalid config: Missing 'version' field.\n\nExpected format: \"1.0\"");
            return;
          }
          if (typeof json.version !== "string" || !/^[0-9]+\.[0-9]+$/.test(json.version)) {
            alert("Invalid config: 'version' must be in format X.Y (e.g., \"1.0\")");
            return;
          }

          // Layer shape check. C++ accepts both `trigger` and `triggers`.
          if (json.layers && Array.isArray(json.layers)) {
            for (let i = 0; i < json.layers.length; i++) {
              const layer = json.layers[i];
              const hasTrigger =
                layer.trigger ||
                (Array.isArray(layer.triggers) && layer.triggers.length > 0);
              if (!layer.name || !hasTrigger || !layer.mappings) {
                alert(
                  `Invalid config: Layer ${i + 1} is missing required fields (name, trigger(s), mappings)`,
                );
                return;
              }
            }
          }

          if (json.remapping && typeof json.remapping !== "object") {
            alert("Invalid config: 'remapping' must be an object");
            return;
          }

          // Hand off to store, which normalizes (trigger → triggers, drops obsolete fields).
          setConfig(json);
        } catch (error) {
          alert(
            "Invalid JSON file: " +
              (error instanceof Error ? error.message : "Unknown error"),
          );
        }
      };
      reader.readAsText(file);
    };
    input.click();
  };

  return (
    <header className="border-b border-border bg-background/95 backdrop-blur">
      <div className="px-6 py-3 flex items-center justify-between">
        {/* Brand */}
        <div className="flex items-center gap-3">
          <div
            className="relative flex items-center justify-center h-9 w-9 rounded-md border border-border bg-card"
            aria-hidden
          >
            <Keyboard size={18} className="text-primary" />
            <span
              className="absolute -top-0.5 -right-0.5 w-1.5 h-1.5 rounded-full bg-primary"
              style={{ boxShadow: "0 0 6px hsl(var(--primary))" }}
            />
          </div>
          <div className="leading-tight">
            <h1 className="text-sm font-semibold tracking-wide uppercase font-mono">
              Keyflow Designer
            </h1>
            <p className="text-xs text-muted-foreground font-mono">
              {config.name || "Untitled"} · v{config.version}
            </p>
          </div>
        </div>

        {/* Actions */}
        <div className="flex items-center gap-2">
          <button onClick={handleImport} className={chromeBtn} aria-label="Import config">
            <Upload size={16} />
            <span className="hidden md:inline">Import</span>
          </button>
          <button
            onClick={handleExportJSON}
            className={chromeBtn}
            aria-label="Export JSON"
          >
            <FileJson size={16} />
            <span className="hidden md:inline">JSON</span>
          </button>
          <button
            onClick={() => setShowExportModal(true)}
            className={primaryBtn}
            aria-label="Export image"
          >
            <Download size={16} />
            <span className="hidden md:inline">Export</span>
          </button>
          <button
            onClick={toggleTheme}
            className={cn(chromeBtn, "w-9 px-0")}
            aria-label="Toggle theme"
          >
            {theme === "dark" ? <Sun size={16} /> : <Moon size={16} />}
          </button>
        </div>
      </div>

      <ExportPreviewModal
        isOpen={showExportModal}
        onClose={() => setShowExportModal(false)}
      />
    </header>
  );
}
