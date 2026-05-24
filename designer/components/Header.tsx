"use client";

import { useState } from "react";
import { Moon, Sun, Download, Upload, FileJson } from "lucide-react";
import { useTheme } from "./ThemeProvider";
import { useConfigStore } from "@/lib/store";
import { ExportPreviewModal } from "./ExportPreviewModal";

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
    a.download = "config.json";
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleImport = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json";
    input.onchange = (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (file) {
        const reader = new FileReader();
        reader.onload = (e) => {
          try {
            const json = JSON.parse(e.target?.result as string);

            // Validate required fields
            if (!json.version) {
              alert("Invalid config: Missing 'version' field.\n\nExpected format: \"1.0\"");
              return;
            }

            // Validate version format
            if (typeof json.version !== 'string' || !/^[0-9]+\.[0-9]+$/.test(json.version)) {
              alert("Invalid config: 'version' must be in format X.Y (e.g., \"1.0\")");
              return;
            }

            // Validate layer structure if layers exist
            if (json.layers && Array.isArray(json.layers)) {
              for (let i = 0; i < json.layers.length; i++) {
                const layer = json.layers[i];
                if (!layer.name || !layer.trigger || !layer.mappings) {
                  alert(`Invalid config: Layer ${i + 1} is missing required fields (name, trigger, mappings)`);
                  return;
                }
                if (typeof layer.mappings !== 'object') {
                  alert(`Invalid config: Layer ${i + 1} mappings must be an object`);
                  return;
                }
              }
            }

            // Validate remapping if exists
            if (json.remapping && typeof json.remapping !== 'object') {
              alert("Invalid config: 'remapping' must be an object");
              return;
            }

            setConfig(json);
          } catch (error) {
            alert("Invalid JSON file: " + (error instanceof Error ? error.message : "Unknown error"));
          }
        };
        reader.readAsText(file);
      }
    };
    input.click();
  };

  return (
    <header className="border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
      <div className="px-6 py-4">
        <div className="flex items-center justify-between">
          {/* Logo and Title */}
          <div className="flex items-center gap-3">
            <div className="text-2xl">⌨️</div>
            <div>
              <h1 className="text-lg font-semibold tracking-tight">
                KeyFlow Designer
              </h1>
              <p className="text-xs text-muted-foreground">
                Configure your keyboard layout
              </p>
            </div>
          </div>

          {/* Actions */}
          <div className="flex items-center gap-2">
            {/* Import Button */}
            <button
              onClick={handleImport}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium ring-offset-background transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:pointer-events-none disabled:opacity-50 border border-input bg-background hover:bg-accent hover:text-accent-foreground h-9 px-4 gap-2"
            >
              <Upload size={16} />
              <span className="hidden sm:inline">Import</span>
            </button>

            {/* Export JSON Button */}
            <button
              onClick={handleExportJSON}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium ring-offset-background transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:pointer-events-none disabled:opacity-50 border border-input bg-background hover:bg-accent hover:text-accent-foreground h-9 px-4 gap-2"
            >
              <FileJson size={16} />
              <span className="hidden sm:inline">JSON</span>
            </button>

            {/* Export Preview Button */}
            <button
              onClick={() => setShowExportModal(true)}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium ring-offset-background transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:pointer-events-none disabled:opacity-50 bg-primary text-primary-foreground hover:bg-primary/90 h-9 px-4 gap-2"
            >
              <Download size={16} />
              <span className="hidden sm:inline">Export</span>
            </button>

            {/* Theme Toggle */}
            <button
              onClick={toggleTheme}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium ring-offset-background transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 disabled:pointer-events-none disabled:opacity-50 border border-input bg-background hover:bg-accent hover:text-accent-foreground h-9 w-9"
              aria-label="Toggle theme"
            >
              {theme === "dark" ? (
                <Sun size={18} />
              ) : (
                <Moon size={18} />
              )}
            </button>
          </div>
        </div>
      </div>

      <ExportPreviewModal
        isOpen={showExportModal}
        onClose={() => setShowExportModal(false)}
      />
    </header>
  );
}
