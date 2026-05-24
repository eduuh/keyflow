"use client";

import { useState } from "react";
import { Moon, Sun, Download, Upload, FileJson, Keyboard } from "lucide-react";
import { useTheme } from "./ThemeProvider";
import { useConfigStore } from "@/lib/store";
import { CategoryExport } from "./Export/CategoryExport";
import { Button } from "@/components/ui/button";

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
      reader.onload = (ev) => {
        try {
          const json = JSON.parse(ev.target?.result as string);
          if (!json.version) {
            alert("Invalid config: missing 'version' field");
            return;
          }
          setConfig(json);
        } catch (err) {
          alert("Invalid JSON: " + (err instanceof Error ? err.message : "unknown"));
        }
      };
      reader.readAsText(file);
    };
    input.click();
  };

  return (
    <header className="border-b">
      <div className="px-6 py-3 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <Keyboard size={20} />
          <div className="leading-tight">
            <h1 className="text-sm font-semibold">Keyflow Designer</h1>
            <p className="text-xs text-muted-foreground">
              {config.name || "Untitled"} · v{config.version}
            </p>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <Button variant="outline" size="sm" onClick={handleImport}>
            <Upload className="h-4 w-4" />
            <span className="hidden md:inline ml-2">Import</span>
          </Button>
          <Button variant="outline" size="sm" onClick={handleExportJSON}>
            <FileJson className="h-4 w-4" />
            <span className="hidden md:inline ml-2">JSON</span>
          </Button>
          <Button size="sm" onClick={() => setShowExportModal(true)}>
            <Download className="h-4 w-4" />
            <span className="hidden md:inline ml-2">Export</span>
          </Button>
          <Button variant="outline" size="icon" onClick={toggleTheme} aria-label="Toggle theme">
            {theme === "dark" ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
          </Button>
        </div>
      </div>

      <CategoryExport isOpen={showExportModal} onClose={() => setShowExportModal(false)} />
    </header>
  );
}
