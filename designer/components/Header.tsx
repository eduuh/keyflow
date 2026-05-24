"use client";

import { useState } from "react";
import {
  Moon, Sun, Download, Upload, FileJson, Keyboard, Github,
  HardDriveDownload, Compass,
} from "lucide-react";
import { useTheme } from "./ThemeProvider";
import { useConfigStore } from "@/lib/store";
import { CategoryExport } from "./Export/CategoryExport";
import { CommunityLayoutsButton } from "./CommunityLayouts";
import { JsonEditor } from "./JsonEditor";
import { ShareButton } from "./Share";
import { Button } from "@/components/ui/button";
import { Separator } from "@/components/ui/separator";
import { startTour } from "./Tour";

// Public-facing header. Two filled primary actions side by side:
//   • Get keyflow.exe  →  GitHub's predictable "latest release" download URL
//   • Export image     →  opens the category PNG export dialog
// Plus the config-workflow cluster (Layouts / Import / JSON) and the
// project-links cluster (Tour / GitHub / theme).

const RELEASE_DOWNLOAD_URL =
  "https://github.com/eduuh/keyflow/releases/latest/download/keyflow-release.zip";
const REPO_URL = "https://github.com/eduuh/keyflow";

export function Header() {
  const { theme, toggleTheme } = useTheme();
  const { config, setConfig } = useConfigStore();
  const [showExportModal, setShowExportModal] = useState(false);
  const [showJsonEditor, setShowJsonEditor] = useState(false);

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
    <header
      data-tour="header"
      className="border-b sticky top-0 z-30 bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/80"
    >
      <div className="px-4 lg:px-6 py-3 flex items-center justify-between gap-4">
        <div className="flex items-center gap-3 min-w-0">
          <div className="p-2 rounded-md bg-modifier text-modifier-foreground shrink-0">
            <Keyboard className="h-4 w-4" />
          </div>
          <div className="leading-tight min-w-0">
            <h1 className="text-sm font-semibold truncate">Keyflow Designer</h1>
            <p className="text-xs text-muted-foreground font-mono truncate">
              {config.name || "Untitled"} · v{config.version}
            </p>
          </div>
        </div>

        <div className="flex items-center gap-1.5 flex-wrap justify-end">
          {/* Config workflow: community / import / inspect+edit JSON */}
          <CommunityLayoutsButton />
          <Button variant="outline" size="sm" onClick={handleImport}>
            <Upload className="h-4 w-4" />
            <span className="hidden md:inline ml-2">Import</span>
          </Button>
          <ShareButton />
          <Button
            variant="outline"
            size="sm"
            onClick={() => setShowJsonEditor(true)}
            title="View / edit JSON"
          >
            <FileJson className="h-4 w-4" />
            <span className="hidden md:inline ml-2">JSON</span>
          </Button>

          {/* Primary CTAs */}
          <Button asChild size="sm" data-tour="download-binary">
            <a href={RELEASE_DOWNLOAD_URL} target="_blank" rel="noopener noreferrer">
              <HardDriveDownload className="h-4 w-4" />
              <span className="hidden md:inline ml-2">Get keyflow.exe</span>
            </a>
          </Button>
          <Button size="sm" onClick={() => setShowExportModal(true)} data-tour="export-image">
            <Download className="h-4 w-4" />
            <span className="hidden md:inline ml-2">Export image</span>
          </Button>

          <Separator orientation="vertical" className="h-6 mx-1 hidden sm:block" />

          {/* Tour + project links */}
          <Button
            variant="outline"
            size="icon"
            onClick={startTour}
            aria-label="Take a tour"
            title="Take a tour"
          >
            <Compass className="h-4 w-4" />
          </Button>
          <Button asChild variant="outline" size="icon" aria-label="View on GitHub">
            <a href={REPO_URL} target="_blank" rel="noopener noreferrer">
              <Github className="h-4 w-4" />
            </a>
          </Button>
          <Button variant="outline" size="icon" onClick={toggleTheme} aria-label="Toggle theme">
            {theme === "dark" ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
          </Button>
        </div>
      </div>

      <CategoryExport isOpen={showExportModal} onClose={() => setShowExportModal(false)} />
      <JsonEditor isOpen={showJsonEditor} onClose={() => setShowJsonEditor(false)} />
    </header>
  );
}
